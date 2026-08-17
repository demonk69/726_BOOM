#include "boom_interfaces.hpp"
#include "boom_state.hpp"

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

struct Token {
    uint64_t pc;
    uint32_t instruction;
    uint32_t original;
    uint32_t fetch_id;
    uint32_t epoch;
    uint64_t cause;
    bool rvc;
    bool fault;
};

struct PendingResponse {
    ImemResponse response;
    uint32_t due;
    uint32_t generation;
    bool cross_request;
};

struct Coverage {
    uint64_t requests;
    uint64_t delayed;
    uint64_t decode_stalls;
    uint64_t runtime_resets;
    uint64_t architectural_redirects;
    uint64_t branch_redirects;
    uint64_t generic_flushes;
    uint64_t stale_id;
    uint64_t stale_epoch;
    uint64_t stale_address;
    uint64_t faults;
    uint64_t rvc;
    uint64_t base;
    uint64_t carry_created;
    uint64_t cross_word_completions;
    uint64_t near_full;
    uint64_t full;

    Coverage() : requests(0), delayed(0), decode_stalls(0), runtime_resets(0),
        architectural_redirects(0), branch_redirects(0), generic_flushes(0),
        stale_id(0), stale_epoch(0), stale_address(0), faults(0), rvc(0),
        base(0), carry_created(0), cross_word_completions(0), near_full(0),
        full(0) {}
};

Token producer_token(const FrontendState& fe) {
    Token token = {fe.producer_uop.debug_pc, fe.producer_uop.inst,
                   fe.producer_uop.debug_inst, fe.producer_fetch_id, fe.epoch,
                   fe.producer_uop.exc_cause, fe.producer_uop.is_rvc,
                   fe.producer_uop.exception};
    return token;
}

bool equal(const Token& token, const MicroOp& uop) {
    return token.pc == uop.debug_pc && token.instruction == uop.inst &&
           token.original == uop.debug_inst && token.rvc == uop.is_rvc &&
           token.fault == uop.exception && token.cause == uop.exc_cause;
}

uint32_t memory_word(uint64_t address, uint32_t seed) {
    // The first pattern creates a 32-bit instruction in the upper parcel.  The
    // next word both completes it and is fetched again at its upper RVC parcel.
    switch (((address >> 2) + seed) & 3u) {
    case 0: return 0x00930001u;
    case 1: return 0x00010001u;
    case 2: return 0x00108093u;
    default: return 0x00010001u;
    }
}

bool all_coverage(const Coverage& c) {
    return c.requests && c.delayed && c.decode_stalls && c.runtime_resets &&
        c.architectural_redirects && c.branch_redirects && c.generic_flushes &&
        c.stale_id && c.stale_epoch && c.stale_address && c.faults && c.rvc &&
        c.base && c.carry_created && c.cross_word_completions && c.near_full &&
        c.full;
}

}  // namespace

