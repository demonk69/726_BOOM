#include "boom_config.hpp"
#include "boom_types.hpp"
#include "boom_state.hpp"
#include "boom_interfaces.hpp"
#include "completion.hpp"
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
#ifdef BOOM_HLS_W3_DIAGNOSTIC
#pragma HLS INLINE
#endif
    completion_service_execute(state);
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
