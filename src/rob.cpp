#include "boom_config.hpp"
#include "boom_types.hpp"
#include "boom_state.hpp"
#include "boom_interfaces.hpp"
namespace boom {

extern void branch_complete(BoomCoreState& state, const ExecuteState::AluResult& result);
extern bool lsu_accept_completion(BoomCoreState& state, const ExecuteState::AluResult& result);

static void complete_selected(BoomCoreState& state, ExecuteState::AluResult& result) {
#pragma HLS INLINE
    uint8_t ridx=result.uop.queue.rob_idx;
    if (result.uop.branch.is_br || result.uop.branch.is_jal || result.uop.branch.is_jalr)
        branch_complete(state, result);
    if (!state.rob.entries[ridx].valid ||
        state.rob.entries[ridx].uop.queue.rob_allocation_id != result.uop.queue.rob_allocation_id) {
        result=ExecuteState::AluResult();
        return;
    }
    if ((result.memory_valid || result.is_load || result.is_store) &&
        !lsu_accept_completion(state, result)) return;

    RobEntry& entry=state.rob.entries[ridx];
    if (!result.is_load) entry.busy=false;
    if (result.exception) { entry.exception=true; entry.uop.exception=true; entry.uop.exc_cause=result.exc_cause; }
    if (!result.is_load && result.uop.rename.dst_rtype==DST_INT && result.uop.rename.pdst!=0) {
        state.int_rf[result.uop.rename.pdst]=result.result;
        state.rename.int_free_list.busy_table[result.uop.rename.pdst]=false;
    }
    result=ExecuteState::AluResult();
}

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
#ifdef BOOM_HLS_W3_DIAGNOSTIC
#pragma HLS INLINE
#endif
    ExecuteState::AluResult& mem_result=state.execute.alu_results[MEM_ISSUE_LANE];
    ExecuteState::AluResult& int_result=state.execute.alu_results[INT_ISSUE_LANE];
    uint8_t mem_idx=mem_result.uop.queue.rob_idx;
    uint8_t int_idx=int_result.uop.queue.rob_idx;
    bool mem_valid=mem_result.valid && mem_idx<ROB_DEPTH && state.rob.entries[mem_idx].valid &&
        state.rob.entries[mem_idx].uop.queue.rob_allocation_id==mem_result.uop.queue.rob_allocation_id;
    bool int_valid=int_result.valid && int_idx<ROB_DEPTH && state.rob.entries[int_idx].valid &&
        state.rob.entries[int_idx].uop.queue.rob_allocation_id==int_result.uop.queue.rob_allocation_id;
    if (mem_result.valid && !mem_valid) mem_result=ExecuteState::AluResult();
    if (int_result.valid && !int_valid) int_result=ExecuteState::AluResult();
    if (!mem_valid && !int_valid) return;

    uint8_t mem_age=(uint8_t)((mem_idx+ROB_DEPTH-state.rob.head)%ROB_DEPTH);
    uint8_t int_age=(uint8_t)((int_idx+ROB_DEPTH-state.rob.head)%ROB_DEPTH);
    uint8_t selected_lane=(mem_valid && (!int_valid || mem_age<=int_age)) ?
        MEM_ISSUE_LANE : INT_ISSUE_LANE;

    if (selected_lane==MEM_ISSUE_LANE) complete_selected(state, mem_result);
    else complete_selected(state, int_result);
}

void rob_allocate(BoomCoreState& state) {
    RobInternalState& rob = state.rob;
    if (rob.state == ROB_INIT) rob.state = ROB_NORMAL;
    if (state.global_flush) return;
ROB_ALLOCATE_LANES:
    for (int i=0; i<DISPATCH_WIDTH; i++) {
        RenameDispatchPacket& packet = state.rename.dispatch_packets[i];
        if (!packet.valid || packet.rob_allocated) continue;
        bool is_full = (rob.head==rob.tail) && rob.maybe_full;
        if (is_full) break;
        MicroOp uop = packet.uop;
        if (uop.uopc == 0) continue;
        uint32_t allocation_id = rob.next_allocation_id++;
        if (allocation_id == 0) allocation_id = rob.next_allocation_id++;
        RobEntry& entry = rob.entries[rob.tail];
        entry = RobEntry();
        entry.valid=true; entry.busy=!uop.exception; entry.exception=uop.exception;
        uop.queue.rob_idx = rob.tail;
        uop.queue.rob_allocation_id = allocation_id;
        entry.uop = uop;
        packet.uop.queue.rob_idx = rob.tail;
        packet.uop.queue.rob_allocation_id = allocation_id;
        packet.rob_allocated = true;
        rob.tail=(rob.tail+1)%ROB_DEPTH;
        if (rob.tail==rob.head) rob.maybe_full=true;
    }
}
}
