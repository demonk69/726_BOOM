#include "boom_config.hpp"
#include "boom_types.hpp"
#include "boom_state.hpp"
#include "boom_interfaces.hpp"
#include "completion.hpp"

namespace boom {

static void enqueue_load(BoomCoreState& state, const MicroOp& uop,
                         bool signed_load, uint64_t address,
                         uint8_t mask, uint8_t size, RobEntry& entry);

static bool older_store_in_rob(const BoomCoreState& state, uint8_t rob_idx) {
    const RobInternalState& rob = state.rob;
    uint8_t idx = rob.head;
OLDER_STORE_SCAN:
    for (int i = 0; i < ROB_DEPTH; i++) {
        if (idx == rob_idx) return false;
        const RobEntry& entry = rob.entries[idx];
        if (entry.valid && entry.uop.ctrl.is_sta) return true;
        idx = (idx + 1) % ROB_DEPTH;
    }
    return false;
}

static void clear_lsu_queues(LsuState& lsu) {
    lsu.ldq_head = lsu.ldq_tail = lsu.ldq_count = 0;
    lsu.stq_head = lsu.stq_tail = lsu.stq_count = 0;
    lsu.load_response_pending = false;
    lsu.pending_load_transaction_id = 0;
    lsu.pending_load_rob_idx = 0;
    lsu.pending_load_allocation_id = 0;
CLEAR_LDQ:
    for (int i = 0; i < LDQ_DEPTH; i++) lsu.ldq[i] = LoadQueueEntry();
CLEAR_STQ:
    for (int i = 0; i < STQ_DEPTH; i++) lsu.stq[i] = StoreQueueEntry();
}

static bool try_issue_load(BoomCoreState& state, PipeSignals& pipe, uint8_t rob_idx) {
    RobEntry& entry = state.rob.entries[rob_idx];
    if (!entry.valid || !entry.is_load || !entry.memory_valid || entry.memory_request_sent) return false;
    if (state.lsu.load_response_pending || older_store_in_rob(state, rob_idx) || pipe.dmem_req.full()) return false;

    uint32_t tx = state.lsu.next_transaction_id++;
    DmemRequest req;
    req.transaction_id = tx;
    req.rob_idx = rob_idx;
    req.command = DMEM_LOAD;
    req.is_store = false;
    req.address = entry.memory_address;
    req.size = entry.memory_size;
    req.mask = entry.memory_mask;
    req.signed_load = entry.signed_load;
    req.branch_mask = entry.uop.branch.br_mask;
    pipe.dmem_req.write(req);

    entry.memory_request_sent = true;
    entry.memory_transaction_id = tx;
    state.lsu.load_response_pending = true;
    state.lsu.pending_load_transaction_id = tx;
    state.lsu.pending_load_rob_idx = rob_idx;
    state.lsu.pending_load_allocation_id = entry.uop.queue.rob_allocation_id;
    return true;
}

static void enqueue_store(BoomCoreState& state, const MicroOp& uop,
                          uint64_t address, uint64_t data,
                          uint8_t mask, uint8_t size, RobEntry& entry) {
    entry.memory_valid = true;
    entry.is_store = true;
    entry.is_load = false;
    entry.memory_completed = true;
    entry.memory_address = address;
    entry.memory_data = data;
    entry.memory_mask = mask;
    entry.memory_size = size;

    LsuState& lsu = state.lsu;
    if (lsu.stq_count < STQ_DEPTH) {
        StoreQueueEntry& stq = lsu.stq[lsu.stq_tail];
        stq.valid = true;
        stq.rob_idx = uop.queue.rob_idx;
        stq.rob_allocation_id = uop.queue.rob_allocation_id;
        stq.address_valid = true;
        stq.address = address;
        stq.data_valid = true;
        stq.data = data;
        stq.mask = mask;
        stq.size = size;
        stq.branch_mask = uop.branch.br_mask;
        lsu.stq_tail = (lsu.stq_tail + 1) % STQ_DEPTH;
        lsu.stq_count++;
    }
}

bool lsu_accept_completion(BoomCoreState& state, const MicroOp& uop,
                           bool is_load, bool is_store, bool signed_load,
                           uint64_t memory_address, uint64_t store_data,
                           uint8_t memory_mask, uint8_t memory_size) {
    uint8_t rob_idx=uop.queue.rob_idx;
    if (rob_idx>=ROB_DEPTH || !state.rob.entries[rob_idx].valid ||
        state.rob.entries[rob_idx].uop.queue.rob_allocation_id != uop.queue.rob_allocation_id) return true;
    if (is_store) {
        if (state.lsu.stq_count>=STQ_DEPTH) return false;
        enqueue_store(state, uop, memory_address, store_data, memory_mask,
                      memory_size, state.rob.entries[rob_idx]);
    } else if (is_load) {
        if (state.lsu.ldq_count>=LDQ_DEPTH) return false;
        enqueue_load(state, uop, signed_load, memory_address, memory_mask,
                     memory_size, state.rob.entries[rob_idx]);
    }
    return true;
}

static void reclaim_ldq(BoomCoreState& state, uint8_t rob_idx, uint32_t allocation_id) {
    LoadQueueEntry compacted[LDQ_DEPTH];
    int count=0;
    for (int i=0; i<LDQ_DEPTH; i++)
        if (state.lsu.ldq[i].valid && (state.lsu.ldq[i].rob_idx!=rob_idx ||
            state.lsu.ldq[i].rob_allocation_id!=allocation_id)) compacted[count++]=state.lsu.ldq[i];
    for (int i=count; i<LDQ_DEPTH; i++) compacted[i]=LoadQueueEntry();
    for (int i=0; i<LDQ_DEPTH; i++) state.lsu.ldq[i]=compacted[i];
    state.lsu.ldq_head=0; state.lsu.ldq_tail=(uint8_t)(count%LDQ_DEPTH); state.lsu.ldq_count=(uint8_t)count;
}

bool lsu_finish_load_response(BoomCoreState& state, uint8_t rob_idx,
                              uint32_t allocation_id,
                              uint32_t transaction_id) {
    if (!state.lsu.load_response_pending ||
        state.lsu.pending_load_transaction_id != transaction_id ||
        state.lsu.pending_load_rob_idx != rob_idx ||
        state.lsu.pending_load_allocation_id != allocation_id) return false;
    bool owns_ldq = false;
LSU_RESPONSE_OWNERSHIP_SCAN:
    for (int i = 0; i < LDQ_DEPTH; i++)
        if (state.lsu.ldq[i].valid && state.lsu.ldq[i].rob_idx == rob_idx &&
            state.lsu.ldq[i].rob_allocation_id == allocation_id)
            owns_ldq = true;
    if (!owns_ldq) return false;
    state.lsu.load_response_pending = false;
    state.lsu.pending_load_transaction_id = 0;
    state.lsu.pending_load_rob_idx = 0;
    state.lsu.pending_load_allocation_id = 0;
    reclaim_ldq(state, rob_idx, allocation_id);
    return true;
}

void lsu_reclaim_store(BoomCoreState& state, uint8_t rob_idx, uint32_t allocation_id) {
    StoreQueueEntry compacted[STQ_DEPTH];
    int count=0;
    for (int i=0; i<STQ_DEPTH; i++)
        if (state.lsu.stq[i].valid && (state.lsu.stq[i].rob_idx!=rob_idx ||
            state.lsu.stq[i].rob_allocation_id!=allocation_id)) compacted[count++]=state.lsu.stq[i];
    for (int i=count; i<STQ_DEPTH; i++) compacted[i]=StoreQueueEntry();
    for (int i=0; i<STQ_DEPTH; i++) state.lsu.stq[i]=compacted[i];
    state.lsu.stq_head=0; state.lsu.stq_tail=(uint8_t)(count%STQ_DEPTH); state.lsu.stq_count=(uint8_t)count;
}

static void enqueue_load(BoomCoreState& state, const MicroOp& uop,
                         bool signed_load, uint64_t address,
                         uint8_t mask, uint8_t size, RobEntry& entry) {
    entry.memory_valid = true;
    entry.is_load = true;
    entry.is_store = false;
    entry.signed_load = signed_load;
    entry.memory_address = address;
    entry.memory_mask = mask;
    entry.memory_size = size;

    LsuState& lsu = state.lsu;
    if (lsu.ldq_count < LDQ_DEPTH) {
        LoadQueueEntry& ldq = lsu.ldq[lsu.ldq_tail];
        ldq.valid = true;
        ldq.rob_idx = uop.queue.rob_idx;
        ldq.rob_allocation_id = uop.queue.rob_allocation_id;
        ldq.address = address;
        ldq.size = size;
        ldq.signed_load = signed_load;
        ldq.branch_mask = uop.branch.br_mask;
        lsu.ldq_tail = (lsu.ldq_tail + 1) % LDQ_DEPTH;
        lsu.ldq_count++;
    }
}

void lsu_module(BoomCoreState& state, PipeSignals& pipe) {
    if (state.global_flush) {
        clear_lsu_queues(state.lsu);
        return;
    }

LSU_LOAD_ISSUE_SCAN:
    for (int i = 0; i < ROB_DEPTH; i++) {
        uint8_t idx = (state.rob.head + i) % ROB_DEPTH;
        try_issue_load(state, pipe, idx);
        if (state.lsu.load_response_pending) break;
    }
}

void commit_module(BoomCoreState& state) { (void)state; }

}
