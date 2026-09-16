#include "../../src/divider.cpp"
#include "../../src/predictor.cpp"
#include "../../src/lsu.cpp"

extern "C" void g6_l1r80_older_store(
        uint8_t head, uint8_t store_rob, uint8_t load_rob,
        uint32_t allocation_base, uint64_t store_address,
        uint64_t load_address, uint8_t store_mask, uint8_t load_mask,
        uint64_t& obs0, uint64_t& obs1, uint64_t& obs2, uint64_t& obs3,
        uint64_t& obs4, uint64_t& obs5, uint64_t& obs6, uint64_t& obs7) {
#pragma HLS INTERFACE ap_ctrl_hs port=return
#pragma HLS INTERFACE ap_none port=obs0
#pragma HLS INTERFACE ap_none port=obs1
#pragma HLS INTERFACE ap_none port=obs2
#pragma HLS INTERFACE ap_none port=obs3
#pragma HLS INTERFACE ap_none port=obs4
#pragma HLS INTERFACE ap_none port=obs5
#pragma HLS INTERFACE ap_none port=obs6
#pragma HLS INTERFACE ap_none port=obs7
    BoomCoreState state;
    state.rob.head = head % ROB_DEPTH;
    RobEntry& store = state.rob.entries[store_rob % ROB_DEPTH];
    store.valid = true;
    store.uop.ctrl.is_sta = true;
    store.uop.queue.rob_idx = store_rob % ROB_DEPTH;
    store.uop.queue.rob_allocation_id = allocation_base;
    store.memory_address = store_address;
    RobEntry& load = state.rob.entries[load_rob % ROB_DEPTH];
    load.valid = true;
    load.uop.ctrl.is_load = true;
    load.uop.queue.rob_idx = load_rob % ROB_DEPTH;
    load.uop.queue.rob_allocation_id = allocation_base + 1;
    load.memory_address = load_address;
    store.memory_mask = store_mask;
    const bool blocked = boom::older_store_in_rob(state, load_rob % ROB_DEPTH);
    store.valid = false;
    const bool unblocked = boom::older_store_in_rob(state, load_rob % ROB_DEPTH);
    obs0 = blocked;
    obs1 = unblocked;
    obs2 = store_address;
    obs3 = load_address;
    obs4 = store_rob % ROB_DEPTH;
    obs5 = (uint64_t)store_mask | ((uint64_t)load_mask << 8);
    obs6 = store_address == load_address;
    obs7 = allocation_base;
}
