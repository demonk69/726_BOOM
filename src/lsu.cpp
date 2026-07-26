#include "boom_config.hpp"
#include "boom_types.hpp"
#include "boom_state.hpp"
#include "boom_interfaces.hpp"

namespace boom {

static uint8_t bytes_for_size(uint8_t size) { return (uint8_t)(1u << (size & 0x3)); }

static uint64_t sign_extend(uint64_t value, uint8_t bits) {
    if (bits >= 64) return value;
    uint64_t mask = 1ULL << (bits - 1);
    return (value ^ mask) - mask;
}

static uint64_t load_value(uint64_t data, uint64_t address, uint8_t size, bool signed_load) {
    uint8_t shift = (uint8_t)((address & 0x7) * 8);
    uint8_t bits = (uint8_t)(bytes_for_size(size) * 8);
    uint64_t mask = (bits >= 64) ? ~0ULL : ((1ULL << bits) - 1ULL);
    uint64_t value = (data >> shift) & mask;
    return signed_load ? sign_extend(value, bits) : value;
}

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
    return true;
}

static void enqueue_store(BoomCoreState& state, const ExecuteState::AluResult& result, RobEntry& entry) {
    entry.memory_valid = true;
    entry.is_store = true;
    entry.is_load = false;
    entry.memory_completed = true;
    entry.memory_address = result.memory_address;
    entry.memory_data = result.store_data;
    entry.memory_mask = result.memory_mask;
    entry.memory_size = result.memory_size;
    entry.busy = false;

    LsuState& lsu = state.lsu;
    if (lsu.stq_count < STQ_DEPTH) {
        StoreQueueEntry& stq = lsu.stq[lsu.stq_tail];
        stq.valid = true;
        stq.rob_idx = result.uop.queue.rob_idx;
        stq.address_valid = true;
        stq.address = result.memory_address;
        stq.data_valid = true;
        stq.data = result.store_data;
        stq.mask = result.memory_mask;
        stq.size = result.memory_size;
        stq.branch_mask = result.uop.branch.br_mask;
        lsu.stq_tail = (lsu.stq_tail + 1) % STQ_DEPTH;
        lsu.stq_count++;
    }
}

static void enqueue_load(BoomCoreState& state, const ExecuteState::AluResult& result, RobEntry& entry) {
    entry.memory_valid = true;
    entry.is_load = true;
    entry.is_store = false;
    entry.signed_load = result.signed_load;
    entry.memory_address = result.memory_address;
    entry.memory_mask = result.memory_mask;
    entry.memory_size = result.memory_size;

    LsuState& lsu = state.lsu;
    if (lsu.ldq_count < LDQ_DEPTH) {
        LoadQueueEntry& ldq = lsu.ldq[lsu.ldq_tail];
        ldq.valid = true;
        ldq.rob_idx = result.uop.queue.rob_idx;
        ldq.address = result.memory_address;
        ldq.size = result.memory_size;
        ldq.signed_load = result.signed_load;
        ldq.branch_mask = result.uop.branch.br_mask;
        lsu.ldq_tail = (lsu.ldq_tail + 1) % LDQ_DEPTH;
        lsu.ldq_count++;
    }
}

void lsu_module(BoomCoreState& state, PipeSignals& pipe) {
    if (state.global_flush) {
        clear_lsu_queues(state.lsu);
        return;
    }

    if (!pipe.dmem_resp.empty()) {
        DmemResponse resp = pipe.dmem_resp.read();
        uint32_t resp_tx = resp.transaction_id;
        if (state.lsu.load_response_pending && resp_tx == state.lsu.pending_load_transaction_id) {
            uint8_t rob_idx = state.lsu.pending_load_rob_idx;
            RobEntry& entry = state.rob.entries[rob_idx];
            if (entry.valid && entry.is_load && entry.memory_request_sent) {
                if (resp.exception) {
                    entry.exception = true;
                    entry.uop.exception = true;
                    entry.uop.exc_cause = resp.exception_cause ? resp.exception_cause : resp.exc_cause;
                } else {
                    uint64_t data = resp.read_data ? resp.read_data : resp.data;
                    uint64_t value = load_value(data, entry.memory_address, entry.memory_size, entry.signed_load);
                    if (entry.uop.rename.pdst != 0) {
                        state.int_rf[entry.uop.rename.pdst] = value;
                        state.rename.int_free_list.busy_table[entry.uop.rename.pdst] = false;
                    }
                    entry.memory_data = value;
                }
                entry.memory_completed = true;
                entry.busy = false;
            }
            state.lsu.load_response_pending = false;
            state.lsu.pending_load_transaction_id = 0;
        }
    }

LSU_EXECUTE_RESULTS:
    for (int i = 0; i < DISPATCH_WIDTH; i++) {
        const ExecuteState::AluResult& result = state.execute.alu_results[i];
        if (!result.valid || !result.memory_valid) continue;
        if (state.brupdate.valid && state.brupdate.mispredict &&
            ((result.uop.branch.br_mask & state.brupdate.mispredict_mask) != 0)) continue;
        uint8_t rob_idx = result.uop.queue.rob_idx;
        if (rob_idx >= ROB_DEPTH || !state.rob.entries[rob_idx].valid) continue;
        RobEntry& entry = state.rob.entries[rob_idx];
        if (result.is_store) enqueue_store(state, result, entry);
        else if (result.is_load) enqueue_load(state, result, entry);
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
