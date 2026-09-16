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
    lsu.pending_load_lq_index = 0;
    lsu.pending_load_lq_generation = 0;
CLEAR_LDQ:
    for (int i = 0; i < LQ_DEPTH; i++) {
        lsu.ldq[i].valid = false;
        lsu.ldq[i].response_pending = false;
    }
CLEAR_STQ:
    for (int i = 0; i < SQ_DEPTH; i++) lsu.stq[i].valid = false;
}

static bool try_issue_load(BoomCoreState& state, PipeSignals& pipe, uint8_t rob_idx) {
    RobEntry& entry = state.rob.entries[rob_idx];
    if (!entry.valid || !entry.is_load || !entry.memory_valid || entry.memory_request_sent) return false;
    if (state.lsu.load_response_pending || older_store_in_rob(state, rob_idx) || pipe.dmem_req.full()) return false;

    if (entry.uop.queue.ldq_idx >= LQ_DEPTH) return false;
    int lq_index = entry.uop.queue.ldq_idx;
    const LoadQueueEntry& owner = state.lsu.ldq[lq_index];
    if (!owner.valid || owner.generation != entry.uop.queue.ldq_generation ||
        owner.rob_idx != rob_idx ||
        owner.rob_allocation_id != entry.uop.queue.rob_allocation_id) return false;

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
    state.lsu.pending_load_lq_index = (LqIndex)lq_index;
    state.lsu.pending_load_lq_generation = state.lsu.ldq[lq_index].generation;
    state.lsu.ldq[lq_index].transaction_id = tx;
    state.lsu.ldq[lq_index].response_pending = true;
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
    if (lsu.stq_count < SQ_DEPTH) {
        int slot = -1;
FIND_FREE_STQ:
        for (int offset = 0; offset < SQ_DEPTH; offset++) {
            int candidate = ((int)lsu.stq_tail + offset) % SQ_DEPTH;
            if (slot < 0 && !lsu.stq[candidate].valid) slot = candidate;
        }
        StoreQueueEntry& stq = lsu.stq[slot];
        uint16_t generation = (uint16_t)(stq.generation + 1);
        stq = StoreQueueEntry();
        stq.generation = generation;
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
        entry.uop.queue.stq_idx = (uint8_t)slot;
        entry.uop.queue.stq_generation = generation;
        lsu.stq_tail = (SqIndex)((slot + 1) % SQ_DEPTH);
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

static void reclaim_ldq(BoomCoreState& state, LqIndex lq_index, uint16_t generation,
                        uint8_t rob_idx, uint32_t allocation_id) {
    LoadQueueEntry& entry = state.lsu.ldq[(int)lq_index];
    if (!entry.valid || entry.generation != generation || entry.rob_idx != rob_idx ||
        entry.rob_allocation_id != allocation_id) return;
    entry.valid = false;
    entry.response_pending = false;
    if (state.lsu.ldq_count != 0) state.lsu.ldq_count--;
    state.lsu.ldq_head = lq_index;
}

bool lsu_finish_load_response(BoomCoreState& state, uint8_t rob_idx,
                              uint32_t allocation_id,
                              uint32_t transaction_id) {
    if (!state.lsu.load_response_pending ||
        state.lsu.pending_load_transaction_id != transaction_id ||
        state.lsu.pending_load_rob_idx != rob_idx ||
        state.lsu.pending_load_allocation_id != allocation_id) return false;
    LqIndex lq_index = state.lsu.pending_load_lq_index;
    uint16_t generation = state.lsu.pending_load_lq_generation;
    const LoadQueueEntry& owner = state.lsu.ldq[(int)lq_index];
    if (!owner.valid || owner.generation != generation || owner.rob_idx != rob_idx ||
        owner.rob_allocation_id != allocation_id || owner.transaction_id != transaction_id ||
        !owner.response_pending) return false;
    state.lsu.load_response_pending = false;
    state.lsu.pending_load_transaction_id = 0;
    state.lsu.pending_load_rob_idx = 0;
    state.lsu.pending_load_allocation_id = 0;
    state.lsu.pending_load_lq_index = 0;
    state.lsu.pending_load_lq_generation = 0;
    reclaim_ldq(state, lq_index, generation, rob_idx, allocation_id);
    return true;
}

bool lsu_reclaim_store(BoomCoreState& state, SqIndex sq_index, uint16_t generation,
                       uint8_t rob_idx, uint32_t allocation_id) {
    StoreQueueEntry& entry = state.lsu.stq[(int)sq_index];
    if (!entry.valid || entry.generation != generation || entry.rob_idx != rob_idx ||
        entry.rob_allocation_id != allocation_id) return false;
    entry.valid = false;
    if (state.lsu.stq_count != 0) state.lsu.stq_count--;
    state.lsu.stq_head = sq_index;
    return true;
}

bool lsu_store_owner_matches(const BoomCoreState& state, SqIndex sq_index,
                             uint16_t generation, uint8_t rob_idx,
                             uint32_t allocation_id) {
    const StoreQueueEntry& entry = state.lsu.stq[(int)sq_index];
    return entry.valid && entry.generation == generation && entry.rob_idx == rob_idx &&
           entry.rob_allocation_id == allocation_id;
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
    if (lsu.ldq_count < LQ_DEPTH) {
        int slot = -1;
FIND_FREE_LDQ:
        for (int offset = 0; offset < LQ_DEPTH; offset++) {
            int candidate = ((int)lsu.ldq_tail + offset) % LQ_DEPTH;
            if (slot < 0 && !lsu.ldq[candidate].valid) slot = candidate;
        }
        LoadQueueEntry& ldq = lsu.ldq[slot];
        uint16_t generation = (uint16_t)(ldq.generation + 1);
        ldq = LoadQueueEntry();
        ldq.generation = generation;
        ldq.valid = true;
        ldq.rob_idx = uop.queue.rob_idx;
        ldq.rob_allocation_id = uop.queue.rob_allocation_id;
        ldq.address = address;
        ldq.size = size;
        ldq.signed_load = signed_load;
        ldq.branch_mask = uop.branch.br_mask;
        entry.uop.queue.ldq_idx = (uint8_t)slot;
        entry.uop.queue.ldq_generation = generation;
        lsu.ldq_tail = (LqIndex)((slot + 1) % LQ_DEPTH);
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
