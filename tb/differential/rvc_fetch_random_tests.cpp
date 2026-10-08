#include "boom_interfaces.hpp"
#include "boom_state.hpp"
#include "rvc.hpp"

#include <cstdint>
#include <cstdio>
#include <deque>
#include <vector>

namespace boom { void frontend_module(BoomCoreState&, PipeSignals&); }

namespace {

const unsigned kSeeds = 256;
const unsigned kCycles = 2048;

struct Rng {
    uint64_t s;
    explicit Rng(uint64_t seed) : s(seed ? seed : 1) {}
    uint64_t next() {
        uint64_t x = s;
        x ^= x >> 12;
        x ^= x << 25;
        x ^= x >> 27;
        s = x;
        return x * 2685821657736338717ULL;
    }
    unsigned range(unsigned n) { return static_cast<unsigned>(next() % n); }
};

static uint64_t mix64(uint64_t x) {
    x ^= x >> 30;
    x *= 0xbf58476d1ce4e5b9ULL;
    x ^= x >> 27;
    x *= 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}

enum WordClass {
    WORD_SUPPORTED_RVC,
    WORD_BASE32,
    WORD_RESERVED_RVC,
    WORD_PROTECTED_RVC
};

static unsigned imem_word_class(uint64_t address, uint64_t seed) {
    const uint64_t h = mix64((address >> 2) ^ (seed * 0x9e3779b97f4a7c15ULL));
    return static_cast<unsigned>(h & 3u);
}

struct Coverage {
    uint64_t words[4];
    uint64_t backpressure, latency, stale_id, stale_epoch, stale_address;
    uint64_t redirects, redirects_upper, redirects_odd, resets, stalls;
    uint64_t accepted_responses, publications, compressed, base32, illegal;
    uint64_t faults, access_faults, cross_upper_faults, cross_upper_successes, kills, carries;
    uint64_t post_reset_stale, held_publications;
    Coverage() : backpressure(0), latency(0), stale_id(0), stale_epoch(0),
        stale_address(0), redirects(0), redirects_upper(0), redirects_odd(0),
        resets(0), stalls(0), accepted_responses(0), publications(0),
        compressed(0), base32(0), illegal(0), faults(0), access_faults(0),
        cross_upper_faults(0), cross_upper_successes(0), kills(0), carries(0),
        post_reset_stale(0), held_publications(0) {
        for (unsigned i = 0; i < 4; ++i) words[i] = 0;
    }
};

static const uint16_t kSupported[] = {
    0x0001, 0x0085, 0x4105, 0x6141, 0x852e, 0x9782, 0xc006,
    0x4000, 0x6000, 0xc000, 0xe000, 0x6105
};
static const uint16_t kReserved[] = {
    0x0000, 0x2000, 0xa000, 0x2002, 0xa002, 0x4002, 0x6002, 0x8002
};
static const uint32_t kBase[] = {
    0x00100093u, 0x00208113u, 0x00310193u, 0x00418213u,
    0x00520293u, 0x00628313u, 0x00730393u, 0x00838413u
};

static uint32_t imem_word(uint64_t address, uint64_t seed, Coverage& cov) {
    const uint64_t h = mix64((address >> 2) ^ (seed * 0x9e3779b97f4a7c15ULL));
    const unsigned kind = imem_word_class(address, seed);
    ++cov.words[kind];
    if (kind == WORD_BASE32)
        return kBase[(h >> 8) % (sizeof(kBase) / sizeof(kBase[0]))];

    uint16_t lo = 0, hi = 0;
    if (kind == WORD_SUPPORTED_RVC) {
        lo = kSupported[(h >> 8) % (sizeof(kSupported) / sizeof(kSupported[0]))];
        hi = ((h >> 24) & 3u) == 0 ?
            static_cast<uint16_t>(kBase[(h >> 16) % (sizeof(kBase) / sizeof(kBase[0]))]) :
            kSupported[(h >> 16) % (sizeof(kSupported) / sizeof(kSupported[0]))];
    } else if (kind == WORD_RESERVED_RVC) {
        lo = kReserved[(h >> 8) % (sizeof(kReserved) / sizeof(kReserved[0]))];
        hi = kReserved[(h >> 16) % (sizeof(kReserved) / sizeof(kReserved[0]))];
    } else {
        lo = ((h >> 8) & 1u) ? 0x9002u : 0x9001u;
        hi = ((h >> 9) & 1u) ? 0x9002u : 0x9001u;
    }
    return static_cast<uint32_t>(lo) | (static_cast<uint32_t>(hi) << 16);
}

static bool expected_rvc(uint16_t parcel, uint32_t& instruction) {
    const boom::RvcDecodeResult decoded = boom::decompress_rvc(parcel);
    instruction = decoded.instruction;
    return decoded.legal;
}

struct ExpectedUop {
    uint64_t pc;
    uint32_t inst;
    uint32_t debug_inst;
    bool is_rvc;
    bool exception;
    uint64_t exc_cause;
    bool access_fault;
    bool misaligned;
    unsigned enqueued_cycle;
    ExpectedUop() : pc(0), inst(0), debug_inst(0), is_rvc(false), exception(false),
        exc_cause(0), access_fault(false), misaligned(false), enqueued_cycle(0) {}
};

struct StreamModel {
    uint64_t pc;
    bool carry_valid;
    uint16_t carry;
    uint64_t carry_pc;
    bool odd_fenced;
    std::deque<ExpectedUop> expected;
    StreamModel() : pc(RESET_VECTOR), carry_valid(false), carry(0), carry_pc(0),
        odd_fenced(false), expected() {}
};

struct Stimulus {
    bool reset, redirect, req_full, stalled, has_response;
    uint64_t target;
    ImemResponse response;
    Stimulus() : reset(false), redirect(false), req_full(false), stalled(false),
        has_response(false), target(0), response() {}
};

struct PendingResponse {
    ImemResponse r;
    unsigned due;
    uint64_t generation;
    bool cross_upper;
};

struct Errors {
    uint64_t instruction, pc, duplicate, drop, stale_side_effect, carry;
    uint64_t post_reset_stale, request, state;
    uint64_t total() const { return instruction + pc + duplicate + drop + stale_side_effect +
        carry + post_reset_stale + request + state; }
    Errors() : instruction(0), pc(0), duplicate(0), drop(0), stale_side_effect(0),
        carry(0), post_reset_stale(0), request(0), state(0) {}
};

static void report(Errors& e, uint64_t& member, const char* kind, unsigned seed,
                   unsigned cycle, uint64_t expected, uint64_t actual) {
    ++member;
    if (e.total() <= 32)
        std::fprintf(stderr, "RVC_FETCH_%s seed=%u cycle=%u expected=%llx actual=%llx\n",
            kind, seed, cycle, static_cast<unsigned long long>(expected),
            static_cast<unsigned long long>(actual));
}

static ExpectedUop fault_uop(uint64_t pc, uint32_t original, uint64_t cause,
                             bool access, unsigned cycle) {
    ExpectedUop e;
    e.pc = pc;
    e.debug_inst = original;
    e.exception = true;
    e.exc_cause = cause;
    e.access_fault = access;
    e.misaligned = !access && cause == 0;
    e.enqueued_cycle = cycle;
    return e;
}

static ExpectedUop base_uop(uint64_t pc, uint32_t instruction, unsigned cycle) {
    ExpectedUop e;
    e.pc = pc;
    e.inst = instruction;
    e.enqueued_cycle = cycle;
    return e;
}

static ExpectedUop compressed_uop(uint64_t pc, uint16_t parcel, unsigned cycle) {
    ExpectedUop e;
    e.pc = pc;
    e.debug_inst = parcel;
    e.is_rvc = true;
    e.enqueued_cycle = cycle;
    uint32_t expanded = 0;
    if (expected_rvc(parcel, expanded)) {
        e.inst = expanded;
    } else {
        e.exception = true;
        e.exc_cause = 2;
    }
    return e;
}

static bool same_uop(const ExpectedUop& e, const MicroOp& u) {
    return u.debug_pc == e.pc && u.inst == e.inst &&
           u.debug_inst == e.debug_inst && u.is_rvc == e.is_rvc &&
           u.exception == e.exception && u.exc_cause == e.exc_cause &&
           u.exc.exception == e.exception && u.exc.exc_cause == e.exc_cause &&
           u.exc.xcpt_ae_if == e.access_fault && u.exc.xcpt_ma_if == e.misaligned;
}

static uint64_t uop_signature(const MicroOp& u) {
    return (u.debug_pc & 0xffffffffffULL) ^
           (static_cast<uint64_t>(u.debug_inst) << 40) ^
           (static_cast<uint64_t>(u.inst) << 8) ^
           (static_cast<uint64_t>(u.is_rvc) << 4) ^
           (static_cast<uint64_t>(u.exception) << 3) ^ u.exc_cause;
}

static uint64_t expected_signature(const ExpectedUop& e) {
    return (e.pc & 0xffffffffffULL) ^
           (static_cast<uint64_t>(e.debug_inst) << 40) ^
           (static_cast<uint64_t>(e.inst) << 8) ^
           (static_cast<uint64_t>(e.is_rvc) << 4) ^
           (static_cast<uint64_t>(e.exception) << 3) ^ e.exc_cause;
}

static bool base_parcel(uint16_t parcel) {
    return (parcel & 0x3u) == 0x3u && (parcel & 0x1fu) != 0x1fu;
}

static void append_parcel(StreamModel& model, uint64_t pc, uint16_t parcel,
                          unsigned cycle) {
    if ((parcel & 0x3u) != 0x3u) {
        model.expected.push_back(compressed_uop(pc, parcel, cycle));
        model.pc = pc + 2;
        return;
    }
    model.expected.push_back(fault_uop(pc, parcel, 2, false, cycle));
    model.pc = pc + 2;
}

static void accept_response(StreamModel& model, const ImemResponse& response,
                            Coverage& cov, unsigned cycle) {
    ++cov.accepted_responses;
    if (model.odd_fenced) return;

    const uint64_t first_pc = model.carry_valid ? model.carry_pc : model.pc;
    if (response.exception) {
        model.expected.push_back(fault_uop(first_pc, 0, response.exc_cause, true, cycle));
        model.pc = first_pc + 4;
        model.carry_valid = false;
        return;
    }

    if (model.carry_valid) {
        const uint32_t instruction =
            (response.instruction << 16) | static_cast<uint32_t>(model.carry);
        model.expected.push_back(base_uop(model.carry_pc, instruction, cycle));
        const uint64_t upper_pc = model.carry_pc + 4;
        const uint16_t upper = static_cast<uint16_t>(response.instruction >> 16);
        model.carry_valid = false;
        model.pc = upper_pc + 2;
        if (base_parcel(upper)) {
            model.carry_valid = true;
            model.carry = upper;
            model.carry_pc = upper_pc;
            model.pc = upper_pc;
            ++cov.carries;
        } else {
            append_parcel(model, upper_pc, upper, cycle);
        }
        return;
    }

    const bool start_upper = (model.pc & 0x2ULL) != 0;
    const uint16_t first = start_upper ?
        static_cast<uint16_t>(response.instruction >> 16) :
        static_cast<uint16_t>(response.instruction);
    if (base_parcel(first)) {
        if (!start_upper) {
            model.expected.push_back(base_uop(model.pc, response.instruction, cycle));
            model.pc += 4;
        } else {
            model.carry_valid = true;
            model.carry = first;
            model.carry_pc = model.pc;
            ++cov.carries;
        }
        return;
    }

    const uint64_t parcel_pc = model.pc;
    append_parcel(model, parcel_pc, first, cycle);
    if (start_upper || model.expected.back().exception) return;

    const uint64_t upper_pc = parcel_pc + 2;
    const uint16_t upper = static_cast<uint16_t>(response.instruction >> 16);
    if (base_parcel(upper)) {
        model.carry_valid = true;
        model.carry = upper;
        model.carry_pc = upper_pc;
        model.pc = upper_pc;
        ++cov.carries;
    } else {
        append_parcel(model, upper_pc, upper, cycle);
    }
}

static bool request_address(const StreamModel& model, uint64_t& address) {
    if (model.odd_fenced || (model.pc & 0x1ULL) != 0) return false;
    address = model.carry_valid ? ((model.carry_pc + 2) & ~3ULL) : (model.pc & ~3ULL);
    return true;
}

static void reset_model(StreamModel& model) {
    model.pc = RESET_VECTOR;
    model.carry_valid = false;
    model.carry = 0;
    model.carry_pc = 0;
    model.odd_fenced = false;
    model.expected.clear();
}

static void redirect_model(StreamModel& model, uint64_t target, unsigned cycle) {
    model.pc = target;
    model.carry_valid = false;
    model.carry = 0;
    model.carry_pc = 0;
    model.odd_fenced = (target & 1u) != 0;
    model.expected.clear();
    if (model.odd_fenced)
        model.expected.push_back(fault_uop(target, 0, 0, false, cycle));
}

static bool response_matches_frontend(const BoomCoreState& dut, const ImemResponse& r) {
    return dut.frontend.request_sent && r.address == dut.frontend.pending_address &&
           r.fetch_id == dut.frontend.pending_fetch_id &&
           r.epoch == dut.frontend.pending_epoch;
}

static bool coverage_ok(const Coverage& c) {
    for (unsigned i = 0; i < 4; ++i) if (c.words[i] == 0) return false;
    return c.backpressure && c.latency && c.stale_id && c.stale_epoch &&
        c.stale_address && c.redirects && c.redirects_upper && c.redirects_odd &&
        c.resets && c.stalls && c.accepted_responses && c.publications &&
        c.compressed && c.base32 && c.illegal && c.faults && c.access_faults &&
        c.cross_upper_faults && c.cross_upper_successes && c.kills && c.carries &&
        c.post_reset_stale && c.held_publications;
}

}  // namespace

