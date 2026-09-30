#include "../../src/divider.cpp"
#include "../../src/predictor.cpp"
#include "../../src/completion.cpp"
#include "../../src/lsu.cpp"

static void install_store(BoomCoreState& state, uint64_t rob_meta,
                          uint64_t sq_meta, uint64_t address,
                          uint64_t data, uint8_t mask) {
    bool rob_valid = (rob_meta >> 63) != 0;
    if (!rob_valid) return;
    uint8_t rob_idx = (uint8_t)rob_meta;
    uint8_t sq_idx = (uint8_t)(rob_meta >> 8);
    uint16_t rob_generation = (uint16_t)(rob_meta >> 16);
    uint32_t rob_allocation = (uint32_t)((rob_meta >> 32) & 0x7fffffffu);
    bool resolved = ((sq_meta >> 3) & 1u) != 0;

    RobEntry& entry = state.rob.entries[rob_idx % ROB_DEPTH];
    entry.valid = true;
    entry.busy = !resolved;
    entry.uop.ctrl.is_sta = true;
    entry.uop.mem.uses_stq = true;
    entry.uop.queue.rob_idx = rob_idx % ROB_DEPTH;
    entry.uop.queue.rob_allocation_id = rob_allocation;
    entry.uop.queue.stq_idx = sq_idx % SQ_DEPTH;
    entry.uop.queue.stq_generation = rob_generation;
    entry.memory_valid = resolved;
    entry.is_store = resolved;
    entry.memory_address = address;
    entry.memory_data = data;
    entry.memory_mask = mask;
    if (!resolved) return;

    StoreQueueEntry& store = state.lsu.stq[sq_idx % SQ_DEPTH];
    store.valid = (sq_meta & 1u) != 0;
    store.address_valid = ((sq_meta >> 1) & 1u) != 0;
    store.data_valid = ((sq_meta >> 2) & 1u) != 0;
    store.rob_idx = rob_idx % ROB_DEPTH;
    store.rob_allocation_id = (uint32_t)(sq_meta >> 32);
    store.generation = (uint16_t)(sq_meta >> 16);
    state.lsu.stq_address[sq_idx % SQ_DEPTH] = address;
    state.lsu.stq_data[sq_idx % SQ_DEPTH] = data;
    store.mask = mask;
}

extern "C" void g6_l2_focused_top(
        uint64_t load_meta, uint32_t load_allocation, uint64_t load_address,
        uint64_t store0_rob, uint64_t store0_sq, uint64_t store0_address,
        uint64_t store0_data, uint8_t store0_mask,
        uint64_t store1_rob, uint64_t store1_sq, uint64_t store1_address,
        uint64_t store1_data, uint8_t store1_mask,
        uint64_t store2_rob, uint64_t store2_sq, uint64_t store2_address,
        uint64_t store2_data, uint8_t store2_mask,
        uint64_t& decision, uint64_t& selected_sq, uint64_t& value,
        uint64_t& coverage) {
#pragma HLS INTERFACE ap_ctrl_hs port=return
#pragma HLS INTERFACE ap_none port=decision
#pragma HLS INTERFACE ap_none port=selected_sq
#pragma HLS INTERFACE ap_none port=value
#pragma HLS INTERFACE ap_none port=coverage
    BoomCoreState state;
#if SQ_DEPTH >= 16
#pragma HLS ARRAY_PARTITION variable=state.lsu.stq_address cyclic factor=2 dim=1
#pragma HLS ARRAY_PARTITION variable=state.lsu.stq_data cyclic factor=2 dim=1
#endif
    uint8_t head = (uint8_t)load_meta;
    uint8_t load_rob = (uint8_t)(load_meta >> 8);
    uint8_t size = (uint8_t)((load_meta >> 16) & 3u);
    bool signed_load = ((load_meta >> 18) & 1u) != 0;
    state.rob.head = head % ROB_DEPTH;

    install_store(state, store0_rob, store0_sq, store0_address,
                  store0_data, store0_mask);
    install_store(state, store1_rob, store1_sq, store1_address,
                  store1_data, store1_mask);
    install_store(state, store2_rob, store2_sq, store2_address,
                  store2_data, store2_mask);

    RobEntry& load = state.rob.entries[load_rob % ROB_DEPTH];
    load.valid = true;
    load.busy = true;
    load.is_load = true;
    load.memory_valid = true;
    load.signed_load = signed_load;
    load.memory_address = load_address;
    load.memory_size = size;
    load.uop.ctrl.is_load = true;
    load.uop.queue.rob_idx = load_rob % ROB_DEPTH;
    load.uop.queue.rob_allocation_id = load_allocation;

    boom::LoadIssuePlan plan = boom::plan_load_issue(state, load_rob % ROB_DEPTH);
    decision = plan.action;
    selected_sq = plan.sq_index;
    value = 0;
    coverage = 0;
    if (plan.action == boom::LOAD_ISSUE_FORWARD) {
        const StoreQueueEntry& selected = state.lsu.stq[(int)plan.sq_index];
        coverage = boom::store_load_coverage(
            state.lsu.stq_address[(int)plan.sq_index], selected.mask, load);
        value = boom::extend_load_value(
            boom::extract_forwarded_load_data(
                state.lsu.stq_address[(int)plan.sq_index],
                state.lsu.stq_data[(int)plan.sq_index], load),
            size, signed_load);
    }
}
