#include "boom_interfaces.hpp"
#include "boom_state.hpp"
#include "rvc.hpp"

#include <cstdint>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <set>
#include <tuple>

namespace boom {
void frontend_module(BoomCoreState&, PipeSignals&);
void decode_module(BoomCoreState&);
}

namespace {

struct Rng {
    uint64_t value;
    explicit Rng(uint64_t seed) : value(seed | 1u) {}
    uint32_t next() {
        value ^= value << 13;
        value ^= value >> 7;
        value ^= value << 17;
        return static_cast<uint32_t>(value);
    }
};

struct ExpectedPacket {
    bool valid;
    uint8_t mask;
    boom::FetchInstruction slots[2];
    ExpectedPacket() : valid(false), mask(0), slots() {}
};

struct Oracle {
    uint64_t pc;
    bool carry_valid;
    uint16_t carry;
    uint64_t carry_pc;
    bool response_waiting;
    ImemResponse response;
    ExpectedPacket pending;
    std::deque<boom::FetchInstruction> fifo;
    Oracle() : pc(RESET_VECTOR), carry_valid(false), carry(0), carry_pc(0),
               response_waiting(false), response(), pending(), fifo() {}
};

struct Delayed {
    ImemResponse response;
    uint32_t due;
    uint32_t generation;
};

struct Counters {
    uint64_t matched, stale, carry, packet0, packet1, packet2;
    uint64_t stalls, resets, redirects, faults, illegal, full, atomic_wait;
    uint64_t produced, consumed, killed;
    uint64_t drop, duplicate, order_error, bad_pc, packet_mask_error;
    uint64_t partial_enqueue, stale_side_effect, atomicity, mirror;
    Counters() : matched(0), stale(0), carry(0), packet0(0), packet1(0), packet2(0),
        stalls(0), resets(0), redirects(0), faults(0), illegal(0), full(0),
        atomic_wait(0), produced(0), consumed(0), killed(0), drop(0), duplicate(0),
        order_error(0), bad_pc(0), packet_mask_error(0), partial_enqueue(0),
        stale_side_effect(0), atomicity(0), mirror(0) {}
};

bool equal_entry(const boom::FetchInstruction& a, const boom::FetchInstruction& b) {
    return a.pc == b.pc && a.instruction == b.instruction &&
        a.original_instruction == b.original_instruction && a.fetch_id == b.fetch_id &&
        a.exception_cause == b.exception_cause && a.is_rvc == b.is_rvc &&
        a.exception == b.exception &&
        a.exception_access_fault == b.exception_access_fault &&
        a.exception_misaligned == b.exception_misaligned;
}

boom::FetchInstruction base(uint64_t pc, uint32_t bits, uint32_t id) {
    boom::FetchInstruction e;
    e.pc = pc;
    e.instruction = bits;
    e.fetch_id = id;
    return e;
}

boom::FetchInstruction parcel(uint64_t pc, uint16_t bits, uint32_t id) {
    boom::FetchInstruction e;
    e.pc = pc;
    e.original_instruction = bits;
    e.fetch_id = id;
    e.is_rvc = true;
    const boom::RvcDecodeResult decoded = boom::decompress_rvc(bits);
    if (decoded.valid && decoded.legal) {
        e.instruction = decoded.instruction;
    } else {
        e.exception = true;
        e.exception_cause = 2;
    }
    return e;
}

void append(ExpectedPacket& packet, const boom::FetchInstruction& entry) {
    const unsigned lane = packet.mask == 0 ? 0 : 1;
    packet.slots[lane] = entry;
    packet.mask = lane == 0 ? 1 : 3;
    packet.valid = true;
}

bool append_parcel(ExpectedPacket& packet, uint64_t pc, uint16_t bits, uint32_t id) {
    if ((bits & 3u) != 3u) {
        const boom::FetchInstruction entry = parcel(pc, bits, id);
        append(packet, entry);
        return entry.exception;
    }
    if ((bits & 0x1fu) == 0x1fu) {
        boom::FetchInstruction entry;
        entry.pc = pc;
        entry.original_instruction = bits;
        entry.fetch_id = id;
        entry.exception = true;
        entry.exception_cause = 2;
        entry.exception_misaligned = false;
        append(packet, entry);
        return true;
    }
    return false;
}

ExpectedPacket parse_response(Oracle& oracle, const ImemResponse& response,
                              Counters& count) {
    ExpectedPacket packet;
    const uint64_t first_pc = oracle.carry_valid ? oracle.carry_pc : oracle.pc;
    if (response.exception) {
        boom::FetchInstruction fault;
        fault.pc = first_pc;
        fault.fetch_id = response.fetch_id;
        fault.exception = true;
        fault.exception_cause = response.exc_cause;
        fault.exception_access_fault = true;
        append(packet, fault);
        oracle.pc = first_pc + 4;
        oracle.carry_valid = false;
        ++count.faults;
        ++count.packet1;
        return packet;
    }

    if (oracle.carry_valid) {
        append(packet, base(oracle.carry_pc,
                            (response.instruction << 16) | oracle.carry,
                            response.fetch_id));
        const uint64_t upper_pc = oracle.carry_pc + 4;
        const uint16_t upper = static_cast<uint16_t>(response.instruction >> 16);
        oracle.pc = upper_pc + 2;
        oracle.carry_valid = false;
        if ((upper & 3u) == 3u && (upper & 0x1fu) != 0x1fu) {
            oracle.carry_valid = true;
            oracle.carry = upper;
            oracle.carry_pc = upper_pc;
            oracle.pc = upper_pc;
            ++count.carry;
        } else {
            append_parcel(packet, upper_pc, upper, response.fetch_id);
        }
    } else {
        const bool upper_start = (oracle.pc & 2u) != 0;
        const uint16_t first = upper_start ?
            static_cast<uint16_t>(response.instruction >> 16) :
            static_cast<uint16_t>(response.instruction);
        if ((first & 3u) == 3u && (first & 0x1fu) != 0x1fu) {
            if (upper_start) {
                oracle.carry_valid = true;
                oracle.carry = first;
                oracle.carry_pc = oracle.pc;
                ++count.carry;
            } else {
                append(packet, base(oracle.pc, response.instruction, response.fetch_id));
                oracle.pc += 4;
            }
        } else {
            const bool terminal = append_parcel(packet, oracle.pc, first, response.fetch_id);
            oracle.pc += 2;
            if (!terminal && !upper_start) {
                const uint16_t upper = static_cast<uint16_t>(response.instruction >> 16);
                if ((upper & 3u) == 3u && (upper & 0x1fu) != 0x1fu) {
                    oracle.carry_valid = true;
                    oracle.carry = upper;
                    oracle.carry_pc = oracle.pc;
                    ++count.carry;
                } else {
                    append_parcel(packet, oracle.pc, upper, response.fetch_id);
                    oracle.pc += 2;
                }
            }
        }
    }
    if (!packet.valid) ++count.packet0;
    else if (packet.mask == 1) ++count.packet1;
    else ++count.packet2;
    if (packet.valid && packet.slots[packet.mask == 3 ? 1 : 0].exception) ++count.illegal;
    return packet;
}

uint32_t memory_word(uint64_t address, uint32_t seed) {
    switch (((address >> 2) + seed) % 9u) {
    case 0: return 0x00010001u; // two legal compressed instructions
    case 1: return 0x00108093u; // one aligned base instruction
    case 2: return 0x00930001u; // compressed plus cross-word base start
    case 3: return 0x00010010u; // completes carry plus compressed
    case 4: return 0x00000001u; // legal then illegal compressed
    case 5: return 0x001f0001u; // legal then unsupported long encoding
    case 6: return 0x00010085u;
    case 7: return 0x00930085u;
    default: return 0x00010001u;
    }
}

unsigned packet_count(const ExpectedPacket& packet) {
    return packet.valid ? (packet.mask == 3 ? 2u : 1u) : 0u;
}

void compare_pending(const Oracle& oracle, const FrontendState& fe, Counters& count) {
    if (oracle.pending.valid != fe.pending_packet.valid ||
        oracle.pending.mask != fe.pending_packet.valid_mask) {
        ++count.mirror;
        return;
    }
    for (unsigned lane = 0; lane < 2; ++lane)
        if ((oracle.pending.mask >> lane) & 1u) {
            if (oracle.pending.slots[lane].pc != fe.pending_packet.slots[lane].pc)
                ++count.bad_pc;
            else if (!equal_entry(oracle.pending.slots[lane], fe.pending_packet.slots[lane]))
                ++count.mirror;
        }
    if (fe.producer_valid != fe.pending_packet.valid ||
        fe.stalled != fe.pending_packet.valid) ++count.mirror;
}

}  // namespace