int main() {
    uint64_t produced = 0, consumed = 0, killed = 0, stale = 0;
    uint64_t drops = 0, duplicates = 0, ordering = 0;
    uint64_t stale_side_effect = 0, post_flush_old_entry = 0;
    Coverage coverage;

    for (uint32_t seed = 0; seed < 256; ++seed) {
        Rng rng(0x9e3779b97f4a7c15ULL ^ seed);
        BoomCoreState state;
        PipeSignals pipe;
        std::deque<PendingResponse> pending;
        std::deque<Token> scoreboard;
        std::set<std::tuple<uint32_t, uint32_t, uint64_t> > consumed_keys;
        uint32_t generation = 0;
        uint64_t request_number = 0;

        for (uint32_t cycle = 0; cycle < 4096; ++cycle) {
            const uint32_t random = rng.next();
            const bool runtime_reset = cycle != 0 &&
                (cycle % 997 == 113 || (random & 0x7fffu) == 0);
            const bool architectural_redirect = !runtime_reset && cycle != 0 &&
                (cycle % 431 == 47 || (random & 0xffffu) == 1);
            const bool branch_redirect = !runtime_reset && !architectural_redirect &&
                cycle != 0 && (cycle % 337 == 71 || (random & 0x7fffu) == 2);
            const bool generic_flush = !runtime_reset && !architectural_redirect &&
                !branch_redirect && cycle != 0 &&
                (cycle % 293 == 89 || (random & 0xffffu) == 3);
            const bool flush = runtime_reset || architectural_redirect ||
                branch_redirect || generic_flush;

            // Long stall windows guarantee pressure; random ready cycles exercise
            // simultaneous dequeue/enqueue and wraparound.
            const bool forced_stall = (cycle & 127u) < 48u;
            const bool decode_ready = !forced_stall && ((random >> 12) & 3u) != 0;
            if (!decode_ready) ++coverage.decode_stalls;

            state.brupdate = BranchUpdate();
            state.global_flush = generic_flush;
            if (runtime_reset) {
                state.frontend.reset_done = false;
                ++generation;
                ++coverage.runtime_resets;
            } else if (architectural_redirect) {
                state.frontend_redirect.valid = true;
                state.frontend_redirect.cause = FRONTEND_REDIRECT_DEBUG;
                state.frontend_redirect.target_pc = RESET_VECTOR +
                    (static_cast<uint64_t>((cycle + seed) & 255u) << 2) +
                    (((cycle + seed) & 1u) ? 2u : 0u);
                ++generation;
                ++coverage.architectural_redirects;
            } else if (branch_redirect) {
                state.brupdate.valid = true;
                state.brupdate.mispredict = true;
                state.brupdate.jalr_target = RESET_VECTOR +
                    (static_cast<uint64_t>((cycle * 3 + seed) & 255u) << 2);
                ++generation;
                ++coverage.branch_redirects;
            } else if (generic_flush) {
                ++generation;
                ++coverage.generic_flushes;
            }

            if (decode_ready) {
                state.decode.dec_valids[0] = false;
                state.rename.dispatch_packets[0] = RenameDispatchPacket();
            } else {
                state.rename.dispatch_packets[0].valid = true;
            }

            bool injected_matching = false;
            bool injected_stale = false;
            bool matching_cross = false;
            if (!flush && state.frontend.request_sent && cycle % 127 == 17) {
                ImemResponse bad;
                bad.address = state.frontend.pending_address;
                bad.fetch_id = state.frontend.pending_fetch_id + 1;
                bad.epoch = state.frontend.pending_epoch;
                bad.instruction = 0xffffffffu;
                pipe.imem_resp.write(bad);
                injected_stale = true;
                ++stale;
                ++coverage.stale_id;
            } else if (!flush && state.frontend.request_sent && cycle % 131 == 19) {
                ImemResponse bad;
                bad.address = state.frontend.pending_address;
                bad.fetch_id = state.frontend.pending_fetch_id;
                bad.epoch = state.frontend.pending_epoch + 1;
                bad.instruction = 0xffffffffu;
                pipe.imem_resp.write(bad);
                injected_stale = true;
                ++stale;
                ++coverage.stale_epoch;
            } else if (!flush && state.frontend.request_sent && cycle % 137 == 23) {
                ImemResponse bad;
                bad.address = state.frontend.pending_address + 4;
                bad.fetch_id = state.frontend.pending_fetch_id;
                bad.epoch = state.frontend.pending_epoch;
                bad.instruction = 0xffffffffu;
                pipe.imem_resp.write(bad);
                injected_stale = true;
                ++stale;
                ++coverage.stale_address;
            } else {
                for (std::deque<PendingResponse>::iterator it = pending.begin();
                     it != pending.end(); ++it) {
                    if (it->due <= cycle) {
                        pipe.imem_resp.write(it->response);
                        injected_matching = it->generation == generation &&
                            state.frontend.request_sent &&
                            it->response.address == state.frontend.pending_address &&
                            it->response.fetch_id == state.frontend.pending_fetch_id &&
                            it->response.epoch == state.frontend.pending_epoch;
                        matching_cross = injected_matching && it->cross_request;
                        if (!injected_matching) ++stale;
                        pending.erase(it);
                        break;
                    }
                }
            }

            const bool old_producer_valid = state.frontend.producer_valid;
            const Token old_producer = old_producer_valid ?
                producer_token(state.frontend) : Token();
            const bool expected_pop = !flush && decode_ready && !scoreboard.empty();
            const Token expected = expected_pop ? scoreboard.front() : Token();
            const bool expected_push = !flush && old_producer_valid &&
                (scoreboard.size() < FETCH_BUFFER_DEPTH || expected_pop);

            if (flush) {
                killed += scoreboard.size() + (old_producer_valid ? 1u : 0u);
                scoreboard.clear();
            } else {
                if (expected_pop) scoreboard.pop_front();
                if (expected_push) {
                    scoreboard.push_back(old_producer);
                    ++produced;
                }
            }

            const bool old_halfword = state.frontend.halfword_valid;
            const uint64_t old_halfword_pc = state.frontend.halfword_pc;
            const bool old_response_received = state.frontend.response_received;
            const uint64_t old_pc = state.frontend.pc;
            const uint32_t old_epoch = state.frontend.epoch;
            boom::frontend_module(state, pipe);
            boom::decode_module(state);

            if (expected_pop) {
                if (!state.decode.dec_valids[0] ||
                    !equal(expected, state.decode.dec_uops[0])) {
                    ++ordering;
                } else {
                    const std::tuple<uint32_t, uint32_t, uint64_t> key(
                        expected.epoch, expected.fetch_id, expected.pc);
                    if (!consumed_keys.insert(key).second) ++duplicates;
                    ++consumed;
                }
            } else if (decode_ready && state.decode.dec_valids[0]) {
                if (flush) ++post_flush_old_entry;
                else ++ordering;
            }

            if (state.frontend.fetch_buffer.count != scoreboard.size()) ++drops;
            if (!scoreboard.empty()) {
                const boom::FetchInstruction& entry = state.frontend.fetch_buffer.entries[
                    state.frontend.fetch_buffer.head];
                if (entry.pc != scoreboard.front().pc ||
                    entry.instruction != scoreboard.front().instruction ||
                    entry.fetch_id != scoreboard.front().fetch_id) {
                    ++ordering;
                }
            }
            if (state.frontend.fetch_buffer.count == FETCH_BUFFER_DEPTH) ++coverage.full;
            if (state.frontend.fetch_buffer.count + 1 >= FETCH_BUFFER_DEPTH)
                ++coverage.near_full;

            if (!old_halfword && state.frontend.halfword_valid)
                ++coverage.carry_created;
            if (matching_cross && old_halfword && state.frontend.producer_valid &&
                state.frontend.producer_uop.debug_pc == old_halfword_pc)
                ++coverage.cross_word_completions;
            if (injected_stale && !old_response_received &&
                state.frontend.response_received)
                ++stale_side_effect;
            if (injected_stale && state.frontend.producer_valid &&
                !old_producer_valid && state.frontend.producer_uop.debug_pc == old_pc &&
                state.frontend.epoch == old_epoch)
                ++stale_side_effect;

            if (!old_producer_valid && state.frontend.producer_valid) {
                if (state.frontend.producer_uop.exception) ++coverage.faults;
                else if (state.frontend.producer_uop.is_rvc) ++coverage.rvc;
                else ++coverage.base;
            }

            while (!pipe.imem_req.empty()) {
                const ImemRequest request = pipe.imem_req.read();
                PendingResponse item;
                item.response.address = request.address;
                item.response.fetch_id = request.fetch_id;
                item.response.epoch = request.epoch;
                item.response.instruction = memory_word(request.address, seed);
                item.cross_request = state.frontend.halfword_valid;
                item.response.exception = (++request_number % 29u) == 0;
                item.response.exc_cause = item.response.exception ? 12 : 0;
                const uint32_t delay = rng.next() % 13u;
                item.due = cycle + delay + 1;
                item.generation = generation;
                pending.push_back(item);
                ++coverage.requests;
                if (delay) ++coverage.delayed;
            }
            state.global_flush = false;
        }
    }

    const bool pass = drops == 0 && duplicates == 0 && ordering == 0 &&
        stale_side_effect == 0 && post_flush_old_entry == 0 && produced && consumed &&
        killed && stale && all_coverage(coverage);
    std::cout << "GATE5_3_B2_FETCH_BUFFER_INTEGRATION_RANDOM_"
              << (pass ? "PASS" : "FAIL")
              << " seeds=256 cycles_per_seed=4096 produced=" << produced
              << " consumed=" << consumed << " killed=" << killed
              << " stale=" << stale << " requests=" << coverage.requests
              << " delayed=" << coverage.delayed
              << " decode_stall=" << coverage.decode_stalls
              << " runtime_reset=" << coverage.runtime_resets
              << " arch_redirect=" << coverage.architectural_redirects
              << " branch_redirect=" << coverage.branch_redirects
              << " generic_flush=" << coverage.generic_flushes
              << " stale_id=" << coverage.stale_id
              << " stale_epoch=" << coverage.stale_epoch
              << " stale_address=" << coverage.stale_address
              << " faults=" << coverage.faults << " rvc=" << coverage.rvc
              << " base=" << coverage.base
              << " carry_created=" << coverage.carry_created
              << " cross_word_completion=" << coverage.cross_word_completions
              << " full=" << coverage.full << " near_full=" << coverage.near_full
              << " drop=" << drops << " duplicate=" << duplicates
              << " ordering_error=" << ordering
              << " stale_side_effect=" << stale_side_effect
              << " post_flush_old_entry=" << post_flush_old_entry << '\n';
    return pass ? EXIT_SUCCESS : EXIT_FAILURE;
}
