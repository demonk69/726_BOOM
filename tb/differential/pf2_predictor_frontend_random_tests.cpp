#include "frontend.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>

namespace {

struct Rng {
    uint64_t value;
    explicit Rng(uint64_t seed) : value(seed | 1ull) {}
    uint32_t next() {
        value ^= value << 13;
        value ^= value >> 7;
        value ^= value << 17;
        return static_cast<uint32_t>(value);
    }
};

struct Errors {
    uint64_t predecode_error;
    uint64_t predictor_request_error;
    uint64_t predictor_response_error;
    uint64_t latency_error;
    uint64_t stale_response_error;
    uint64_t packet_drop;
    uint64_t packet_duplicate;
    uint64_t packet_mask_error;
    uint64_t jal_target_error;
    uint64_t conditional_shadow_error;
    uint64_t jalr_prediction_error;
    uint64_t frontend_pc_error;
    uint64_t fetch_buffer_ordering_error;
    uint64_t reset_error;
    uint64_t redirect_priority_error;
    Errors() : predecode_error(0), predictor_request_error(0),
        predictor_response_error(0), latency_error(0), stale_response_error(0),
        packet_drop(0), packet_duplicate(0), packet_mask_error(0),
        jal_target_error(0), conditional_shadow_error(0),
        jalr_prediction_error(0), frontend_pc_error(0),
        fetch_buffer_ordering_error(0), reset_error(0),
        redirect_priority_error(0) {}
    uint64_t total() const {
        return predecode_error + predictor_request_error + predictor_response_error +
            latency_error + stale_response_error + packet_drop + packet_duplicate +
            packet_mask_error + jal_target_error + conditional_shadow_error +
            jalr_prediction_error + frontend_pc_error + fetch_buffer_ordering_error +
            reset_error + redirect_priority_error;
    }
};

enum Kind {
    NO_CFI, COND_L0, COND_L1, JAL_L0, JAL_L1,
    JALR_L0, JALR_L1, BASE_COND, BASE_JAL, FAULT_PACKET
};

uint32_t encode_branch(int32_t immediate) {
    const uint32_t x = static_cast<uint32_t>(immediate) & 0x1fffu;
    return (((x >> 12) & 1u) << 31) | (((x >> 5) & 0x3fu) << 25) |
           (1u << 20) | (1u << 15) | (((x >> 1) & 0xfu) << 8) |
           (((x >> 11) & 1u) << 7) | 0x63u;
}

uint32_t encode_jal(int32_t immediate) {
    const uint32_t x = static_cast<uint32_t>(immediate) & 0x1fffffu;
    return (((x >> 20) & 1u) << 31) | (((x >> 1) & 0x3ffu) << 21) |
           (((x >> 11) & 1u) << 20) | (((x >> 12) & 0xffu) << 12) |
           (1u << 7) | 0x6fu;
}

void initialize(BoomCoreState& state, uint32_t seed) {
    state.frontend.reset_done = true;
    state.frontend.epoch = seed + 1;
    state.rob.state = ROB_NORMAL;
    state.predictor_generation = seed + 3;
}

void drain_requests(PipeSignals& pipe) {
    while (!pipe.imem_req.empty()) pipe.imem_req.read();
}

void frontend_step(BoomCoreState& state, PipeSignals& pipe, uint32_t& cycles) {
    boom::frontend_module(state, pipe);
    drain_requests(pipe);
    ++cycles;
}

void inject(BoomCoreState& state, PipeSignals& pipe, uint64_t pc,
            uint32_t word, bool fault, uint32_t& cycles) {
    state.frontend.pc = pc;
    state.frontend.request_sent = true;
    state.frontend.pending_fetch_id = 41;
    state.frontend.pending_epoch = state.frontend.epoch;
    state.frontend.pending_address = pc & ~3ull;
    ImemResponse response;
    response.address = state.frontend.pending_address;
    response.fetch_id = state.frontend.pending_fetch_id;
    response.epoch = state.frontend.pending_epoch;
    response.instruction = word;
    response.exception = fault;
    response.exc_cause = fault ? 12 : 0;
    pipe.imem_resp.write(response);
    frontend_step(state, pipe, cycles);
}

void train(BoomCoreState& state, uint64_t pc, unsigned desired) {
    boom::PredictorStepInput input;
    input.active_generation = state.predictor_generation;
    input.update.valid = true;
    input.update.commit_qualified = true;
    input.update.cfi_type = boom::CFI_CONDITIONAL_BRANCH;
    input.update.pc = pc;
    input.update.metadata_token = static_cast<uint16_t>((pc >> 1) & 255u);
    input.update.generation = state.predictor_generation;
    input.update.taken = desired >= 2;
    if (desired == 0 || desired == 2 || desired == 3) state.predictor.step(input);
    if (desired == 3) state.predictor.step(input);
}

void run_scenario(uint32_t seed, uint32_t scenario, Rng& rng,
                  uint32_t& cycles, Errors& errors) {
    BoomCoreState state;
    PipeSignals pipe;
    initialize(state, seed);
    const Kind kind = static_cast<Kind>(rng.next() % 10u);
    const uint64_t pc = 0x80000000ull +
        (static_cast<uint64_t>(seed) << 16) + scenario * 0x20ull;
    const unsigned counter = rng.next() & 3u;
    uint32_t word = 0x00010001u;
    uint8_t expected_type = boom::CFI_NONE;
    uint8_t expected_lane = 0;
    uint8_t expected_mask = 3;
    uint64_t expected_pc = pc + 4;
    bool conditional = false;
    bool jalr = false;
    bool fault = false;

    switch (kind) {
    case NO_CFI: break;
    case COND_L0:
        word = 0x0001c001u; expected_type = boom::CFI_CONDITIONAL_BRANCH;
        conditional = true; break;
    case COND_L1:
        word = 0xc0010001u; expected_type = boom::CFI_CONDITIONAL_BRANCH;
        expected_lane = 1; conditional = true; break;
    case JAL_L0:
        word = 0x0001a001u; expected_type = boom::CFI_JAL;
        expected_mask = 1; expected_pc = pc; break;
    case JAL_L1:
        word = 0xa0010001u; expected_type = boom::CFI_JAL;
        expected_lane = 1; expected_pc = pc + 2; break;
    case JALR_L0:
        word = 0x00018082u; expected_type = boom::CFI_JALR;
        jalr = true; break;
    case JALR_L1:
        word = 0x80820001u; expected_type = boom::CFI_JALR;
        expected_lane = 1; jalr = true; break;
    case BASE_COND:
        word = encode_branch(static_cast<int32_t>((rng.next() & 0x3fu) * 2) - 64);
        expected_type = boom::CFI_CONDITIONAL_BRANCH; expected_mask = 1;
        conditional = true; break;
    case BASE_JAL: {
        const int32_t offset = (rng.next() & 1u) ? 128 : -128;
        word = encode_jal(offset); expected_type = boom::CFI_JAL;
        expected_mask = 1; expected_pc = pc + static_cast<uint64_t>(offset); break;
    }
    case FAULT_PACKET:
        word = encode_jal(64); expected_mask = 1; fault = true; break;
    }

    if (conditional) train(state, pc + (expected_lane ? 2 : 0), counter);

    const bool inject_stale = (rng.next() & 7u) == 0;
    if (inject_stale && cycles < 8192) {
        state.frontend.pc = pc;
        state.frontend.request_sent = true;
        state.frontend.pending_fetch_id = 41;
        state.frontend.pending_epoch = state.frontend.epoch;
        state.frontend.pending_address = pc & ~3ull;
        ImemResponse stale;
        stale.address = state.frontend.pending_address;
        stale.fetch_id = 42;
        stale.epoch = state.frontend.pending_epoch;
        stale.instruction = 0xffffffffu;
        pipe.imem_resp.write(stale);
        frontend_step(state, pipe, cycles);
        if (state.frontend.response_received || !state.frontend.request_sent)
            ++errors.stale_response_error;
    }
    if (cycles >= 8192) return;
    inject(state, pipe, pc, word, fault, cycles);

    if (fault) {
        if (state.frontend.pending_predecode.packet_has_cfi)
            ++errors.predecode_error;
        if (!state.frontend.pending_packet.valid ||
            !state.frontend.pending_packet.slots[0].exception)
            ++errors.packet_drop;
        if (state.frontend.pc != pc + 4) ++errors.frontend_pc_error;
    } else if (expected_type == boom::CFI_NONE) {
        if (state.frontend.pending_predecode.packet_has_cfi)
            ++errors.predecode_error;
        if (state.frontend.prediction_pending ||
            state.frontend.predictor_request_accepted)
            ++errors.predictor_request_error;
        if (!state.frontend.request_sent) ++errors.latency_error;
    } else {
        if (!state.frontend.pending_predecode.packet_has_cfi ||
            state.frontend.pending_predecode.selected_cfi_lane != expected_lane ||
            state.frontend.pending_predecode.selected_cfi_result.cfi_type != expected_type)
            ++errors.predecode_error;
    }

    if (state.frontend.pending_packet.valid_mask != expected_mask)
        ++errors.packet_mask_error;
    if (state.frontend.pc != expected_pc) {
        if (expected_type == boom::CFI_JAL) ++errors.jal_target_error;
        else ++errors.frontend_pc_error;
    }

    if (conditional) {
        if (!state.frontend.prediction_pending ||
            !state.frontend.predictor_request_accepted ||
            state.frontend.predictor_response_valid)
            ++errors.predictor_request_error;
        if (cycles >= 8192) return;
        const unsigned redirect_kind = rng.next() % 12u;
        if (redirect_kind < 3) {
            const uint64_t target = pc + 0x1000 + redirect_kind * 4;
            if (redirect_kind == 0) {
                state.frontend_redirect.valid = true;
                state.frontend_redirect.cause = FRONTEND_REDIRECT_DEBUG;
                state.frontend_redirect.target_pc = target;
            } else if (redirect_kind == 1) {
                state.exception_commit.valid = true;
                state.exception_commit.target = target;
                state.frontend_redirect.valid = true;
                state.frontend_redirect.cause = FRONTEND_REDIRECT_EXCEPTION;
                state.frontend_redirect.target_pc = target;
            } else {
                state.brupdate.valid = true;
                state.brupdate.mispredict = true;
                state.brupdate.jalr_target = target;
            }
            frontend_step(state, pipe, cycles);
            if (state.frontend.pc != target || state.frontend.prediction_pending ||
                state.frontend.fetch_buffer.count != 0)
                ++errors.redirect_priority_error;
            if (!state.frontend.predictor_response_stale)
                ++errors.stale_response_error;
            return;
        }
        if (redirect_kind == 3) {
            state.frontend.reset_done = false;
            frontend_step(state, pipe, cycles);
            if (state.frontend.pc != RESET_VECTOR || state.frontend.prediction_pending ||
                state.frontend.pending_packet.valid)
                ++errors.reset_error;
            if (!state.frontend.predictor_response_stale)
                ++errors.stale_response_error;
            return;
        }

        const bool blocked = (rng.next() & 3u) == 0;
        if (blocked) {
            state.frontend.fetch_buffer.count = FETCH_BUFFER_DEPTH;
            state.rename.dispatch_packets[0].valid = true;
        }
        frontend_step(state, pipe, cycles);
        if (!state.frontend.predictor_response_valid ||
            !state.frontend.predictor_prediction_valid ||
            state.frontend.predictor_predicted_taken != (counter >= 2))
            ++errors.predictor_response_error;
        if (state.frontend.pc != pc + 4 ||
            state.frontend.pending_packet.valid_mask != (blocked ? expected_mask : 0))
            ++errors.conditional_shadow_error;
        if (blocked) {
            if (!state.frontend.prediction_pending ||
                state.frontend.fetch_buffer.count != FETCH_BUFFER_DEPTH)
                ++errors.packet_drop;
            if (cycles >= 8192) return;
            const uint64_t token = state.frontend.prediction_token;
            frontend_step(state, pipe, cycles);
            if (!state.frontend.predictor_response_valid ||
                state.frontend.prediction_token != token ||
                state.frontend.predictor_request_accepted)
                ++errors.packet_duplicate;
            return;
        }
    } else {
        if (jalr && (state.frontend.prediction_pending ||
                     state.frontend.predictor_request_accepted ||
                     state.frontend.predictor_prediction_valid))
            ++errors.jalr_prediction_error;
        if (cycles >= 8192) return;
        frontend_step(state, pipe, cycles);
        const uint8_t expected_count = expected_mask == 3 ? 2 : 1;
        if (state.frontend.fetch_buffer.count != expected_count)
            ++errors.packet_drop;
        if (state.frontend.fetch_buffer.count != 0 &&
            state.frontend.fetch_buffer.entries[state.frontend.fetch_buffer.head].pc != pc)
            ++errors.fetch_buffer_ordering_error;
    }
}

}  // namespace

