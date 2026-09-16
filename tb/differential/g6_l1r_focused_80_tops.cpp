#include "boom_config.hpp"
#include "boom_state.hpp"
#include "boom_interfaces.hpp"
#include "completion.hpp"
#include <cstdint>

namespace boom {
bool lsu_accept_completion(BoomCoreState&, const MicroOp&, bool, bool, bool,
                           uint64_t, uint64_t, uint8_t, uint8_t);
bool lsu_finish_load_response(BoomCoreState&, uint8_t, uint32_t, uint32_t);
bool lsu_reclaim_store(BoomCoreState&, SqIndex, uint16_t, uint8_t, uint32_t);
bool lsu_store_owner_matches(const BoomCoreState&, SqIndex, uint16_t, uint8_t,
                             uint32_t);
void lsu_module(BoomCoreState&, PipeSignals&);
void branch_complete_event(BoomCoreState&, const MicroOp&, bool, uint64_t);
}

#define FOCUSED_80_INTERFACES() \
    _Pragma("HLS INTERFACE ap_ctrl_hs port=return") \
    _Pragma("HLS INTERFACE ap_none port=obs0") \
    _Pragma("HLS INTERFACE ap_none port=obs1") \
    _Pragma("HLS INTERFACE ap_none port=obs2") \
    _Pragma("HLS INTERFACE ap_none port=obs3") \
    _Pragma("HLS INTERFACE ap_none port=obs4") \
    _Pragma("HLS INTERFACE ap_none port=obs5") \
    _Pragma("HLS INTERFACE ap_none port=obs6") \
    _Pragma("HLS INTERFACE ap_none port=obs7")

static MicroOp focused_80_uop(uint8_t rob_idx, uint32_t allocation_id,
                              uint8_t branch_mask, bool store) {
    MicroOp uop;
    uop.queue.rob_idx = rob_idx;
    uop.queue.rob_allocation_id = allocation_id;
    uop.branch.br_mask = branch_mask;
    uop.ctrl.is_load = !store;
    uop.ctrl.is_sta = store;
    uop.mem.uses_ldq = !store;
    uop.mem.uses_stq = store;
    return uop;
}

static void focused_80_install(BoomCoreState& state, const MicroOp& uop,
                               bool valid) {
    RobEntry& entry = state.rob.entries[uop.queue.rob_idx];
    entry = RobEntry();
    entry.valid = valid;
    entry.busy = valid;
    entry.uop = uop;
}

static uint8_t focused_80_valid_lq(const BoomCoreState& state) {
    uint8_t result = 0;
    for (int i = 0; i < LQ_DEPTH; ++i) result += state.lsu.ldq[i].valid;
    return result;
}

static uint8_t focused_80_valid_sq(const BoomCoreState& state) {
    uint8_t result = 0;
    for (int i = 0; i < SQ_DEPTH; ++i) result += state.lsu.stq[i].valid;
    return result;
}