int main() {
    Coverage cov;
    Errors errors;
    uint64_t generation = 0;

    for (unsigned i = 0; i < sizeof(kSupported) / sizeof(kSupported[0]); ++i) {
        uint32_t expected = 0;
        const boom::RvcDecodeResult got = boom::decompress_rvc(kSupported[i]);
        if (!expected_rvc(kSupported[i], expected) || !got.valid || !got.legal ||
            got.instruction != expected)
            report(errors, errors.instruction, "RVC_ORACLE", 0, i, expected, got.instruction);
    }
    for (unsigned i = 0; i < sizeof(kReserved) / sizeof(kReserved[0]); ++i) {
        const boom::RvcDecodeResult got = boom::decompress_rvc(kReserved[i]);
        if (!got.valid || got.legal)
            report(errors, errors.instruction, "RVC_RESERVED", 0, i, 0, got.legal);
    }
    const uint16_t closed_gap_parcels[] = {0x9002u, 0x9001u, 0x9782u};
    for (unsigned i = 0; i < 3; ++i) {
        const boom::RvcDecodeResult got = boom::decompress_rvc(closed_gap_parcels[i]);
        if (!got.valid || !got.legal || got.instruction == 0)
            report(errors, errors.instruction, "RVC_CLOSED_GAP", 0, i, 1, got.legal);
    }

    for (unsigned seed = 0; seed < kSeeds; ++seed) {
        Rng rng(0x72626f6f6dULL ^ (static_cast<uint64_t>(seed) << 32) ^ seed);
        BoomCoreState dut;
        PipeSignals pipe;
        StreamModel model;
        std::deque<PendingResponse> pending;
        ImemResponse reset_stale;
        bool reset_stale_valid = false;
        bool recover_odd_redirect = false;
        bool recover_fault = false;
        bool held_valid = false;
        MicroOp held_uop;

        for (unsigned cycle = 0; cycle < kCycles; ++cycle) {
            Stimulus in;
            dut.brupdate = BranchUpdate();
            dut.frontend_redirect = FrontendRedirect();
            dut.global_flush = false;
            dut.rob.state = ROB_NORMAL;

            in.stalled = (cycle > 1) && (rng.range(100) < 31 || cycle % 127 == 17);
            if (in.stalled) ++cov.stalls;
            dut.decode.dec_valids[0] = in.stalled && (rng.next() & 1u);
            dut.rename.dispatch_packets[0].valid = in.stalled && !dut.decode.dec_valids[0];

            in.req_full = cycle % 257 == 5 || rng.range(2048) == 0;
            if (in.req_full) {
                ++cov.backpressure;
                for (unsigned i = 0; i < 1024; ++i) {
                    ImemRequest dummy;
                    dummy.kill = true;
                    pipe.imem_req.write(dummy);
                }
            }

            in.reset = cycle != 0 && (cycle % 701 == 0 || rng.range(4096) == 0);
            if (in.reset) {
                if (!pending.empty() || !model.expected.empty() || model.carry_valid || held_valid)
                    ++cov.kills;
                ++cov.resets;
                ++generation;
                reset_model(model);
                held_valid = false;
                if (dut.frontend.request_sent) {
                    reset_stale.address = dut.frontend.pending_address;
                    reset_stale.fetch_id = dut.frontend.pending_fetch_id;
                    reset_stale.epoch = dut.frontend.pending_epoch;
                    reset_stale.instruction = imem_word(reset_stale.address, seed, cov);
                    reset_stale_valid = true;
                }
                dut.frontend.reset_done = false;
            }

            const bool force_upper = cycle != 0 && cycle % 311 == 0;
            const bool force_odd = cycle != 0 && cycle % 509 == 0;
            const bool force_recovery = recover_odd_redirect || recover_fault;
            recover_odd_redirect = false;
            recover_fault = false;
            in.redirect = !in.reset && (force_recovery || force_upper || force_odd ||
                                         rng.range(1024) < 5);
            if (in.redirect) {
                if (!pending.empty() || !model.expected.empty() || model.carry_valid || held_valid)
                    ++cov.kills;
                uint64_t target = RESET_VECTOR + 4 * rng.range(512);
                if (force_upper || (!force_odd && rng.range(4) == 0)) target += 2;
                if (force_odd) target += 1;
                in.target = target;
                ++generation;
                redirect_model(model, target, cycle);
                held_valid = false;
                ++cov.redirects;
                if ((target & 3u) == 2) ++cov.redirects_upper;
                if (target & 1u) ++cov.redirects_odd;
                if (target & 1u) recover_odd_redirect = true;
                dut.brupdate.valid = true;
                dut.brupdate.mispredict = true;
                dut.brupdate.jalr_target = target;
            }

            bool stale_input = false;
            bool post_reset_input = false;
            bool delivered_real = false;
            uint64_t delivered_generation = generation;
            bool delivered_cross = false;

            std::deque<PendingResponse>::iterator live = pending.end();
            for (std::deque<PendingResponse>::iterator it = pending.begin();
                 it != pending.end(); ++it) {
                if (it->generation == generation) {
                    live = it;
                    break;
                }
            }

            if (reset_stale_valid && cycle % 701 == 1) {
                in.has_response = true;
                in.response = reset_stale;
                reset_stale_valid = false;
                stale_input = post_reset_input = true;
                ++cov.post_reset_stale;
            } else if (live != pending.end() && cycle % 97 == 11) {
                in.has_response = true;
                in.response = live->r;
                in.response.fetch_id = live->r.fetch_id + 1;
                stale_input = true;
                ++cov.stale_id;
            } else if (live != pending.end() && cycle % 101 == 13) {
                in.has_response = true;
                in.response = live->r;
                in.response.epoch = live->r.epoch + 1;
                stale_input = true;
                ++cov.stale_epoch;
            } else if (live != pending.end() && cycle % 103 == 19) {
                in.has_response = true;
                in.response = live->r;
                in.response.address = live->r.address + 4;
                stale_input = true;
                ++cov.stale_address;
            } else {
                for (std::deque<PendingResponse>::iterator it = pending.begin();
                     it != pending.end(); ++it) {
                    if (it->due <= cycle) {
                        in.has_response = true;
                        in.response = it->r;
                        delivered_real = true;
                        delivered_generation = it->generation;
                        delivered_cross = it->cross_upper;
                        post_reset_input = it->generation != generation;
                        stale_input = post_reset_input;
                        if (post_reset_input) ++cov.post_reset_stale;
                        pending.erase(it);
                        break;
                    }
                }
            }

            const bool redirecting = in.reset || in.redirect;
            const bool matches_frontend = in.has_response && response_matches_frontend(dut, in.response);
            if (in.has_response && !redirecting && delivered_real &&
                delivered_generation == generation && matches_frontend) {
                accept_response(model, in.response, cov, cycle);
            } else if (in.has_response && !redirecting && delivered_real &&
                       delivered_generation == generation && !matches_frontend) {
                report(errors, errors.state, "RESPONSE_NOT_ACCEPTED", seed, cycle,
                       in.response.address, dut.frontend.pending_address);
            } else if (in.has_response && !redirecting && !delivered_real && matches_frontend) {
                report(errors, errors.stale_side_effect, "STALE_ACCEPTED", seed, cycle,
                       0, in.response.address);
            }
            if (in.has_response) pipe.imem_resp.write(in.response);

            boom::frontend_module(dut, pipe);

            ImemRequest actual_request;
            bool actual_request_valid = false;
            if (in.req_full) {
                for (unsigned i = 0; i < 1024; ++i) {
                    const ImemRequest dummy = pipe.imem_req.read();
                    if (!dummy.kill)
                        report(errors, errors.request, "REQUEST_WHILE_FULL", seed, cycle, 0, 1);
                }
            } else if (!pipe.imem_req.empty()) {
                actual_request = pipe.imem_req.read();
                actual_request_valid = true;
            }
            if (!pipe.imem_req.empty())
                report(errors, errors.duplicate, "DUPLICATE_REQUEST", seed, cycle, 0, 1);

            if (actual_request_valid) {
                uint64_t expected_address = 0;
                if (!request_address(model, expected_address)) {
                    report(errors, errors.request, "REQUEST_WHILE_FENCED", seed, cycle,
                           0, actual_request.address);
                } else if (actual_request.address != expected_address) {
                    report(errors, errors.request, "REQUEST_MISMATCH", seed, cycle,
                           expected_address, actual_request.address);
                }
                if ((actual_request.address & 3u) != 0)
                    report(errors, errors.request, "UNALIGNED_REQUEST", seed, cycle,
                           0, actual_request.address);

                PendingResponse pr;
                pr.r.address = actual_request.address;
                pr.r.fetch_id = actual_request.fetch_id;
                pr.r.epoch = actual_request.epoch;
                pr.r.instruction = imem_word(actual_request.address, seed, cov);
                pr.cross_upper = model.carry_valid &&
                    actual_request.address == ((model.carry_pc + 2) & ~3ULL);
                pr.r.exception = pr.cross_upper ? rng.range(4) == 0 : rng.range(32) == 0;
                pr.r.exc_cause = 1 + rng.range(4);
                if (pr.cross_upper && pr.r.exception) ++cov.cross_upper_faults;
                if (pr.cross_upper && !pr.r.exception) ++cov.cross_upper_successes;
                const unsigned delay = rng.range(17);
                pr.due = cycle + delay + 1;
                pr.generation = generation;
                if (delay) ++cov.latency;
                pending.push_back(pr);
            }

            const bool output_valid = dut.frontend.fetch_packet_valid;
            if (output_valid && in.stalled) {
                ++cov.held_publications;
                if (!model.expected.empty() && !same_uop(model.expected.front(), dut.frontend.fetch_uop)) {
                    report(errors, errors.instruction, "HELD_INSTRUCTION_MISMATCH", seed, cycle,
                           expected_signature(model.expected.front()), uop_signature(dut.frontend.fetch_uop));
                }
                if (held_valid && uop_signature(held_uop) != uop_signature(dut.frontend.fetch_uop)) {
                    report(errors, errors.carry, "HELD_PACKET_CHANGED", seed, cycle,
                           uop_signature(held_uop), uop_signature(dut.frontend.fetch_uop));
                }
                held_valid = true;
                held_uop = dut.frontend.fetch_uop;
            } else if (output_valid) {
                if (model.expected.empty()) {
                    report(errors, errors.duplicate, "UNEXPECTED_PUBLICATION", seed, cycle,
                           0, uop_signature(dut.frontend.fetch_uop));
                } else {
                    const ExpectedUop expected = model.expected.front();
                    if (dut.frontend.fetch_uop.debug_pc != expected.pc)
                        report(errors, errors.pc, "PC_MISMATCH", seed, cycle,
                               expected.pc, dut.frontend.fetch_uop.debug_pc);
                    if (!same_uop(expected, dut.frontend.fetch_uop))
                        report(errors, errors.instruction, "INSTRUCTION_MISMATCH", seed, cycle,
                               expected_signature(expected), uop_signature(dut.frontend.fetch_uop));
                    model.expected.pop_front();
                    ++cov.publications;
                    if (dut.frontend.fetch_uop.exception) {
                        ++cov.faults;
                        if (dut.frontend.fetch_uop.exc.xcpt_ae_if) ++cov.access_faults;
                        if (dut.frontend.fetch_uop.exc_cause == 2) ++cov.illegal;
                        recover_fault = true;
                    } else if (dut.frontend.fetch_uop.is_rvc) {
                        ++cov.compressed;
                    } else {
                        ++cov.base32;
                    }
                }
                held_valid = false;
            } else if (!in.stalled) {
                held_valid = false;
            }

            if (!model.expected.empty() &&
                cycle > model.expected.front().enqueued_cycle + 128 && !in.stalled) {
                report(errors, errors.drop, "PUBLICATION_TIMEOUT", seed, cycle,
                       model.expected.front().pc, dut.frontend.pc);
                model.expected.pop_front();
            }
            if (model.expected.size() > FETCH_BUFFER_DEPTH + 8)
                report(errors, errors.drop, "PUBLICATION_BACKLOG", seed, cycle,
                       FETCH_BUFFER_DEPTH + 8, model.expected.size());

            (void)stale_input;
            (void)post_reset_input;
            (void)delivered_cross;
        }
    }

    std::printf("RVC_FETCH_RANDOM seeds=%u cycles_per_seed=%u total_cycles=%u errors=%llu\n",
        kSeeds, kCycles, kSeeds * kCycles,
        static_cast<unsigned long long>(errors.total()));
    std::printf("RVC_FETCH_CLASSES supported_words=%llu base_words=%llu reserved_words=%llu protected_words=%llu compressed=%llu base32=%llu illegal=%llu\n",
        static_cast<unsigned long long>(cov.words[0]), static_cast<unsigned long long>(cov.words[1]),
        static_cast<unsigned long long>(cov.words[2]), static_cast<unsigned long long>(cov.words[3]),
        static_cast<unsigned long long>(cov.compressed), static_cast<unsigned long long>(cov.base32),
        static_cast<unsigned long long>(cov.illegal));
    std::printf("RVC_FETCH_PROTOCOL requests_backpressured=%llu delayed=%llu accepted_responses=%llu publications=%llu carries=%llu kills=%llu held=%llu\n",
        static_cast<unsigned long long>(cov.backpressure), static_cast<unsigned long long>(cov.latency),
        static_cast<unsigned long long>(cov.accepted_responses), static_cast<unsigned long long>(cov.publications),
        static_cast<unsigned long long>(cov.carries), static_cast<unsigned long long>(cov.kills),
        static_cast<unsigned long long>(cov.held_publications));
    std::printf("RVC_FETCH_ADVERSARIAL stale_id=%llu stale_epoch=%llu stale_address=%llu post_reset_stale=%llu redirects=%llu upper_redirects=%llu odd_redirects=%llu resets=%llu stalls=%llu faults=%llu access_faults=%llu cross_upper_faults=%llu cross_upper_successes=%llu\n",
        static_cast<unsigned long long>(cov.stale_id), static_cast<unsigned long long>(cov.stale_epoch),
        static_cast<unsigned long long>(cov.stale_address), static_cast<unsigned long long>(cov.post_reset_stale),
        static_cast<unsigned long long>(cov.redirects), static_cast<unsigned long long>(cov.redirects_upper),
        static_cast<unsigned long long>(cov.redirects_odd), static_cast<unsigned long long>(cov.resets),
        static_cast<unsigned long long>(cov.stalls), static_cast<unsigned long long>(cov.faults),
        static_cast<unsigned long long>(cov.access_faults),
        static_cast<unsigned long long>(cov.cross_upper_faults),
        static_cast<unsigned long long>(cov.cross_upper_successes));
    std::printf("RVC_FETCH_ERRORS instruction=%llu pc=%llu duplicate=%llu drop=%llu stale_side_effect=%llu carry=%llu post_reset_stale=%llu request=%llu state=%llu\n",
        static_cast<unsigned long long>(errors.instruction), static_cast<unsigned long long>(errors.pc),
        static_cast<unsigned long long>(errors.duplicate), static_cast<unsigned long long>(errors.drop),
        static_cast<unsigned long long>(errors.stale_side_effect), static_cast<unsigned long long>(errors.carry),
        static_cast<unsigned long long>(errors.post_reset_stale), static_cast<unsigned long long>(errors.request),
        static_cast<unsigned long long>(errors.state));

    if (errors.total() || !coverage_ok(cov)) {
        if (!coverage_ok(cov)) std::fprintf(stderr, "RVC_FETCH_COVERAGE_MISSING\n");
        return 1;
    }
    std::printf("RVC_FETCH_RANDOM_256X2048_PASS\n");
    return 0;
}
