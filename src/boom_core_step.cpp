#include "boom_config.hpp"
#include "boom_types.hpp"
#include "boom_state.hpp"
#include "boom_interfaces.hpp"

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
void rob_commit_module(BoomCoreState& state, PipeSignals& pipe);

}

void boom_core_step(BoomCoreState& state, PipeSignals& pipe) {
    state.global_flush = false;
    state.io_success = state.io_trap = false;
    state.brupdate.valid = state.brupdate.mispredict = false;

    boom::csr_module(state);
    boom::branch_module(state);
    boom::lsu_module(state, pipe);
    boom::frontend_module(state, pipe);
    boom::decode_module(state);
    boom::rename_module(state);
    boom::rob_allocate(state);
    boom::issue_module(state);
    boom::execute_module(state);
    boom::rob_commit_module(state, pipe);
}