extern "C" void g6_l1r80_queue_allocate(
        uint8_t queue_kind, uint8_t tail, uint8_t initial_count,
        uint8_t allocations, uint16_t generation, uint32_t allocation_base,
        uint8_t mask, uint64_t address,
        uint64_t& obs0, uint64_t& obs1, uint64_t& obs2, uint64_t& obs3,
        uint64_t& obs4, uint64_t& obs5, uint64_t& obs6, uint64_t& obs7) {
    FOCUSED_80_INTERFACES();
    BoomCoreState state;
    const bool store = queue_kind != 0;
    state.lsu.ldq_tail = (LqIndex)(tail % LQ_DEPTH);
    state.lsu.stq_tail = (SqIndex)(tail % SQ_DEPTH);
    for (int i = 0; i < LQ_DEPTH; ++i) {
        state.lsu.ldq[i].valid = !store && i < initial_count;
        if (state.lsu.ldq[i].valid) {
            state.lsu.ldq[i].rob_idx = i;
            state.lsu.ldq[i].rob_allocation_id = allocation_base + i;
        }
    }
    for (int i = 0; i < SQ_DEPTH; ++i) {
        state.lsu.stq[i].valid = store && i < initial_count;
        if (state.lsu.stq[i].valid) {
            state.lsu.stq[i].rob_idx = i;
            state.lsu.stq[i].rob_allocation_id = allocation_base + i;
        }
    }
    state.lsu.ldq_count = store ? 0 : initial_count;
    state.lsu.stq_count = store ? initial_count : 0;
    if (store) state.lsu.stq[tail % SQ_DEPTH].generation = generation;
    else state.lsu.ldq[tail % LQ_DEPTH].generation = generation;
    const uint8_t second_slot = store ? (tail + 1) % SQ_DEPTH :
                                        (tail + 1) % LQ_DEPTH;
    if (store) state.lsu.stq[second_slot].generation = generation;
    else state.lsu.ldq[second_slot].generation = generation;

    const uint8_t pre_lq = state.lsu.ldq_count;
    const uint8_t pre_sq = state.lsu.stq_count;
    const uint8_t pre_valid_lq = focused_80_valid_lq(state);
    const uint8_t pre_valid_sq = focused_80_valid_sq(state);
    MicroOp first = focused_80_uop(24, allocation_base + 0x100, 0, store);
    focused_80_install(state, first, allocations > 0);
    bool accepted_first = boom::lsu_accept_completion(
        state, first, !store, store, false, address, allocation_base, mask, 3);
    const uint8_t first_index = store ? state.rob.entries[24].uop.queue.stq_idx :
                                        state.rob.entries[24].uop.queue.ldq_idx;
    const uint16_t first_generation = store ?
        state.rob.entries[24].uop.queue.stq_generation :
        state.rob.entries[24].uop.queue.ldq_generation;
    const uint8_t count_after_first = store ? state.lsu.stq_count : state.lsu.ldq_count;

    MicroOp second = focused_80_uop(25, allocation_base + 0x101, 0, store);
    focused_80_install(state, second, allocations > 1);
    bool accepted_second = boom::lsu_accept_completion(
        state, second, !store, store, false, address + 8, allocation_base + 1,
        mask, 3);
    const uint8_t second_index = store ? state.rob.entries[25].uop.queue.stq_idx :
                                         state.rob.entries[25].uop.queue.ldq_idx;
    const uint16_t second_generation = store ?
        state.rob.entries[25].uop.queue.stq_generation :
        state.rob.entries[25].uop.queue.ldq_generation;
    obs0 = (uint64_t)pre_lq | ((uint64_t)pre_sq << 8) |
           ((uint64_t)pre_valid_lq << 16) | ((uint64_t)pre_valid_sq << 24);
    obs1 = (uint64_t)accepted_first | ((uint64_t)accepted_second << 1);
    obs2 = (uint64_t)first_index | ((uint64_t)first_generation << 8);
    obs3 = (uint64_t)second_index | ((uint64_t)second_generation << 8);
    obs4 = (uint64_t)count_after_first |
           ((uint64_t)(store ? state.lsu.stq_count : state.lsu.ldq_count) << 8);
    obs5 = (uint64_t)focused_80_valid_lq(state) |
           ((uint64_t)focused_80_valid_sq(state) << 8);
    obs6 = (uint64_t)state.lsu.ldq_tail | ((uint64_t)state.lsu.stq_tail << 8);
    obs7 = mask;
}

