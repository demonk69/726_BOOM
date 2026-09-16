#include "../../src/divider.cpp"
#include "../../src/predictor.cpp"
#include "../../src/lsu.cpp"

extern "C" void g6_l1r_pilot_older_store_block(
        uint32_t seed, uint64_t& obs0, uint64_t& obs1, uint64_t& obs2,
        uint64_t& obs3, uint64_t& obs4, uint64_t& obs5) {
#pragma HLS INTERFACE ap_ctrl_hs port=return
#pragma HLS INTERFACE ap_none port=seed
#pragma HLS INTERFACE ap_none port=obs0
#pragma HLS INTERFACE ap_none port=obs1
#pragma HLS INTERFACE ap_none port=obs2
#pragma HLS INTERFACE ap_none port=obs3
#pragma HLS INTERFACE ap_none port=obs4
#pragma HLS INTERFACE ap_none port=obs5
    BoomCoreState state;
    state.rob.head = 0;
    state.rob.tail = 2;
    RobEntry& store = state.rob.entries[0];
    store.valid = true;
    store.uop.ctrl.is_sta = true;
    store.uop.queue.rob_idx = 0;
    store.uop.queue.rob_allocation_id = 0x56000000u | seed;
    RobEntry& load = state.rob.entries[1];
    load.valid = true;
    load.uop.ctrl.is_load = true;
    load.uop.queue.rob_idx = 1;
    load.uop.queue.rob_allocation_id = 0x56010000u | seed;

    const bool blocked = boom::older_store_in_rob(state, 1);
    store.valid = false;
    const bool unblocked_after_store_removal = boom::older_store_in_rob(state, 1);
    store.valid = true;
    const bool head_is_not_blocked = boom::older_store_in_rob(state, 0);

    obs0 = blocked;
    obs1 = unblocked_after_store_removal;
    obs2 = head_is_not_blocked;
    obs3 = store.valid;
    obs4 = state.rob.head;
    obs5 = store.uop.queue.rob_allocation_id;
}
