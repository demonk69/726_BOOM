#include "boom_config.hpp"
#include "boom_types.hpp"
#include "boom_state.hpp"
#include "boom_interfaces.hpp"
namespace boom {

void rob_flush(BoomCoreState& state) {
    RobInternalState& rob = state.rob;
ROB_FLUSH_ENTRIES:
    for (int i=0; i<ROB_DEPTH; i++) {
        if (rob.entries[i].valid) { rob.entries[i].valid=false; rob.entries[i].busy=false; }
    }
    rob.head=0; rob.tail=0; rob.maybe_full=false; rob.state=ROB_NORMAL;
}

bool rob_branch_kill(BoomCoreState& state) {
    (void)state;
    return false;
}

void rob_complete(BoomCoreState& state) {
ROB_COMPLETE_LANES:
    for (int i=0; i<EXECUTE_RESULT_LANES; i++) {
        if (!state.execute.alu_results[i].valid) continue;
        const ExecuteState::AluResult& r = state.execute.alu_results[i];
        if (r.memory_valid || r.is_load || r.is_store || r.uop.ctrl.is_load || r.uop.ctrl.is_sta) continue;
        uint8_t ridx = r.uop.queue.rob_idx;
        if (ridx<ROB_DEPTH && state.rob.entries[ridx].valid) {
            state.rob.entries[ridx].busy=false;
            if (r.exception) state.rob.entries[ridx].exception=true;
        }
    }
}

void rob_allocate(BoomCoreState& state) {
    RobInternalState& rob = state.rob;
    if (rob.state == ROB_INIT) rob.state = ROB_NORMAL;
    if (state.global_flush) return;
ROB_ALLOCATE_LANES:
    for (int i=0; i<DISPATCH_WIDTH; i++) {
        if (!state.rename.renamed_valids[i]) continue;
        bool is_full = (rob.head==rob.tail) && rob.maybe_full;
        if (is_full) { state.rename.renamed_valids[i]=false; break; }
        MicroOp uop = state.rename.renamed_uops[i];
        if (uop.uopc == 0) continue;
        RobEntry& entry = rob.entries[rob.tail];
        entry = RobEntry();
        entry.valid=true; entry.busy=true; entry.exception=uop.exception;
        uop.queue.rob_idx = rob.tail; entry.uop = uop;
        state.rename.renamed_uops[i].queue.rob_idx = rob.tail;
        rob.tail=(rob.tail+1)%ROB_DEPTH;
        if (rob.tail==rob.head) rob.maybe_full=true;
    }
}
}