extern "C" void g6_l1r80_queue_lifecycle(
        uint8_t queue_kind, uint8_t initial_count, uint8_t initial_slot,
        uint16_t generation, uint8_t phase_enables, uint32_t allocation_base,
        uint32_t transaction, uint8_t mask,
        uint64_t& obs0, uint64_t& obs1, uint64_t& obs2, uint64_t& obs3,
        uint64_t& obs4, uint64_t& obs5, uint64_t& obs6, uint64_t& obs7) {
    FOCUSED_80_INTERFACES();
    BoomCoreState state;
    const bool store = queue_kind != 0;
    const uint8_t depth = store ? SQ_DEPTH : LQ_DEPTH;
    const uint8_t selected = initial_slot % depth;
    for (int i = 0; i < LQ_DEPTH; ++i) {
        state.lsu.ldq[i].valid = !store && i < initial_count;
        state.lsu.ldq[i].rob_idx = i;
        state.lsu.ldq[i].rob_allocation_id = allocation_base + i;
        state.lsu.ldq[i].generation = i == selected ? generation : 1;
    }
    for (int i = 0; i < SQ_DEPTH; ++i) {
        state.lsu.stq[i].valid = store && i < initial_count;
        state.lsu.stq[i].rob_idx = i;
        state.lsu.stq[i].rob_allocation_id = allocation_base + i;
        state.lsu.stq[i].generation = i == selected ? generation : 1;
    }
    state.lsu.ldq_count = store ? 0 : initial_count;
    state.lsu.stq_count = store ? initial_count : 0;
    state.lsu.ldq_tail = (LqIndex)(initial_count % LQ_DEPTH);
    state.lsu.stq_tail = (SqIndex)(initial_count % SQ_DEPTH);

    MicroOp a = focused_80_uop(24, allocation_base + 0x100, 0, store);
    bool accepted_a = false;
    uint8_t a_slot = 0;
    uint16_t a_generation = 0;
    if ((phase_enables & 1) != 0) {
        focused_80_install(state, a, true);
        accepted_a = boom::lsu_accept_completion(
            state, a, !store, store, false, 0x8000, allocation_base, mask, 3);
        a_slot = store ? state.rob.entries[24].uop.queue.stq_idx :
                         state.rob.entries[24].uop.queue.ldq_idx;
        a_generation = store ? state.rob.entries[24].uop.queue.stq_generation :
                               state.rob.entries[24].uop.queue.ldq_generation;
    }
    const uint8_t count_a = store ? state.lsu.stq_count : state.lsu.ldq_count;

    MicroOp b = focused_80_uop(25, allocation_base + 0x101, 0, store);
    bool accepted_b = false;
    if ((phase_enables & 2) != 0) {
        focused_80_install(state, b, true);
        accepted_b = boom::lsu_accept_completion(
            state, b, !store, store, false, 0x8008, allocation_base + 1, mask, 3);
    }
    const uint8_t count_b = store ? state.lsu.stq_count : state.lsu.ldq_count;

    const bool reclaim_initial = (phase_enables & 8) != 0;
    const uint8_t reclaim_slot = reclaim_initial ? selected : a_slot;
    const uint8_t reclaim_rob = reclaim_initial ? selected : 24;
    const uint32_t reclaim_allocation = reclaim_initial ?
        allocation_base + selected : allocation_base + 0x100;
    const uint16_t reclaim_generation = reclaim_initial ? generation : a_generation;
    bool exact_match = true;
    bool stale_match = false;
    bool wrong_rob_match = false;
    bool stale_reclaimed = false;
    bool reclaimed = false;
    if (store) {
        exact_match = boom::lsu_store_owner_matches(
            state, (SqIndex)reclaim_slot, reclaim_generation, reclaim_rob,
            reclaim_allocation);
        stale_match = boom::lsu_store_owner_matches(
            state, (SqIndex)reclaim_slot, reclaim_generation + 1, reclaim_rob,
            reclaim_allocation);
        wrong_rob_match = boom::lsu_store_owner_matches(
            state, (SqIndex)reclaim_slot, reclaim_generation,
            (uint8_t)(reclaim_rob + 1), reclaim_allocation);
        stale_reclaimed = boom::lsu_reclaim_store(
            state, (SqIndex)reclaim_slot, (uint16_t)(reclaim_generation + 1),
            reclaim_rob, reclaim_allocation);
        reclaimed = boom::lsu_reclaim_store(
            state, (SqIndex)reclaim_slot, reclaim_generation, reclaim_rob,
            reclaim_allocation);
    } else {
        LoadQueueEntry& owner = state.lsu.ldq[reclaim_slot];
        owner.transaction_id = transaction;
        owner.response_pending = true;
        state.lsu.load_response_pending = true;
        state.lsu.pending_load_transaction_id = transaction;
        state.lsu.pending_load_rob_idx = reclaim_rob;
        state.lsu.pending_load_allocation_id = reclaim_allocation;
        state.lsu.pending_load_lq_index = (LqIndex)reclaim_slot;
        state.lsu.pending_load_lq_generation = reclaim_generation;
        reclaimed = boom::lsu_finish_load_response(
            state, reclaim_rob, reclaim_allocation, transaction);
    }
    const uint8_t count_reclaim = store ? state.lsu.stq_count : state.lsu.ldq_count;
    if (store) state.lsu.stq_tail = (SqIndex)reclaim_slot;
    else state.lsu.ldq_tail = (LqIndex)reclaim_slot;

    MicroOp c = focused_80_uop(26, allocation_base + 0x102, 0, store);
    bool accepted_c = false;
    uint8_t c_slot = 0;
    uint16_t c_generation = 0;
    if ((phase_enables & 4) != 0) {
        focused_80_install(state, c, true);
        accepted_c = boom::lsu_accept_completion(
            state, c, !store, store, false, 0x8010, allocation_base + 2, mask, 3);
        c_slot = store ? state.rob.entries[26].uop.queue.stq_idx :
                         state.rob.entries[26].uop.queue.ldq_idx;
        c_generation = store ? state.rob.entries[26].uop.queue.stq_generation :
                               state.rob.entries[26].uop.queue.ldq_generation;
    }
    obs0 = (uint64_t)accepted_a | ((uint64_t)accepted_b << 1) |
           ((uint64_t)reclaimed << 2) | ((uint64_t)accepted_c << 3);
    obs1 = (uint64_t)count_a | ((uint64_t)count_b << 8) |
           ((uint64_t)count_reclaim << 16) |
           ((uint64_t)(store ? state.lsu.stq_count : state.lsu.ldq_count) << 24);
    obs2 = (uint64_t)a_slot | ((uint64_t)a_generation << 8);
    obs3 = (uint64_t)c_slot | ((uint64_t)c_generation << 8);
    obs4 = (uint64_t)exact_match | ((uint64_t)stale_match << 1) |
           ((uint64_t)wrong_rob_match << 2) | ((uint64_t)stale_reclaimed << 3);
    obs5 = (uint64_t)focused_80_valid_lq(state) |
           ((uint64_t)focused_80_valid_sq(state) << 8);
    obs6 = (uint64_t)reclaim_slot | ((uint64_t)reclaim_generation << 8);
    obs7 = (uint64_t)mask | ((uint64_t)transaction << 8);
}