int main() {
    Errors errors;
    for (uint32_t seed = 0; seed < 256; ++seed) {
        Rng rng(0x9e3779b97f4a7c15ull ^ seed);
        uint32_t cycles = 0;
        uint32_t scenario = 0;
        while (cycles < 8192) run_scenario(seed, scenario++, rng, cycles, errors);
        if (cycles != 8192) ++errors.latency_error;
    }
    const bool pass = errors.total() == 0;
    std::printf("PF2_RANDOM_%s seeds=256 cycles_per_seed=8192 "
                "predecode_error=%llu predictor_request_error=%llu "
                "predictor_response_error=%llu latency_error=%llu "
                "stale_response_error=%llu packet_drop=%llu packet_duplicate=%llu "
                "packet_mask_error=%llu jal_target_error=%llu "
                "conditional_shadow_error=%llu jalr_prediction_error=%llu "
                "frontend_pc_error=%llu fetch_buffer_ordering_error=%llu "
                "reset_error=%llu redirect_priority_error=%llu errors=%llu\n",
                pass ? "PASS" : "FAIL",
                static_cast<unsigned long long>(errors.predecode_error),
                static_cast<unsigned long long>(errors.predictor_request_error),
                static_cast<unsigned long long>(errors.predictor_response_error),
                static_cast<unsigned long long>(errors.latency_error),
                static_cast<unsigned long long>(errors.stale_response_error),
                static_cast<unsigned long long>(errors.packet_drop),
                static_cast<unsigned long long>(errors.packet_duplicate),
                static_cast<unsigned long long>(errors.packet_mask_error),
                static_cast<unsigned long long>(errors.jal_target_error),
                static_cast<unsigned long long>(errors.conditional_shadow_error),
                static_cast<unsigned long long>(errors.jalr_prediction_error),
                static_cast<unsigned long long>(errors.frontend_pc_error),
                static_cast<unsigned long long>(errors.fetch_buffer_ordering_error),
                static_cast<unsigned long long>(errors.reset_error),
                static_cast<unsigned long long>(errors.redirect_priority_error),
                static_cast<unsigned long long>(errors.total()));
    return pass ? EXIT_SUCCESS : EXIT_FAILURE;
}
