#include "boom_interfaces.hpp"
#include "boom_state.hpp"
#include "frontend.hpp"

#include <cstdint>
#include <cstdio>

namespace boom {
void branch_complete_event(BoomCoreState&, const RobCompleteEvent&);
void rob_commit_module(BoomCoreState&, PipeSignals&);
}

namespace {

struct Checker {
    uint64_t checks;
    uint64_t failures;
    Checker() : checks(0), failures(0) {}
    void expect(bool value, const char* name) {
        ++checks;
        if (!value) {
            ++failures;
            if (failures <= 20) std::printf("PF5_FAIL,%s\n", name);
        }
    }
};

struct Fixture {
    BoomCoreState state;
    PipeSignals pipe;
    uint32_t allocation_id;
    Fixture() : state(), pipe(), allocation_id(1) {
        state.rob.state = ROB_NORMAL;
        state.product_ftq_enabled = true;
        state.predictor_generation = 7;
        state.frontend.reset_done = true;
    }
};

bool predict(BoomCoreState& state, uint64_t pc) {
    boom::PredictorStepInput input;
    input.active_generation = state.predictor_generation;
    input.req_valid = true;
    input.request.pc = pc;
    input.request.cfi_type = boom::CFI_CONDITIONAL_BRANCH;
    input.request.static_target_valid = true;
    input.request.static_target = pc + 16;
    input.request.generation = state.predictor_generation;
    input.request.request_token = pc;
    state.predictor.step(input);
    boom::PredictorStepInput consume;
    consume.active_generation = state.predictor_generation;
    consume.resp_ready = true;
    const boom::PredictorStepOutput output = state.predictor.step(consume);
    return output.resp_valid && output.response.taken;
}

void seed(BoomCoreState& state, uint64_t pc, bool taken, unsigned count) {
    for (unsigned i = 0; i < count; ++i) {
        boom::PredictorStepInput input;
        input.active_generation = state.predictor_generation;
        input.update.valid = true;
        input.update.commit_qualified = true;
        input.update.cfi_type = boom::CFI_CONDITIONAL_BRANCH;
        input.update.pc = pc;
        input.update.metadata_token = static_cast<uint16_t>((pc >> 1) & 255u);
        input.update.taken = taken;
        input.update.generation = state.predictor_generation;
        state.predictor.step(input);
    }
}

MicroOp install(Fixture& f, uint64_t pc, uint8_t cfi_type,
                 bool stale_generation, bool wrong_metadata,
                 bool predicted_taken = false, bool target_valid = false,
                 uint64_t predicted_target = 0) {
    boom::FtqStepInput ftq_input;
    ftq_input.alloc_valid = true;
    ftq_input.allocation.packet_base_pc = pc;
    ftq_input.allocation.packet_valid_mask = 1;
    ftq_input.allocation.prediction_valid = true;
    ftq_input.allocation.predicted_taken = predicted_taken;
    ftq_input.allocation.target_valid = target_valid;
    ftq_input.allocation.predicted_target = predicted_target;
    ftq_input.allocation.cfi_lane = 0;
    ftq_input.allocation.cfi_type = cfi_type;
    ftq_input.allocation.predictor_metadata_index = static_cast<uint8_t>(
        ((pc >> 1) & 255u) ^ (wrong_metadata ? 1u : 0u));
    ftq_input.allocation.predictor_generation = f.state.predictor_generation;
    const boom::FtqStepOutput allocated = f.state.ftq.step(ftq_input);

    MicroOp uop;
    uop.debug_pc = pc;
    uop.inst = 0x00000063u;
    uop.uopc = cfi_type == boom::CFI_CONDITIONAL_BRANCH ? 31 :
        (cfi_type == boom::CFI_JAL ? 29 : 30);
    uop.branch.is_br = cfi_type == boom::CFI_CONDITIONAL_BRANCH;
    uop.branch.is_jal = cfi_type == boom::CFI_JAL;
    uop.branch.is_jalr = cfi_type == boom::CFI_JALR;
    uop.ftq_valid = true;
    uop.ftq_idx = allocated.alloc_ftq_idx;
    uop.ftq_lane = 0;
    uop.ftq_generation = allocated.alloc_generation +
        (stale_generation ? 1u : 0u);
    uop.queue.rob_idx = f.state.rob.head;
    uop.queue.rob_allocation_id = f.allocation_id++;
    return uop;
}

bool commit(Fixture& f, const MicroOp& uop, bool actual_taken,
            bool resolved, bool exception) {
    RobEntry& entry = f.state.rob.entries[f.state.rob.head];
    entry = RobEntry();
    entry.valid = true;
    entry.busy = false;
    entry.exception = exception;
    entry.uop = uop;
    entry.branch_resolved = resolved;
    entry.branch_actual_taken = actual_taken;
    f.state.rob.tail = static_cast<uint8_t>((f.state.rob.head + 1) % ROB_DEPTH);
    f.state.rob.maybe_full = false;
    boom::rob_commit_module(f.state, f.pipe);
    return f.state.predictor_update_pending.valid;
}

void consume(Fixture& f) {
    boom::frontend_product_module(f.state, f.pipe);
    while (!f.pipe.commit_trace.empty()) (void)f.pipe.commit_trace.read();
    while (!f.pipe.imem_req.empty()) (void)f.pipe.imem_req.read();
}

void eligibility_tests(Checker& check) {
    const uint64_t pc = 0x180;
    {
        Fixture f;
        MicroOp uop = install(f, pc, boom::CFI_CONDITIONAL_BRANCH, false, false);
        check.expect(commit(f, uop, true, true, false), "eligible_commit_update");
        check.expect(f.state.predictor_update_pending.taken, "actual_taken_forwarded");
        check.expect(f.state.ftq_retire_pending.valid, "retire_published");
        consume(f);
        check.expect(!f.state.predictor_update_pending.valid, "update_consumed");
        check.expect(f.state.ftq_last_output.retire_accepted, "retire_after_update");
        check.expect(f.state.bim_training.attempts == 1, "attempt_counted");
        check.expect(f.state.bim_training.accepted == 1, "accepted_counted");
        check.expect(f.state.bim_training.dropped == 0, "no_training_drop");
        check.expect(f.state.bim_training.duplicate == 0, "no_duplicate_training");
    }
    const uint8_t types[] = {boom::CFI_JAL, boom::CFI_JALR};
    for (unsigned i = 0; i < 2; ++i) {
        Fixture f;
        MicroOp uop = install(f, pc + i * 4, types[i], false, false);
        check.expect(!commit(f, uop, true, true, false), "jal_jalr_filtered");
    }
    for (unsigned mode = 0; mode < 3; ++mode) {
        Fixture f;
        MicroOp uop = install(f, pc + 16 + mode * 4,
            boom::CFI_CONDITIONAL_BRANCH, mode == 0, mode == 1);
        check.expect(!commit(f, uop, true, mode != 2, false),
                      "invalid_identity_or_unresolved_filtered");
        check.expect(mode == 2 ? f.state.bim_training.dropped == 1 :
                     f.state.bim_training.stale_rejected == 1,
                     "rejection_accounted");
    }
}

void resolution_training_tests(Checker& check) {
    struct Scenario {
        bool predicted_taken;
        bool actual_taken;
        bool target_match;
    } scenarios[] = {
        {false, false, true}, {true, true, true},
        {false, true, true}, {true, false, true},
        {true, true, false}
    };
    for (unsigned i = 0; i < sizeof(scenarios) / sizeof(scenarios[0]); ++i) {
        Fixture f;
        const uint64_t pc = 0x400 + i * 4;
        const uint64_t predicted_target = pc + 16;
        MicroOp uop = install(f, pc, boom::CFI_CONDITIONAL_BRANCH, false,
                              false, scenarios[i].predicted_taken, true,
                              predicted_target);
        RobEntry& entry = f.state.rob.entries[0];
        entry.valid = true;
        entry.busy = true;
        entry.uop = uop;
        f.state.rob.tail = 1;
        RobCompleteEvent event;
        event.valid = true;
        event.kind = COMPLETION_BRANCH;
        event.uop = uop;
        event.actual_valid = true;
        event.actual_taken = scenarios[i].actual_taken;
        event.actual_target = scenarios[i].target_match ? predicted_target : pc + 24;
        event.fallthrough_pc = pc + 4;
        boom::branch_complete_event(f.state, event);
        entry.busy = false;
        boom::rob_commit_module(f.state, f.pipe);
        check.expect(f.state.rob.commit_valid, "resolved_branch_committed");
        check.expect(f.state.predictor_update_pending.valid,
                     "resolved_branch_trains");
        check.expect(f.state.predictor_update_pending.taken ==
                     scenarios[i].actual_taken, "actual_direction_trains");
        if (i == 4)
            check.expect(f.state.brupdate.target_mispredict &&
                         f.state.predictor_update_pending.taken,
                         "target_mismatch_trains_taken");
        consume(f);
    }

    const uint8_t branch_types[] = {BR_EQ, BR_NE, BR_LT, BR_GE, BR_LTU, BR_GEU,
                                    BR_EQ, BR_NE};
    for (unsigned i = 0; i < 8; ++i) {
        Fixture f;
        const uint64_t pc = 0x600 + i * 2;
        MicroOp uop = install(f, pc, boom::CFI_CONDITIONAL_BRANCH, false, false);
        uop.ctrl.br_type = branch_types[i];
        uop.is_rvc = i >= 6;
        check.expect(commit(f, uop, (i & 1u) != 0, true, false),
                     "conditional_opcode_trains");
        check.expect(f.state.predictor_update_pending.metadata_token ==
                     ((pc >> 1) & 255u), "rvc_and_rv64_index_exact");
        consume(f);
    }
}

void outcome_capture_test(Checker& check) {
    Fixture f;
    MicroOp uop = install(f, 0x220, boom::CFI_CONDITIONAL_BRANCH, false, false);
    RobEntry& entry = f.state.rob.entries[uop.queue.rob_idx];
    entry.valid = true;
    entry.busy = true;
    entry.uop = uop;
    RobCompleteEvent event;
    event.valid = true;
    event.actual_valid = true;
    event.actual_taken = true;
    event.actual_target = uop.debug_pc + 8;
    event.fallthrough_pc = uop.debug_pc + 4;
    event.uop = uop;
    boom::branch_complete_event(f.state, event);
    check.expect(entry.branch_resolved, "completion_records_resolved");
    check.expect(entry.branch_actual_taken, "completion_records_direction");
}

void learning_and_throughput(Checker& check) {
    Fixture f;
    const uint64_t pc = 0x300;
    check.expect(!predict(f.state, pc), "lazy_valid_initial_wn");
    for (unsigned i = 0; i < 2; ++i) {
        MicroOp uop = install(f, pc, boom::CFI_CONDITIONAL_BRANCH, false, false);
        check.expect(commit(f, uop, true, true, false), "taken_training_emitted");
        consume(f);
    }
    check.expect(predict(f.state, pc), "two_taken_updates_predict_taken");
    for (unsigned i = 0; i < 2; ++i) {
        MicroOp uop = install(f, pc, boom::CFI_CONDITIONAL_BRANCH, false, false);
        check.expect(commit(f, uop, false, true, false), "nt_training_emitted");
        consume(f);
    }
    check.expect(!predict(f.state, pc), "two_nt_updates_predict_not_taken");

    uint64_t updates = 0;
    for (unsigned i = 0; i < 64; ++i) {
        const uint64_t branch_pc = 0x800 + i * 2;
        MicroOp uop = install(f, branch_pc, boom::CFI_CONDITIONAL_BRANCH,
                              false, false);
        if (commit(f, uop, (i & 1u) != 0, true, false)) ++updates;
        consume(f);
    }
    check.expect(updates == 64, "continuous_64_commits_no_drop");
}

void alias_and_stress(Checker& check) {
    Fixture f;
    const uint64_t pc_a = 0x100;
    const uint64_t pc_b = pc_a + 512;
    seed(f.state, pc_a, true, 1);
    MicroOp uop = install(f, pc_a, boom::CFI_CONDITIONAL_BRANCH, false, false);
    check.expect(commit(f, uop, true, true, false), "alias_update_emitted");
    consume(f);
    check.expect(predict(f.state, pc_b), "alias_observes_shared_counter");

    for (unsigned i = 0; i < 2048; ++i) {
        const uint64_t pc = 0x1000 + ((i * 2u) & 0x1feu);
        MicroOp branch = install(f, pc, boom::CFI_CONDITIONAL_BRANCH,
                                 false, false);
        const bool taken = (i & 3u) != 0;
        check.expect(commit(f, branch, taken, true, false), "stress_update");
        check.expect(f.state.predictor_update_pending.metadata_token ==
                     ((pc >> 1) & 255u), "ftq_commit_index_equal");
        consume(f);
        check.expect(!f.state.predictor_update_pending.valid,
                     "stress_no_duplicate_pending");
    }
}

}  // namespace

int main() {
    Checker check;
    eligibility_tests(check);
    outcome_capture_test(check);
    resolution_training_tests(check);
    learning_and_throughput(check);
    alias_and_stress(check);
    std::printf("PF5_COMMIT_BIM_TRAINING_PASS checks=%llu failures=%llu "
                "continuous_commits=64\n",
                static_cast<unsigned long long>(check.checks),
                static_cast<unsigned long long>(check.failures));
    return check.failures == 0 ? 0 : 1;
}