extern "C" void g6_l1r80_load_response(
        uint8_t rob_idx, uint8_t slot, uint16_t generation,
        uint32_t live_allocation, uint32_t pending_allocation,
        uint32_t owner_allocation, uint32_t transaction,
        uint32_t pending_transaction, uint32_t owner_transaction,
        uint32_t first_response_transaction, uint32_t second_response_transaction,
        uint16_t pending_generation, uint8_t pending_rob, uint8_t owner_rob,
        uint8_t flags, uint8_t pdst, uint8_t memory_mask, uint8_t sequence_mode,
        uint64_t data, uint64_t address,
        uint64_t& obs0, uint64_t& obs1, uint64_t& obs2, uint64_t& obs3,
        uint64_t& obs4, uint64_t& obs5, uint64_t& obs6, uint64_t& obs7) {
    FOCUSED_80_INTERFACES();
    BoomCoreState state;
    const uint8_t lq_slot = slot % LQ_DEPTH;
    const uint32_t installed_allocation = sequence_mode == 0 ?
        live_allocation : pending_allocation;
    MicroOp uop = focused_80_uop(rob_idx, installed_allocation, 0, false);
    uop.queue.ldq_idx = lq_slot;
    uop.queue.ldq_generation = generation;
    uop.rename.dst_rtype = DST_INT;
    uop.rename.pdst = pdst;
    focused_80_install(state, uop, (flags & 1) != 0);
    RobEntry& rob = state.rob.entries[rob_idx];
    rob.busy = (flags & 2) != 0;
    rob.is_load = true;
    rob.memory_valid = true;
    rob.memory_request_sent = (flags & 4) != 0;
    rob.memory_completed = (flags & 8) != 0;
    rob.memory_transaction_id = transaction;
    rob.memory_address = address;
    rob.memory_size = 3;
    rob.memory_mask = memory_mask;
    LoadQueueEntry& owner = state.lsu.ldq[lq_slot];
    owner.valid = (flags & 16) != 0;
    owner.response_pending = (flags & 32) != 0;
    owner.rob_idx = owner_rob;
    owner.rob_allocation_id = owner_allocation;
    owner.generation = generation;
    owner.transaction_id = owner_transaction;
    state.lsu.ldq_count = owner.valid ? 1 : 0;
    state.lsu.load_response_pending = (flags & 64) != 0;
    state.lsu.pending_load_transaction_id = pending_transaction;
    state.lsu.pending_load_rob_idx = pending_rob;
    state.lsu.pending_load_allocation_id = pending_allocation;
    state.lsu.pending_load_lq_index = (LqIndex)lq_slot;
    state.lsu.pending_load_lq_generation = pending_generation;
    boom::prf_seed(state, pdst, 0xaaaaaaaaaaaaaaaaULL);

    if (sequence_mode == 1) {
        PipeSignals pipe;
        state.global_flush = true;
        boom::lsu_module(state, pipe);
        state.global_flush = false;
        state.lsu.ldq_tail = (LqIndex)lq_slot;
        state.lsu.ldq[lq_slot].generation = generation;
        MicroOp reused = focused_80_uop(rob_idx, live_allocation, 0, false);
        reused.rename.dst_rtype = DST_INT;
        reused.rename.pdst = pdst;
        focused_80_install(state, reused, true);
        boom::lsu_accept_completion(state, reused, true, false, false,
                                    address + 8, 0, memory_mask, 3);
    } else if (sequence_mode == 2) {
        MicroOp reused = focused_80_uop(rob_idx, live_allocation, 0, false);
        reused.rename.dst_rtype = DST_INT;
        reused.rename.pdst = pdst;
        focused_80_install(state, reused, true);
    }

    DmemResponse first_response;
    first_response.transaction_id = first_response_transaction;
    first_response.data = data;
    CompletionEvent first_event;
    boom::completion_from_load_response(state, first_response, first_event);
    bool first_applied = boom::apply_completion(state, first_event);
    bool first_finished = first_event.valid && boom::lsu_finish_load_response(
        state, first_event.uop.queue.rob_idx,
        first_event.uop.queue.rob_allocation_id,
        first_event.transaction_id);
    const uint64_t first_prf = boom::prf_read(state, pdst);
    const uint8_t first_lq_count = state.lsu.ldq_count;
    const bool first_owner_valid = state.lsu.ldq[lq_slot].valid;
    const bool first_pending = state.lsu.load_response_pending;
    const bool first_rob_busy = state.rob.entries[rob_idx].busy;
    const bool first_completed = state.rob.entries[rob_idx].memory_completed;

    DmemResponse second_response;
    second_response.transaction_id = second_response_transaction;
    second_response.data = data + 1;
    CompletionEvent second_event;
    boom::completion_from_load_response(state, second_response, second_event);
    bool second_applied = boom::apply_completion(state, second_event);
    bool second_finished = second_event.valid && boom::lsu_finish_load_response(
        state, second_event.uop.queue.rob_idx,
        second_event.uop.queue.rob_allocation_id,
        second_event.transaction_id);
    obs0 = (uint64_t)first_event.valid | ((uint64_t)first_applied << 1) |
           ((uint64_t)first_finished << 2) | ((uint64_t)second_event.valid << 3) |
           ((uint64_t)second_applied << 4) | ((uint64_t)second_finished << 5);
    obs1 = (uint64_t)first_lq_count | ((uint64_t)first_owner_valid << 8) |
           ((uint64_t)first_pending << 9) | ((uint64_t)first_rob_busy << 10) |
           ((uint64_t)first_completed << 11);
    obs2 = first_prf;
    obs3 = boom::prf_read(state, pdst);
    obs4 = (uint64_t)state.lsu.ldq_count |
           ((uint64_t)state.lsu.ldq[lq_slot].valid << 8) |
           ((uint64_t)state.lsu.load_response_pending << 9);
    obs5 = (uint64_t)state.rob.entries[rob_idx].busy |
           ((uint64_t)state.rob.entries[rob_idx].memory_completed << 1);
    obs6 = (uint64_t)owner.rob_idx | ((uint64_t)owner.generation << 8);
    obs7 = (uint64_t)memory_mask | ((uint64_t)transaction << 8);
}

