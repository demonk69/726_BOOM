#include "boom_state.hpp"
#include "reset.hpp"

#include <cstdint>
#include <cstdio>

namespace {

unsigned checks;
unsigned failures;

void expect(bool condition, const char* name) {
    ++checks;
    if (!condition) {
        ++failures;
        std::printf("PF6_RESET_FAIL,%s\n", name);
    }
}

void run_reset(BoomCoreState& state) {
    ResetControllerState control;
    for (unsigned step = 0; !control.completed && step < 512; ++step)
        boom_core_reset_step(state, control);
    expect(control.completed, "reset_completed");
}

}  // namespace

int main() {
    BoomCoreState state;
    state.predictor_generation = 9;

    boom::PredictorStepInput request;
    request.active_generation = 9;
    request.req_valid = true;
    request.request.pc = 0x180;
    request.request.cfi_type = boom::CFI_CONDITIONAL_BRANCH;
    request.request.generation = 9;
    request.request.request_token = 0x55;
    state.predictor.step(request);
    expect(state.predictor.peek(false).resp_valid, "request_pending_before_reset");
    run_reset(state);
    expect(!state.predictor.peek(false).resp_valid, "request_killed_by_reset");
    expect(state.predictor_generation == 10, "predictor_generation_advanced");

    boom::FtqStepInput allocation;
    allocation.alloc_valid = true;
    allocation.allocation.packet_base_pc = 0x200;
    allocation.allocation.packet_valid_mask = 1;
    const boom::FtqStepOutput allocated = state.ftq.step(allocation);
    expect(allocated.alloc_accepted, "ftq_occupied_before_reset");
    run_reset(state);
    expect(!state.ftq.lookup_prediction(allocated.alloc_ftq_idx,
                                       allocated.alloc_generation, 0,
                                       boom::CFI_NONE).reference_valid,
           "old_ftq_reference_stale");

    state.rob.entries[0].valid = true;
    state.rob.entries[0].busy = false;
    state.rob.entries[0].branch_resolved = true;
    state.rob.entries[0].uop.branch.is_br = true;
    state.rob.tail = 1;
    run_reset(state);
    expect(!state.rob.entries[0].valid, "resolved_branch_killed_before_commit");
    expect(!state.predictor_update_pending.valid, "resolved_branch_no_post_reset_training");

    state.predictor_update_pending.valid = true;
    state.predictor_update_pending.commit_qualified = true;
    state.predictor_update_pending.cfi_type = boom::CFI_CONDITIONAL_BRANCH;
    state.predictor_update_pending.pc = 0x240;
    state.predictor_update_pending.metadata_token = 0x20;
    state.predictor_update_pending.taken = true;
    state.predictor_update_pending.generation = state.predictor_generation;
    run_reset(state);
    expect(!state.predictor_update_pending.valid, "pending_training_killed_by_reset");
    bool valid = true;
    expect(state.predictor.debug_counter(0x240, valid) == 1 && !valid,
           "pending_training_did_not_update_bim");

    std::printf("PF6_RESET_STRESS_PASS checks=%u failures=%u scenarios=4\n",
                checks, failures);
    return failures == 0 ? 0 : 1;
}
