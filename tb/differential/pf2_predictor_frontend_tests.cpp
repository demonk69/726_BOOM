#include "frontend.hpp"
#include "reset.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>

namespace {

struct Checker {
    uint64_t checks;
    uint64_t failures;
    Checker() : checks(0), failures(0) {}
    void expect(bool condition, const char* label) {
        ++checks;
        if (!condition) {
            if (failures < 32) std::printf("FAIL,%s\n", label);
            ++failures;
        }
    }
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

uint32_t encode_jalr() {
    return (1u << 15) | (5u << 7) | 0x67u;
}

void drain_requests(PipeSignals& pipe) {
    while (!pipe.imem_req.empty()) pipe.imem_req.read();
}

void initialize(BoomCoreState& state) {
    state.frontend.reset_done = true;
    state.frontend.epoch = 7;
    state.frontend.pc = 0x80000000ull;
    state.rob.state = ROB_NORMAL;
    state.predictor_generation = 3;
}

void inject_word(BoomCoreState& state, PipeSignals& pipe, uint64_t pc,
                 uint32_t instruction, bool exception = false) {
    state.frontend.pc = pc;
    state.frontend.request_sent = true;
    state.frontend.pending_fetch_id = 19;
    state.frontend.pending_epoch = state.frontend.epoch;
    state.frontend.pending_address = pc & ~3ull;
    ImemResponse response;
    response.address = state.frontend.pending_address;
    response.fetch_id = state.frontend.pending_fetch_id;
    response.epoch = state.frontend.pending_epoch;
    response.instruction = instruction;
    response.exception = exception;
    response.exc_cause = exception ? 12 : 0;
    pipe.imem_resp.write(response);
    boom::frontend_module(state, pipe);
    drain_requests(pipe);
}

void train_counter(BoomCoreState& state, uint64_t pc, unsigned desired) {
    boom::PredictorStepInput input;
    input.active_generation = state.predictor_generation;
    input.update.valid = true;
    input.update.commit_qualified = true;
    input.update.cfi_type = boom::CFI_CONDITIONAL_BRANCH;
    input.update.pc = pc;
    input.update.metadata_token = static_cast<uint16_t>((pc >> 1) & 255u);
    input.update.generation = state.predictor_generation;
    if (desired == 0) {
        input.update.taken = false;
        state.predictor.step(input);
    } else if (desired == 2) {
        input.update.taken = true;
        state.predictor.step(input);
    } else if (desired == 3) {
        input.update.taken = true;
        state.predictor.step(input);
        state.predictor.step(input);
    }
}

void conditional_case(unsigned counter, bool lane1, Checker& c) {
    BoomCoreState state;
    PipeSignals pipe;
    initialize(state);
    const uint64_t pc = 0x80001000ull + counter * 0x400ull;
    const uint32_t word = lane1 ? 0xc0010001u : 0x0001c001u;
    const uint64_t branch_pc = pc + (lane1 ? 2 : 0);
    train_counter(state, branch_pc, counter);
    inject_word(state, pipe, pc, word);
    c.expect(state.frontend.prediction_pending, "conditional_pending_N");
    c.expect(state.frontend.predictor_request_sent, "conditional_request_sent_N");
    c.expect(state.frontend.predictor_request_accepted, "conditional_request_accept_N");
    c.expect(!state.frontend.predictor_response_valid, "conditional_no_response_N");
    c.expect(state.frontend.pending_packet.valid_mask == 3, "conditional_original_mask_N");
    c.expect(state.frontend.pc == pc + 4, "conditional_sequential_pc_N");
    boom::frontend_module(state, pipe);
    drain_requests(pipe);
    const bool taken = counter >= 2;
    c.expect(state.frontend.predictor_response_valid, "conditional_response_N1");
    c.expect(state.frontend.predictor_prediction_valid, "conditional_prediction_valid");
    c.expect(state.frontend.predictor_predicted_taken == taken, "conditional_direction");
    c.expect(!state.frontend.predictor_response_stale, "conditional_response_fresh");
    c.expect(state.frontend.pc == pc + 4, "conditional_shadow_pc");
    c.expect(state.frontend.fetch_buffer.count == 2, "conditional_packet_admitted");
    c.expect(state.frontend.fetch_buffer.entries[state.frontend.fetch_buffer.head].pc == pc,
             "conditional_lane0_order");
    const uint8_t second = static_cast<uint8_t>((state.frontend.fetch_buffer.head + 1) &
                                                 (FETCH_BUFFER_DEPTH - 1));
    c.expect(state.frontend.fetch_buffer.entries[second].pc == pc + 2,
             "conditional_lane1_preserved");
    c.expect(!state.frontend.prediction_pending, "conditional_context_released");
}

void test_no_cfi(Checker& c) {
    for (unsigned repetition = 0; repetition < 256; ++repetition) {
        BoomCoreState state;
        PipeSignals pipe;
        initialize(state);
        const uint64_t pc = 0x80100000ull + repetition * 4ull;
        inject_word(state, pipe, pc, 0x00010001u);
        c.expect(state.frontend.pending_packet.valid, "no_cfi_packet_built");
        c.expect(state.frontend.pending_packet.valid_mask == 3, "no_cfi_mask11");
        c.expect(!state.frontend.prediction_pending, "no_cfi_no_wait");
        c.expect(!state.frontend.predictor_request_accepted, "no_cfi_no_request");
        c.expect(state.frontend.pc == pc + 4, "no_cfi_sequential_pc");
        c.expect(state.frontend.request_sent, "no_cfi_next_request_fastpath");
        boom::frontend_module(state, pipe);
        drain_requests(pipe);
        c.expect(state.frontend.fetch_buffer.count == 2, "no_cfi_admission_N1");
        c.expect(!state.frontend.pending_packet.valid, "no_cfi_release");
    }
}

void test_jal(Checker& c) {
    const int32_t offsets[] = {64, -64};
    for (unsigned i = 0; i < 2; ++i) {
        BoomCoreState state;
        PipeSignals pipe;
        initialize(state);
        const uint64_t pc = 0x80200000ull + i * 0x100ull;
        inject_word(state, pipe, pc, encode_jal(offsets[i]));
        c.expect(state.frontend.pc == pc + static_cast<uint64_t>(offsets[i]),
                 "jal32_target");
        c.expect(state.frontend.pending_packet.valid_mask == 1, "jal32_mask01");
        c.expect(!state.frontend.prediction_pending, "jal32_bypass");
    }
    {
        BoomCoreState state;
        PipeSignals pipe;
        initialize(state);
        const uint64_t pc = 0x80201000ull;
        inject_word(state, pipe, pc, 0x0001a001u);
        c.expect(state.frontend.pending_predecode.selected_cfi_lane == 0,
                 "jal_lane0_selected");
        c.expect(state.frontend.original_packet_mask == 3, "jal_lane0_original11");
        c.expect(state.frontend.pending_packet.valid_mask == 1, "jal_lane0_younger_masked");
        c.expect(state.frontend.pc == pc, "rvc_jal_lane0_target");
    }
    {
        BoomCoreState state;
        PipeSignals pipe;
        initialize(state);
        const uint64_t pc = 0x80202000ull;
        inject_word(state, pipe, pc, 0xa0010001u);
        c.expect(state.frontend.pending_predecode.selected_cfi_lane == 1,
                 "jal_lane1_selected");
        c.expect(state.frontend.pending_packet.valid_mask == 3, "jal_lane1_mask11");
        c.expect(state.frontend.pc == pc + 2, "rvc_jal_lane1_target");
    }
}

void test_base_branch(Checker& c) {
    BoomCoreState state;
    PipeSignals pipe;
    initialize(state);
    const uint64_t pc = 0x80280000ull;
    inject_word(state, pipe, pc, encode_branch(-32));
    c.expect(state.frontend.prediction_pending, "base_branch_pending");
    c.expect(state.frontend.pending_predecode.selected_cfi_lane == 0,
             "base_branch_lane0");
    c.expect(state.frontend.pending_predecode.selected_cfi_result.static_target == pc - 32,
             "base_branch_backward_target");
    boom::frontend_module(state, pipe);
    c.expect(state.frontend.pc == pc + 4, "base_branch_shadow_fallthrough");
}

void test_jalr_and_fault(Checker& c) {
    const uint32_t words[] = {encode_jalr(), 0x00018082u, 0x80820001u};
    for (unsigned i = 0; i < 3; ++i) {
        BoomCoreState state;
        PipeSignals pipe;
        initialize(state);
        const uint64_t pc = 0x80300000ull + i * 0x100ull;
        inject_word(state, pipe, pc, words[i]);
        c.expect(state.frontend.pending_predecode.packet_has_cfi, "jalr_detected");
        c.expect(state.frontend.pending_predecode.selected_cfi_result.cfi_type ==
                 boom::CFI_JALR, "jalr_type");
        c.expect(!state.frontend.prediction_pending, "jalr_no_prediction");
        c.expect(!state.frontend.predictor_request_accepted, "jalr_no_request");
        c.expect(state.frontend.pc == pc + 4, "jalr_fallthrough");
    }
    BoomCoreState fault;
    PipeSignals pipe;
    initialize(fault);
    inject_word(fault, pipe, 0x80310000ull, encode_jal(32), true);
    c.expect(fault.frontend.pending_packet.valid, "fault_packet_present");
    c.expect(fault.frontend.pending_packet.slots[0].exception, "fault_owner_preserved");
    c.expect(!fault.frontend.pending_predecode.packet_has_cfi, "fault_not_predecoded");
    c.expect(fault.frontend.pc == 0x80310004ull, "fault_precedes_jal_steering");

    BoomCoreState younger_fault;
    PipeSignals younger_fault_pipe;
    initialize(younger_fault);
    const uint64_t pc = 0x80311000ull;
    inject_word(younger_fault, younger_fault_pipe, pc, 0x0000a001u);
    c.expect(younger_fault.frontend.original_packet_mask == 3,
             "jal_before_fault_original_mask11");
    c.expect(younger_fault.frontend.pending_packet.slots[1].exception,
             "jal_before_fault_younger_fault_present");
    c.expect(younger_fault.frontend.pending_predecode.packet_has_cfi,
             "jal_before_fault_older_cfi_survives");
    c.expect(younger_fault.frontend.pending_predecode.selected_cfi_lane == 0,
             "jal_before_fault_oldest_token_lane0");
    c.expect(younger_fault.frontend.pending_packet.valid_mask == 1,
             "jal_before_fault_younger_fault_masked");
    c.expect(younger_fault.frontend.pc == pc,
             "jal_before_fault_older_jal_steers");
}

void test_cross_word(Checker& c) {
    BoomCoreState state;
    PipeSignals pipe;
    initialize(state);
    inject_word(state, pipe, 0x80400002ull, 0x00930001u);
    c.expect(state.frontend.fetch_buffer.count == 0, "cross_word_partial_not_admitted");
    c.expect(state.frontend.halfword_valid, "cross_word_carry_held");
    inject_word(state, pipe, 0x80400002ull, 0x00010010u);
    c.expect(state.frontend.pending_packet.valid, "cross_word_completion_packet");
    c.expect(state.frontend.pending_packet.valid_mask == 3, "cross_word_mask11");
    c.expect(state.frontend.pending_packet.slots[0].pc == 0x80400002ull,
             "cross_word_complete_pc");
    c.expect(state.frontend.pending_packet.slots[1].pc == 0x80400006ull,
             "cross_word_rvc_pc");
}

void test_backpressure_and_redirects(Checker& c) {
    {
        BoomCoreState state;
        PipeSignals pipe;
        initialize(state);
        for (uint8_t i = 0; i < FETCH_BUFFER_DEPTH; ++i)
            state.frontend.fetch_buffer.entries[i].pc = 0x90000000ull + i * 2;
        state.frontend.fetch_buffer.count = FETCH_BUFFER_DEPTH;
        state.rename.dispatch_packets[0].valid = true;
        inject_word(state, pipe, 0x80500000ull, 0x0001c001u);
        boom::frontend_module(state, pipe);
        c.expect(state.frontend.predictor_response_valid, "blocked_response_visible");
        c.expect(state.frontend.prediction_pending, "blocked_context_held");
        c.expect(state.frontend.pending_packet.valid, "blocked_packet_held");
        c.expect(state.frontend.fetch_buffer.count == FETCH_BUFFER_DEPTH,
                 "blocked_no_enqueue");
        const uint64_t token = state.frontend.prediction_token;
        boom::frontend_module(state, pipe);
        c.expect(state.frontend.predictor_response_valid, "blocked_response_stable");
        c.expect(state.frontend.prediction_token == token, "blocked_token_stable");
        c.expect(!state.frontend.predictor_request_accepted, "blocked_no_duplicate_request");
        state.rename.dispatch_packets[0] = RenameDispatchPacket();
        state.decode.dec_valids[0] = false;
        boom::frontend_module(state, pipe);
        c.expect(state.frontend.fetch_buffer.count == FETCH_BUFFER_DEPTH - 1,
                 "two_lane_packet_waits_for_two_slots");
        state.decode.dec_valids[0] = false;
        boom::frontend_module(state, pipe);
        c.expect(!state.frontend.prediction_pending, "blocked_packet_eventually_admitted");
        c.expect(state.frontend.fetch_buffer.count == FETCH_BUFFER_DEPTH,
                 "blocked_atomic_admission");
    }

    for (unsigned kind = 0; kind < 3; ++kind) {
        BoomCoreState state;
        PipeSignals pipe;
        initialize(state);
        inject_word(state, pipe, 0x80600000ull + kind * 0x100, 0x0001c001u);
        const uint64_t target = 0x80700000ull + kind * 0x100;
        if (kind == 0) {
            state.frontend_redirect.valid = true;
            state.frontend_redirect.cause = FRONTEND_REDIRECT_DEBUG;
            state.frontend_redirect.target_pc = target;
            boom::frontend_module(state, pipe);
            c.expect(state.frontend.pc == target, "debug_redirect_priority");
        } else if (kind == 1) {
            state.exception_commit.valid = true;
            state.exception_commit.target = target;
            state.frontend_redirect.valid = true;
            state.frontend_redirect.cause = FRONTEND_REDIRECT_EXCEPTION;
            state.frontend_redirect.target_pc = target;
            boom::frontend_module(state, pipe);
            c.expect(state.frontend.pc == target, "exception_redirect_priority");
        } else {
            ResetControllerState reset;
            while (!reset.completed) boom_core_reset_step(state, reset);
            c.expect(state.predictor_generation == 4, "runtime_reset_generation");
            c.expect(!state.frontend.prediction_pending, "runtime_reset_pending_clear");
            c.expect(!state.frontend.pending_packet.valid, "runtime_reset_packet_clear");
        }
        if (kind != 2) {
            c.expect(state.frontend.predictor_response_valid, "redirect_stale_response_drained");
            c.expect(state.frontend.predictor_response_stale, "redirect_stale_marked");
            c.expect(!state.frontend.prediction_pending, "redirect_context_clear");
            c.expect(state.frontend.fetch_buffer.count == 0, "redirect_no_enqueue");
        }
    }
}

}  // namespace

int main() {
    Checker checker;
    test_no_cfi(checker);
    for (unsigned counter = 0; counter < 4; ++counter) {
        conditional_case(counter, false, checker);
        conditional_case(counter, true, checker);
    }
    test_jal(checker);
    test_base_branch(checker);
    test_jalr_and_fault(checker);
    test_cross_word(checker);
    test_backpressure_and_redirects(checker);
    const bool pass = checker.checks == 2239 && checker.failures == 0;
    std::printf("PF2_DIRECTED_%s checks=%llu failures=%llu conditional_mode=SHADOW_ONLY\n",
                pass ? "PASS" : "FAIL",
                static_cast<unsigned long long>(checker.checks),
                static_cast<unsigned long long>(checker.failures));
    return pass ? EXIT_SUCCESS : EXIT_FAILURE;
}