int main() {
    Counters count;
    for (uint32_t seed = 0; seed < 256; ++seed) {
        Rng rng(0xd1b54a32d192ed03ULL ^ seed);
        BoomCoreState state;
        PipeSignals pipe;
        Oracle oracle;
        std::deque<Delayed> delayed;
        std::set<std::tuple<uint32_t, uint32_t, uint64_t> > consumed;
        uint32_t generation = 0;
        uint64_t request_number = 0;

        for (uint32_t cycle = 0; cycle < 4096; ++cycle) {
            const uint32_t random = rng.next();
            const bool runtime_reset = cycle == 0 || (cycle % 1009u) == 127u;
            const bool arch_redirect = !runtime_reset && (cycle % 487u) == 53u;
            const bool branch_redirect = !runtime_reset && !arch_redirect &&
                (cycle % 383u) == 79u;
            const bool generic_flush = !runtime_reset && !arch_redirect &&
                !branch_redirect && (cycle % 307u) == 101u;
            const bool rob_exception = !runtime_reset && !arch_redirect &&
                !branch_redirect && !generic_flush && (cycle % 1201u) == 211u;
            const bool kill = runtime_reset || arch_redirect || branch_redirect ||
                generic_flush || rob_exception;
            const uint64_t target = RESET_VECTOR +
                (static_cast<uint64_t>((cycle * 5u + seed) & 0x3ffu) << 1);

            state.brupdate = BranchUpdate();
            state.frontend_redirect = FrontendRedirect();
            state.global_flush = generic_flush;
            state.rob.state = rob_exception ? ROB_EXCEPTION : ROB_NORMAL;
            if (runtime_reset) {
                state.frontend.reset_done = false;
                ++count.resets;
                ++generation;
            } else if (arch_redirect) {
                state.frontend_redirect.valid = true;
                state.frontend_redirect.cause = FRONTEND_REDIRECT_DEBUG;
                state.frontend_redirect.target_pc = target;
                ++count.redirects;
                ++generation;
            } else if (branch_redirect) {
                state.brupdate.valid = true;
                state.brupdate.mispredict = true;
                state.brupdate.jalr_target = target;
                ++count.redirects;
                ++generation;
            } else if (generic_flush || rob_exception) {
                ++count.redirects;
                ++generation;
            }

            const bool forced_stall = (cycle & 127u) < 54u;
            const bool ready = !forced_stall && ((random >> 8) & 3u) != 0;
            if (!ready) ++count.stalls;
            if (ready) {
                state.decode.dec_valids[0] = false;
                state.rename.dispatch_packets[0] = RenameDispatchPacket();
            } else {
                state.rename.dispatch_packets[0].valid = true;
            }

            bool injected_match = false;
            bool injected_stale = false;
            if (!kill && state.frontend.request_sent && cycle % 149u == 17u) {
                ImemResponse stale;
                stale.address = state.frontend.pending_address;
                stale.fetch_id = state.frontend.pending_fetch_id + 1;
                stale.epoch = state.frontend.pending_epoch;
                stale.instruction = 0xffffffffu;
                pipe.imem_resp.write(stale);
                injected_stale = true;
                ++count.stale;
            } else if (!kill && state.frontend.request_sent && cycle % 151u == 19u) {
                ImemResponse stale;
                stale.address = state.frontend.pending_address + 4;
                stale.fetch_id = state.frontend.pending_fetch_id;
                stale.epoch = state.frontend.pending_epoch;
                stale.instruction = 0xffffffffu;
                pipe.imem_resp.write(stale);
                injected_stale = true;
                ++count.stale;
            } else if (!kill && state.frontend.request_sent && cycle % 157u == 23u) {
                ImemResponse stale;
                stale.address = state.frontend.pending_address;
                stale.fetch_id = state.frontend.pending_fetch_id;
                stale.epoch = state.frontend.pending_epoch + 1;
                stale.instruction = 0xffffffffu;
                pipe.imem_resp.write(stale);
                injected_stale = true;
                ++count.stale;
            } else if (!delayed.empty() && delayed.front().due <= cycle) {
                const Delayed item = delayed.front();
                delayed.pop_front();
                pipe.imem_resp.write(item.response);
                injected_match = item.generation == generation && !kill &&
                    state.frontend.request_sent &&
                    item.response.address == state.frontend.pending_address &&
                    item.response.fetch_id == state.frontend.pending_fetch_id &&
                    item.response.epoch == state.frontend.pending_epoch;
                if (injected_match) {
                    oracle.response = item.response;
                    oracle.response_waiting = true;
                    ++count.matched;
                } else {
                    ++count.stale;
                    injected_stale = true;
                }
            }

            const bool old_response = state.frontend.response_received;
            boom::FetchInstruction expected_decode;
            bool expected_pop = false;
            bool expected_admit = false;
            const unsigned actual_count_before = state.frontend.fetch_buffer.count;
            const bool expected_two_lane = !kill && oracle.pending.valid &&
                oracle.pending.mask == 3;

            if (kill) {
                count.killed += oracle.fifo.size() + packet_count(oracle.pending);
                oracle.fifo.clear();
                oracle.pending = ExpectedPacket();
                oracle.response_waiting = false;
                oracle.carry_valid = false;
                if (runtime_reset) oracle.pc = RESET_VECTOR;
                else if (arch_redirect || branch_redirect) oracle.pc = target;
            } else {
                expected_pop = ready && !oracle.fifo.empty();
                const unsigned incoming_before = packet_count(oracle.pending);
                expected_admit = oracle.pending.valid && incoming_before <=
                    FETCH_BUFFER_DEPTH - oracle.fifo.size() + (expected_pop ? 1u : 0u);
                if (expected_pop) {
                    expected_decode = oracle.fifo.front();
                    oracle.fifo.pop_front();
                }
                const unsigned incoming = packet_count(oracle.pending);
                const unsigned available = FETCH_BUFFER_DEPTH - oracle.fifo.size();
                if (oracle.pending.valid && incoming <= available) {
                    for (unsigned lane = 0; lane < 2; ++lane)
                        if ((oracle.pending.mask >> lane) & 1u) {
                            oracle.fifo.push_back(oracle.pending.slots[lane]);
                            ++count.produced;
                        }
                    oracle.pending = ExpectedPacket();
                } else if (incoming == 2 && available == 1) {
                    ++count.atomic_wait;
                }
                if (!oracle.pending.valid && oracle.response_waiting) {
                    oracle.pending = parse_response(oracle, oracle.response, count);
                    oracle.response_waiting = false;
                }
            }

            boom::frontend_module(state, pipe);
            boom::decode_module(state);

            const uint8_t actual_mask = state.frontend.pending_packet.valid ?
                state.frontend.pending_packet.valid_mask : 0;
            if ((state.frontend.pending_packet.valid &&
                 actual_mask != 1 && actual_mask != 3) ||
                (!state.frontend.pending_packet.valid && actual_mask != 0) ||
                actual_mask == 2) ++count.packet_mask_error;
            if (expected_two_lane) {
                const int admitted = static_cast<int>(state.frontend.fetch_buffer.count) -
                    static_cast<int>(actual_count_before) + (expected_pop ? 1 : 0);
                if (admitted == 1) ++count.partial_enqueue;
                if ((expected_admit && admitted != 2) || (!expected_admit && admitted != 0))
                    ++count.atomicity;
            }

            if (expected_pop) {
                if (!state.decode.dec_valids[0]) {
                    ++count.order_error;
                } else if (state.decode.dec_uops[0].debug_pc != expected_decode.pc) {
                    ++count.bad_pc;
                } else if (state.decode.dec_uops[0].inst != expected_decode.instruction ||
                    state.decode.dec_uops[0].debug_inst != expected_decode.original_instruction ||
                    state.decode.dec_uops[0].is_rvc != expected_decode.is_rvc ||
                    state.decode.dec_uops[0].exception != expected_decode.exception ||
                    state.decode.dec_uops[0].exc_cause != expected_decode.exception_cause) {
                    ++count.order_error;
                } else {
                    const std::tuple<uint32_t, uint32_t, uint64_t> key(
                        state.frontend.epoch, expected_decode.fetch_id, expected_decode.pc);
                    if (!consumed.insert(key).second) ++count.duplicate;
                    ++count.consumed;
                }
            } else if (state.decode.dec_valids[0] && ready) {
                ++count.order_error;
            }

            if (state.frontend.fetch_buffer.count != oracle.fifo.size()) ++count.drop;
            if (!oracle.fifo.empty()) {
                const boom::FetchInstruction& actual = state.frontend.fetch_buffer.entries[
                    state.frontend.fetch_buffer.head];
                if (actual.pc != oracle.fifo.front().pc) ++count.bad_pc;
                else if (!equal_entry(actual, oracle.fifo.front())) ++count.order_error;
            }
            compare_pending(oracle, state.frontend, count);
            if (state.frontend.fetch_buffer.count == FETCH_BUFFER_DEPTH) ++count.full;
            if (injected_stale && !injected_match && !old_response &&
                state.frontend.response_received) ++count.stale_side_effect;

            while (!pipe.imem_req.empty()) {
                const ImemRequest request = pipe.imem_req.read();
                Delayed item;
                item.response.address = request.address;
                item.response.fetch_id = request.fetch_id;
                item.response.epoch = request.epoch;
                item.response.instruction = memory_word(request.address, seed);
                item.response.exception = (++request_number % 37u) == 0;
                item.response.exc_cause = item.response.exception ? 12 : 0;
                item.due = cycle + 1 + rng.next() % 11u;
                item.generation = generation;
                delayed.push_back(item);
            }
            state.global_flush = false;
        }
    }

    const bool coverage = count.matched && count.stale && count.carry && count.packet0 &&
        count.packet1 && count.packet2 && count.stalls && count.resets && count.redirects &&
        count.faults && count.illegal && count.full && count.atomic_wait && count.produced &&
        count.consumed && count.killed;
    const bool pass = coverage && count.drop == 0 && count.duplicate == 0 &&
        count.packet_mask_error == 0 && count.partial_enqueue == 0 &&
        count.bad_pc == 0 && count.order_error == 0 &&
        count.stale_side_effect == 0 && count.atomicity == 0 && count.mirror == 0;
    std::cout << "GATE5_3_B3I_FETCH_PACKET_2LANE_RANDOM_" << (pass ? "PASS" : "FAIL")
              << " seeds=256 cycles_per_seed=4096 matched=" << count.matched
              << " stale=" << count.stale << " carry=" << count.carry
              << " packet0=" << count.packet0 << " packet1=" << count.packet1
              << " packet2=" << count.packet2 << " stalls=" << count.stalls
              << " resets=" << count.resets << " redirects=" << count.redirects
              << " faults=" << count.faults << " illegal=" << count.illegal
              << " full=" << count.full << " atomic_wait=" << count.atomic_wait
              << " produced=" << count.produced << " consumed=" << count.consumed
              << " killed=" << count.killed << " drop=" << count.drop
              << " duplicate=" << count.duplicate
              << " packet_mask_error=" << count.packet_mask_error
              << " partial_enqueue=" << count.partial_enqueue
              << " bad_pc=" << count.bad_pc << " order_error=" << count.order_error
              << " stale_side_effect=" << count.stale_side_effect
              << " atomicity_error=" << count.atomicity
              << " mirror_error=" << count.mirror << '\n';
    return pass ? EXIT_SUCCESS : EXIT_FAILURE;
}