extern "C" void g6_l1r80_branch_recovery(
        uint8_t tag, uint8_t lq_slot, uint8_t sq_slot,
        uint16_t generation, uint32_t allocation_base,
        uint32_t transaction, uint8_t queue_valids,
        uint8_t lq_branch_mask, uint8_t sq_branch_mask, uint8_t memory_mask,
        uint8_t mispredict,
        uint64_t& obs0, uint64_t& obs1, uint64_t& obs2, uint64_t& obs3,
        uint64_t& obs4, uint64_t& obs5, uint64_t& obs6, uint64_t& obs7) {
    FOCUSED_80_INTERFACES();
    BoomCoreState state;
    const uint8_t bit = 1u << (tag & 7);
    MicroOp branch = focused_80_uop(0, allocation_base, 0, false);
    branch.branch.is_br = true;
    branch.branch.br_tag = tag & 7;
    focused_80_install(state, branch, true);
    state.lsu.ldq_tail = (LqIndex)(lq_slot % LQ_DEPTH);
    state.lsu.stq_tail = (SqIndex)(sq_slot % SQ_DEPTH);
    state.lsu.ldq[lq_slot % LQ_DEPTH].generation = generation;
    state.lsu.stq[sq_slot % SQ_DEPTH].generation = generation;
    MicroOp load = focused_80_uop(1, allocation_base + 1, lq_branch_mask, false);
    load.rename.dst_rtype = DST_INT;
    load.rename.pdst = 5;
    bool load_accepted = false;
    if ((queue_valids & 1) != 0) {
        focused_80_install(state, load, true);
        load_accepted = boom::lsu_accept_completion(
            state, load, true, false, false, 0xa000, 0, memory_mask, 3);
    }
    MicroOp store = focused_80_uop(2, allocation_base + 2, sq_branch_mask, true);
    bool store_accepted = false;
    if ((queue_valids & 2) != 0) {
        focused_80_install(state, store, true);
        store_accepted = boom::lsu_accept_completion(
            state, store, false, true, false, 0xa008, allocation_base,
            memory_mask, 3);
    }
    LoadQueueEntry& lq = state.lsu.ldq[lq_slot % LQ_DEPTH];
    StoreQueueEntry& sq = state.lsu.stq[sq_slot % SQ_DEPTH];
    lq.response_pending = lq.valid && (queue_valids & 4) != 0;
    lq.transaction_id = transaction;
    state.lsu.load_response_pending = lq.response_pending;
    state.lsu.pending_load_transaction_id = transaction;
    state.lsu.pending_load_rob_idx = 1;
    state.lsu.pending_load_allocation_id = allocation_base + 1;
    state.lsu.pending_load_lq_index = (LqIndex)(lq_slot % LQ_DEPTH);
    state.lsu.pending_load_lq_generation = lq.generation;
    boom::prf_seed(state, 5, 0xaaaaaaaaaaaaaaaaULL);
    state.rob.head = 0;
    state.rob.tail = 3;
    state.branch_state.active_mask = bit;
    state.branch_state.tag_valid[tag & 7] = true;
    state.branch_state.snapshot_valid[tag & 7] = true;
    boom::branch_complete_event(state, branch, mispredict != 0, 0x9000);

    DmemResponse response;
    response.transaction_id = transaction;
    response.data = 0x55;
    CompletionEvent late;
    boom::completion_from_load_response(state, response, late);
    bool late_applied = boom::apply_completion(state, late);
    obs0 = (uint64_t)lq.valid | ((uint64_t)sq.valid << 1) |
           ((uint64_t)state.lsu.load_response_pending << 2) |
           ((uint64_t)late.valid << 3) | ((uint64_t)late_applied << 4);
    obs1 = (uint64_t)state.lsu.ldq_count | ((uint64_t)state.lsu.stq_count << 8);
    obs2 = (uint64_t)lq.branch_mask | ((uint64_t)sq.branch_mask << 8);
    obs3 = (uint64_t)state.rob.entries[1].valid |
           ((uint64_t)state.rob.entries[2].valid << 1);
    obs4 = (uint64_t)state.brupdate.valid |
           ((uint64_t)state.brupdate.mispredict << 1) |
           ((uint64_t)state.brupdate.resolve_mask << 8) |
           ((uint64_t)state.brupdate.mispredict_mask << 16);
    obs5 = state.branch_state.active_mask;
    obs6 = (uint64_t)load_accepted | ((uint64_t)store_accepted << 1) |
           ((uint64_t)lq.generation << 8) | ((uint64_t)sq.generation << 24);
    obs7 = boom::prf_read(state, 5);
}

