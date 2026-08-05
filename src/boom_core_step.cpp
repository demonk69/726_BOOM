#include "boom_config.hpp"
#include "boom_types.hpp"
#include "boom_state.hpp"
#include "boom_interfaces.hpp"
#include "completion.hpp"

namespace boom {
extern void frontend_module(BoomCoreState& state, PipeSignals& pipe);
extern void decode_module(BoomCoreState& state);
extern void rename_module(BoomCoreState& state);
extern void issue_module(BoomCoreState& state);
extern void execute_module(BoomCoreState& state);
extern void branch_module(BoomCoreState& state);
extern void lsu_module(BoomCoreState& state, PipeSignals& pipe);
extern void csr_module(BoomCoreState& state);
extern void rob_allocate(BoomCoreState& state);
extern void rob_complete(BoomCoreState& state);
void rob_commit_module(BoomCoreState& state, PipeSignals& pipe);

}

void boom_core_step(BoomCoreState& state, PipeSignals& pipe) {
    state.global_flush = false;
    state.io_success = state.io_trap = false;
    state.brupdate.valid = state.brupdate.mispredict = false;

    boom::csr_module(state);
    // A queued load response retains W3 ownership of the serial completion path.
    boom::completion_service_cycle(state, pipe);
    boom::lsu_module(state, pipe);
    boom::rob_commit_module(state, pipe);
    boom::frontend_module(state, pipe);
    boom::decode_module(state);
    boom::rename_module(state);
    boom::rob_allocate(state);
    state.issue.port_ready[MEM_ISSUE_LANE] = !state.execute.alu_results[MEM_ISSUE_LANE].valid;
    state.issue.port_ready[INT_ISSUE_LANE] =
        !state.execute.alu_results[INT_ISSUE_LANE].valid &&
        !state.execute.divider.arithmetic.result_pending;
    state.issue.port_ready[FP_ISSUE_LANE] = false;
    boom::issue_module(state);
    boom::execute_module(state);
}
