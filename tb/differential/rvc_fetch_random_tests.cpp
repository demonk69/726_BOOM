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
    uint64_t post_reset_stale;
    Coverage() : backpressure(0), latency(0), stale_id(0), stale_epoch(0),
        stale_address(0), redirects(0), redirects_upper(0), redirects_odd(0),
        resets(0), stalls(0), accepted_responses(0), publications(0),
        compressed(0), base32(0), illegal(0), faults(0), access_faults(0),
        cross_upper_faults(0), cross_upper_successes(0), kills(0), carries(0),
        post_reset_stale(0) {
        for (unsigned i = 0; i < 4; ++i) words[i] = 0;
    }
};

static const uint16_t kSupported[] = {
    0x0001, 0x0085, 0x4105, 0x6141, 0x8082, 0x852e, 0x9782, 0xc006,
    0x4000, 0x6000, 0xc000, 0xe000, 0xa001, 0xc001, 0xe001, 0x6105
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

struct Ref {
    uint64_t pc;
    bool reset_done, request_sent;
    uint32_t fetch_id, pending_fetch_id, epoch, pending_epoch;
    uint64_t pending_address;
    bool response_received;
    uint64_t resp_address;
    uint32_t resp_instruction;
    bool resp_exception;
    uint64_t resp_exc_cause;
    bool halfword_valid;
    uint16_t halfword;
    uint64_t halfword_pc;
    uint32_t halfword_epoch;
    bool fetch_packet_valid;
    MicroOp fetch_uop;

    Ref() : pc(RESET_VECTOR), reset_done(false), request_sent(false), fetch_id(0),
        pending_fetch_id(0), epoch(0xffffffffu), pending_epoch(0), pending_address(0),
        response_received(false), resp_address(0), resp_instruction(0),
        resp_exception(false), resp_exc_cause(0), halfword_valid(false), halfword(0),
        halfword_pc(0), halfword_epoch(0), fetch_packet_valid(false), fetch_uop() {}
};

struct Stimulus {
    bool reset, redirect, req_full, stalled, has_response;
    uint64_t target;
    ImemResponse response;
    Stimulus() : reset(false), redirect(false), req_full(false), stalled(false),
        has_response(false), target(0), response() {}
};

struct StepResult {
    bool request_valid, publication, accepted_response, redirect, reset;
    ImemRequest request;
    StepResult() : request_valid(false), publication(false), accepted_response(false),
        redirect(false), reset(false), request() {}
};

static MicroOp fault_uop(uint64_t pc, uint64_t cause, bool access) {
    MicroOp u;
    u.debug_pc = pc;
    u.exception = true;
    u.exc_cause = cause;
    u.exc.exception = true;
    u.exc.exc_cause = cause;
    u.exc.xcpt_ae_if = access;
    u.exc.xcpt_ma_if = !access && cause == 0;
    return u;
}

static StepResult reference_step(Ref& f, const Stimulus& in, Coverage& cov) {
    StepResult out;
    const bool reset_redirect = !f.reset_done || in.reset;
    if (in.reset) f.reset_done = false;
    if (reset_redirect) {
        f.pc = RESET_VECTOR;
        f.fetch_id = 0;
        ++f.epoch;
        f.reset_done = true;
        f.request_sent = false;
        f.response_received = false;
        f.halfword_valid = false;
        f.fetch_packet_valid = false;
        out.reset = true;
    }
    const bool target_redirect = !reset_redirect && in.redirect;
    const bool redirect = reset_redirect || target_redirect;
    out.redirect = redirect;
    if (redirect) {
        if (f.request_sent || f.response_received || f.halfword_valid || f.fetch_packet_valid)
            ++cov.kills;
        if (!reset_redirect) ++f.epoch;
        f.pc = target_redirect ? in.target : RESET_VECTOR;
        f.request_sent = false;
        f.response_received = false;
        f.halfword_valid = false;
        f.fetch_packet_valid = false;
        if (target_redirect && (f.pc & 1u)) {
            f.fetch_uop = fault_uop(f.pc, 0, false);
            f.fetch_packet_valid = true;
            out.publication = true;
        }
    }

    if (in.has_response && !redirect && f.request_sent &&
        in.response.fetch_id == f.pending_fetch_id &&
        in.response.epoch == f.pending_epoch &&
        in.response.address == f.pending_address) {
        f.resp_address = in.response.address;
        f.resp_instruction = in.response.instruction;
        f.resp_exception = in.response.exception;
        f.resp_exc_cause = in.response.exc_cause;
        f.response_received = true;
        f.request_sent = false;
        out.accepted_response = true;
        ++cov.accepted_responses;
    }

    if (target_redirect && (f.pc & 1u)) return out;
    if (in.stalled) return out;
    if (f.fetch_packet_valid) f.fetch_packet_valid = false;

    if (f.response_received && f.halfword_valid) {
        if (f.halfword_epoch != f.epoch) {
            f.response_received = false;
            f.halfword_valid = false;
        } else if (f.resp_exception) {
            f.fetch_uop = fault_uop(f.halfword_pc, f.resp_exc_cause, true);
        } else {
            MicroOp u;
            u.debug_pc = f.halfword_pc;
            u.inst = (f.resp_instruction << 16) | f.halfword;
            u.is_rvc = false;
            f.fetch_uop = u;
        }
        if (f.halfword_valid) {
            f.fetch_packet_valid = true;
            out.publication = true;
            f.pc = f.halfword_pc + 4;
            f.halfword_valid = false;
            f.response_received = false;
        }
    } else if (f.response_received) {
        const uint64_t parcel_pc = f.pc;
        const bool upper = (parcel_pc & 2u) != 0;
        const uint16_t parcel = upper ? static_cast<uint16_t>(f.resp_instruction >> 16) :
                                        static_cast<uint16_t>(f.resp_instruction);
        if (f.resp_exception) {
            f.fetch_uop = fault_uop(parcel_pc, f.resp_exc_cause, true);
            f.fetch_packet_valid = true;
            out.publication = true;
            f.pc = parcel_pc + 4;
            f.response_received = false;
        } else if ((parcel & 3u) != 3u) {
            uint32_t expanded = 0;
            const bool legal = expected_rvc(parcel, expanded);
            MicroOp u;
            u.debug_pc = parcel_pc;
            u.debug_inst = parcel;
            u.is_rvc = true;
            if (!legal) {
                u.exception = true;
                u.exc_cause = 2;
                u.exc.exception = true;
                u.exc.exc_cause = 2;
            } else {
                u.inst = expanded;
            }
            f.fetch_uop = u;
            f.fetch_packet_valid = true;
            out.publication = true;
            f.pc = parcel_pc + 2;
            if (upper) f.response_received = false;
        } else if ((parcel & 0x1fu) == 0x1fu) {
            f.fetch_uop = fault_uop(parcel_pc, 2, false);
            f.fetch_uop.debug_inst = parcel;
            f.fetch_packet_valid = true;
            out.publication = true;
            f.pc = parcel_pc + 2;
            f.response_received = false;
        } else if (!upper) {
            MicroOp u;
            u.debug_pc = parcel_pc;
            u.inst = f.resp_instruction;
            u.is_rvc = false;
            f.fetch_uop = u;
            f.fetch_packet_valid = true;
            out.publication = true;
            f.pc = parcel_pc + 4;
            f.response_received = false;
        } else {
            f.halfword_valid = true;
            f.halfword = parcel;
            f.halfword_pc = parcel_pc;
            f.halfword_epoch = f.epoch;
            f.response_received = false;
            ++cov.carries;
        }
    }

    if (!f.request_sent && !f.response_received && !in.req_full) {
        out.request_valid = true;
        out.request.address = f.halfword_valid ? ((f.halfword_pc + 2) & ~3ULL) :
                                                (f.pc & ~3ULL);
        out.request.fetch_id = f.fetch_id;
        out.request.epoch = f.epoch;
        out.request.kill = false;
        f.pending_fetch_id = f.fetch_id;
        f.pending_epoch = f.epoch;
        f.pending_address = out.request.address;
        ++f.fetch_id;
        f.request_sent = true;
    }
    return out;
}

struct PendingResponse {
    ImemResponse r;
    unsigned due;
    uint64_t generation;
};

struct Errors {
    uint64_t instruction, pc, duplicate, drop, stale_side_effect, carry;
    uint64_t post_reset_stale, request, state;
    uint64_t total() const { return instruction + pc + duplicate + drop + stale_side_effect +
        carry + post_reset_stale + request + state; }
    Errors() : instruction(0), pc(0), duplicate(0), drop(0), stale_side_effect(0),
        carry(0), post_reset_stale(0), request(0), state(0) {}
};

static bool same_request(const ImemRequest& a, const ImemRequest& b) {
    return a.address == b.address && a.fetch_id == b.fetch_id &&
           a.epoch == b.epoch && a.kill == b.kill;
}

static bool same_uop(const MicroOp& a, const MicroOp& b) {
    return a.debug_pc == b.debug_pc && a.inst == b.inst &&
           a.debug_inst == b.debug_inst && a.is_rvc == b.is_rvc &&
           a.exception == b.exception && a.exc_cause == b.exc_cause &&
           a.exc.exception == b.exc.exception && a.exc.exc_cause == b.exc.exc_cause &&
           a.exc.xcpt_ae_if == b.exc.xcpt_ae_if && a.exc.xcpt_ma_if == b.exc.xcpt_ma_if;
}

static void report(Errors& e, uint64_t& member, const char* kind, unsigned seed,
                   unsigned cycle, uint64_t expected, uint64_t actual) {
    ++member;
    if (e.total() <= 32)
        std::fprintf(stderr, "RVC_FETCH_%s seed=%u cycle=%u expected=%llx actual=%llx\n",
            kind, seed, cycle, static_cast<unsigned long long>(expected),
            static_cast<unsigned long long>(actual));
}

static void compare_state(const Ref& r, const BoomCoreState& s, const StepResult& expected,
                          bool stale_input, bool post_reset_input, unsigned seed,
                          unsigned cycle, Errors& e) {
    const FrontendState& d = s.frontend;
    if (d.pc != r.pc) report(e, e.pc, "PC_MISMATCH", seed, cycle, r.pc, d.pc);
    if (d.fetch_packet_valid != r.fetch_packet_valid) {
        if (d.fetch_packet_valid) report(e, e.duplicate, "DUPLICATE", seed, cycle, 0, 1);
        else report(e, e.drop, "DROP", seed, cycle, 1, 0);
    } else if (r.fetch_packet_valid && !same_uop(r.fetch_uop, d.fetch_uop)) {
        report(e, e.instruction, "INSTRUCTION_MISMATCH", seed, cycle,
               (static_cast<uint64_t>(r.fetch_uop.debug_inst) << 32) | r.fetch_uop.inst,
               (static_cast<uint64_t>(d.fetch_uop.debug_inst) << 32) | d.fetch_uop.inst);
    }
    if (d.halfword_valid != r.halfword_valid || d.halfword != r.halfword ||
        d.halfword_pc != r.halfword_pc || d.halfword_epoch != r.halfword_epoch) {
        report(e, e.carry, "CARRY_CORRUPTION", seed, cycle,
               (static_cast<uint64_t>(r.halfword_valid) << 63) | r.halfword,
               (static_cast<uint64_t>(d.halfword_valid) << 63) | d.halfword);
    }
    const bool core_match = d.reset_done == r.reset_done &&
        d.request_sent == r.request_sent && d.fetch_id == r.fetch_id &&
        d.pending_fetch_id == r.pending_fetch_id && d.epoch == r.epoch &&
        d.pending_epoch == r.pending_epoch && d.pending_address == r.pending_address &&
        d.response_received == r.response_received;
    if (!core_match) {
        uint64_t& bucket = post_reset_input ? e.post_reset_stale :
                           (stale_input ? e.stale_side_effect : e.state);
        report(e, bucket, post_reset_input ? "POST_RESET_STALE" :
               (stale_input ? "STALE_SIDE_EFFECT" : "STATE_MISMATCH"), seed, cycle,
               (static_cast<uint64_t>(r.epoch) << 32) | r.fetch_id,
               (static_cast<uint64_t>(d.epoch) << 32) | d.fetch_id);
    }
    (void)expected;
}

static bool coverage_ok(const Coverage& c) {
    for (unsigned i = 0; i < 4; ++i) if (c.words[i] == 0) return false;
    return c.backpressure && c.latency && c.stale_id && c.stale_epoch &&
        c.stale_address && c.redirects && c.redirects_upper && c.redirects_odd &&
        c.resets && c.stalls && c.accepted_responses && c.publications &&
        c.compressed && c.base32 && c.illegal && c.faults && c.access_faults &&
        c.cross_upper_faults && c.cross_upper_successes && c.kills && c.carries &&
        c.post_reset_stale;
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
        Ref ref;
        std::deque<PendingResponse> pending;
        ImemResponse reset_stale;
        bool reset_stale_valid = false;
        bool recover_odd_redirect = false;
        bool recover_fault = false;

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
                ++cov.resets;
                ++generation;
                dut.frontend.reset_done = false;
                if (dut.frontend.request_sent) {
                    reset_stale.address = dut.frontend.pending_address;
                    reset_stale.fetch_id = dut.frontend.pending_fetch_id;
                    reset_stale.epoch = dut.frontend.pending_epoch;
                    reset_stale.instruction = imem_word(reset_stale.address, seed, cov);
                    reset_stale_valid = true;
                }
            }

            const bool force_upper = cycle != 0 && cycle % 311 == 0;
            const bool force_odd = cycle != 0 && cycle % 509 == 0;
            const bool force_recovery = recover_odd_redirect || recover_fault;
            recover_odd_redirect = false;
            recover_fault = false;
            in.redirect = !in.reset && (force_recovery || force_upper || force_odd ||
                                         rng.range(1024) < 5);
            if (in.redirect) {
                uint64_t target = RESET_VECTOR + 4 * rng.range(512);
                if (force_upper || (!force_odd && rng.range(4) == 0)) target += 2;
                if (force_odd) target += 1;
                while ((target & 2u) && imem_word_class(target, seed) == WORD_BASE32)
                    target += 4;
                in.target = target;
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
            if (reset_stale_valid && cycle % 701 == 1) {
                in.has_response = true;
                in.response = reset_stale;
                reset_stale_valid = false;
                stale_input = post_reset_input = true;
                ++cov.post_reset_stale;
            } else if (ref.request_sent && cycle % 97 == 11) {
                in.has_response = true;
                in.response.address = ref.pending_address;
                in.response.fetch_id = ref.pending_fetch_id + 1;
                in.response.epoch = ref.pending_epoch;
                in.response.instruction = imem_word(in.response.address, seed, cov);
                stale_input = true;
                ++cov.stale_id;
            } else if (ref.request_sent && cycle % 101 == 13) {
                in.has_response = true;
                in.response.address = ref.pending_address;
                in.response.fetch_id = ref.pending_fetch_id;
                in.response.epoch = ref.pending_epoch + 1;
                in.response.instruction = imem_word(in.response.address, seed, cov);
                stale_input = true;
                ++cov.stale_epoch;
            } else if (ref.request_sent && cycle % 103 == 19) {
                in.has_response = true;
                in.response.address = ref.pending_address + 4;
                in.response.fetch_id = ref.pending_fetch_id;
                in.response.epoch = ref.pending_epoch;
                in.response.instruction = imem_word(in.response.address, seed, cov);
                stale_input = true;
                ++cov.stale_address;
            } else {
                for (std::deque<PendingResponse>::iterator it = pending.begin();
                     it != pending.end(); ++it) {
                    if (it->due <= cycle) {
                        in.has_response = true;
                        in.response = it->r;
                        post_reset_input = it->generation != generation;
                        stale_input = post_reset_input || it->r.epoch != ref.epoch;
                        if (post_reset_input) ++cov.post_reset_stale;
                        pending.erase(it);
                        break;
                    }
                }
            }
            if (in.has_response) pipe.imem_resp.write(in.response);

            const StepResult expected = reference_step(ref, in, cov);
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
            if (actual_request_valid != expected.request_valid ||
                (actual_request_valid && !same_request(actual_request, expected.request))) {
                report(errors, errors.request, "REQUEST_MISMATCH", seed, cycle,
                       expected.request_valid ? expected.request.address : 0,
                       actual_request_valid ? actual_request.address : 0);
            }
            if (!pipe.imem_req.empty())
                report(errors, errors.duplicate, "DUPLICATE_REQUEST", seed, cycle, 0, 1);

            compare_state(ref, dut, expected, stale_input, post_reset_input,
                          seed, cycle, errors);

            if (expected.publication) {
                ++cov.publications;
                if (ref.fetch_uop.exception) {
                    ++cov.faults;
                    ++cov.illegal;
                    if (ref.fetch_uop.exc.xcpt_ae_if) ++cov.access_faults;
                    recover_fault = true;
                } else if (ref.fetch_uop.is_rvc) {
                    ++cov.compressed;
                } else {
                    ++cov.base32;
                }
            }

            if (actual_request_valid) {
                PendingResponse pr;
                pr.r.address = actual_request.address;
                pr.r.fetch_id = actual_request.fetch_id;
                pr.r.epoch = actual_request.epoch;
                pr.r.instruction = imem_word(actual_request.address, seed, cov);
                const bool cross = ref.halfword_valid;
                pr.r.exception = cross ? rng.range(4) == 0 : rng.range(32) == 0;
                pr.r.exc_cause = 1 + rng.range(4);
                if (cross && pr.r.exception) ++cov.cross_upper_faults;
                if (cross && !pr.r.exception) ++cov.cross_upper_successes;
                const unsigned delay = rng.range(17);
                pr.due = cycle + delay + 1;
                pr.generation = generation;
                if (delay) ++cov.latency;
                pending.push_back(pr);
            }
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
    std::printf("RVC_FETCH_PROTOCOL requests_backpressured=%llu delayed=%llu accepted_responses=%llu publications=%llu carries=%llu kills=%llu\n",
        static_cast<unsigned long long>(cov.backpressure), static_cast<unsigned long long>(cov.latency),
        static_cast<unsigned long long>(cov.accepted_responses), static_cast<unsigned long long>(cov.publications),
        static_cast<unsigned long long>(cov.carries), static_cast<unsigned long long>(cov.kills));
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