extern "C" void g6_l1r80_global_flush(
        uint8_t lq_slot, uint8_t sq_slot, uint16_t generation,
        uint32_t allocation, uint32_t transaction, uint8_t branch_mask,
        uint64_t& obs0, uint64_t& obs1, uint64_t& obs2, uint64_t& obs3,
        uint64_t& obs4, uint64_t& obs5, uint64_t& obs6, uint64_t& obs7) {
    FOCUSED_80_INTERFACES();
    BoomCoreState state;
    PipeSignals pipe;
    state.lsu.ldq_tail = (LqIndex)(lq_slot % LQ_DEPTH);
    state.lsu.stq_tail = (SqIndex)(sq_slot % SQ_DEPTH);
    state.lsu.ldq[lq_slot % LQ_DEPTH].generation = generation;
    state.lsu.stq[sq_slot % SQ_DEPTH].generation = generation;
    MicroOp load = focused_80_uop(1, allocation, branch_mask, false);
    focused_80_install(state, load, true);
    bool load_accepted = boom::lsu_accept_completion(
        state, load, true, false, false, 0xb000, 0, branch_mask, 3);
    MicroOp store = focused_80_uop(2, allocation + 1, branch_mask, true);
    focused_80_install(state, store, true);
    bool store_accepted = boom::lsu_accept_completion(
        state, store, false, true, false, 0xb008, allocation,
        branch_mask, 3);
    LoadQueueEntry& lq = state.lsu.ldq[lq_slot % LQ_DEPTH];
    StoreQueueEntry& sq = state.lsu.stq[sq_slot % SQ_DEPTH];
    lq.response_pending = true;
    lq.transaction_id = transaction;
    state.lsu.load_response_pending = true;
    state.lsu.pending_load_transaction_id = transaction;
    state.lsu.pending_load_rob_idx = 1;
    state.lsu.pending_load_allocation_id = allocation;
    state.lsu.pending_load_lq_index = (LqIndex)(lq_slot % LQ_DEPTH);
    state.lsu.pending_load_lq_generation = generation;
    state.global_flush = true;
    boom::lsu_module(state, pipe);
    obs0 = (uint64_t)state.lsu.ldq_count | ((uint64_t)state.lsu.stq_count << 8);
    obs1 = (uint64_t)focused_80_valid_lq(state) |
           ((uint64_t)focused_80_valid_sq(state) << 8);
    obs2 = state.lsu.load_response_pending;
    obs3 = state.lsu.pending_load_transaction_id;
    obs4 = (uint64_t)state.lsu.pending_load_rob_idx |
           ((uint64_t)state.lsu.pending_load_lq_index << 8) |
           ((uint64_t)state.lsu.pending_load_lq_generation << 16);
    obs5 = (uint64_t)load_accepted | ((uint64_t)store_accepted << 1);
    obs6 = (uint64_t)generation | ((uint64_t)transaction << 16);
    obs7 = branch_mask;
}
