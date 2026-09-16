#include "boom_config.hpp"
#include "boom_state.hpp"
#include "boom_interfaces.hpp"
#include "completion.hpp"
#include "reset.hpp"
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

#define PILOT_INTERFACES() \
    _Pragma("HLS INTERFACE ap_ctrl_hs port=return") \
    _Pragma("HLS INTERFACE ap_none port=seed") \
    _Pragma("HLS INTERFACE ap_none port=obs0") \
    _Pragma("HLS INTERFACE ap_none port=obs1") \
    _Pragma("HLS INTERFACE ap_none port=obs2") \
    _Pragma("HLS INTERFACE ap_none port=obs3") \
    _Pragma("HLS INTERFACE ap_none port=obs4") \
    _Pragma("HLS INTERFACE ap_none port=obs5")

static MicroOp pilot_uop(uint8_t rob_idx, uint32_t allocation_id,
                         uint8_t branch_mask, bool is_store) {
    MicroOp uop;
    uop.queue.rob_idx = rob_idx;
    uop.queue.rob_allocation_id = allocation_id;
    uop.branch.br_mask = branch_mask;
    uop.ctrl.is_load = !is_store;
    uop.ctrl.is_sta = is_store;
    uop.mem.uses_ldq = !is_store;
    uop.mem.uses_stq = is_store;
    return uop;
}

static void pilot_install(BoomCoreState& state, const MicroOp& uop) {
    RobEntry& entry = state.rob.entries[uop.queue.rob_idx];
    entry = RobEntry();
    entry.valid = true;
    entry.busy = true;
    entry.uop = uop;
}

extern "C" void g6_l1r_pilot_lq_reuse(
        uint32_t seed, uint64_t& obs0, uint64_t& obs1, uint64_t& obs2,
        uint64_t& obs3, uint64_t& obs4, uint64_t& obs5) {
    PILOT_INTERFACES();
    BoomCoreState state;
    const uint32_t allocation = 0x12000000u | seed;
    state.lsu.ldq[0].generation = 0xffffu;
    MicroOp first = pilot_uop(1, allocation, 0, false);
    pilot_install(state, first);
    bool accepted_first = boom::lsu_accept_completion(
        state, first, true, false, false, 0x1000, 0, 0xff, 3);
    const uint8_t slot = state.rob.entries[1].uop.queue.ldq_idx;
    const uint16_t first_generation = state.rob.entries[1].uop.queue.ldq_generation;
    const uint32_t transaction = 0x51000000u | seed;
    LoadQueueEntry& owner = state.lsu.ldq[slot];
    owner.transaction_id = transaction;
    owner.response_pending = true;
    state.lsu.load_response_pending = true;
    state.lsu.pending_load_transaction_id = transaction;
    state.lsu.pending_load_rob_idx = 1;
    state.lsu.pending_load_allocation_id = allocation;
    state.lsu.pending_load_lq_index = (LqIndex)slot;
    state.lsu.pending_load_lq_generation = first_generation;
    bool reclaimed = boom::lsu_finish_load_response(state, 1, allocation, transaction);
    state.lsu.ldq_tail = (LqIndex)slot;
    MicroOp second = pilot_uop(2, allocation + 1, 0, false);
    pilot_install(state, second);
    bool accepted_second = boom::lsu_accept_completion(
        state, second, true, false, false, 0x1008, 0, 0xff, 3);
    obs0 = accepted_first;
    obs1 = reclaimed;
    obs2 = accepted_second;
    obs3 = first_generation;
    obs4 = state.rob.entries[2].uop.queue.ldq_generation;
    obs5 = (uint64_t)state.lsu.ldq_count |
           ((uint64_t)state.lsu.ldq[slot].valid << 8) |
           ((uint64_t)state.rob.entries[2].uop.queue.ldq_idx << 16);
}

extern "C" void g6_l1r_pilot_stale_response(
        uint32_t seed, uint64_t& obs0, uint64_t& obs1, uint64_t& obs2,
        uint64_t& obs3, uint64_t& obs4, uint64_t& obs5) {
    PILOT_INTERFACES();
    BoomCoreState state;
    PipeSignals pipe;
    const uint32_t old_allocation = 0x23000000u | seed;
    const uint32_t old_transaction = 0x52000000u | seed;
    state.lsu.ldq[0].generation = 0xffffu;
    MicroOp old_uop = pilot_uop(3, old_allocation, 0, false);
    pilot_install(state, old_uop);
    bool old_accepted = boom::lsu_accept_completion(
        state, old_uop, true, false, false, 0x2000, 0, 0xff, 3);
    const uint8_t slot = state.rob.entries[3].uop.queue.ldq_idx;
    const uint16_t old_generation = state.rob.entries[3].uop.queue.ldq_generation;
    state.lsu.ldq[slot].transaction_id = old_transaction;
    state.lsu.ldq[slot].response_pending = true;
    state.lsu.load_response_pending = true;
    state.lsu.pending_load_transaction_id = old_transaction;
    state.lsu.pending_load_rob_idx = 3;
    state.lsu.pending_load_allocation_id = old_allocation;
    state.lsu.pending_load_lq_index = (LqIndex)slot;
    state.lsu.pending_load_lq_generation = old_generation;
    state.global_flush = true;
    boom::lsu_module(state, pipe);
    state.global_flush = false;
    state.lsu.ldq_tail = (LqIndex)slot;
    MicroOp new_uop = pilot_uop(4, old_allocation + 1, 0, false);
    pilot_install(state, new_uop);
    bool new_accepted = boom::lsu_accept_completion(
        state, new_uop, true, false, false, 0x2008, 0, 0xff, 3);
    DmemResponse response;
    response.transaction_id = old_transaction;
    response.data = 0x1122334455667788ULL;
    CompletionEvent event;
    boom::completion_from_load_response(state, response, event);
    obs0 = old_accepted;
    obs1 = new_accepted;
    obs2 = event.valid;
    obs3 = state.rob.entries[4].uop.queue.ldq_generation;
    obs4 = (uint64_t)state.lsu.ldq_count |
           ((uint64_t)state.lsu.ldq[slot].valid << 8) |
           ((uint64_t)state.lsu.load_response_pending << 9);
    obs5 = state.lsu.ldq[slot].rob_allocation_id;
}

extern "C" void g6_l1r_pilot_branch_squash(
        uint32_t seed, uint64_t& obs0, uint64_t& obs1, uint64_t& obs2,
        uint64_t& obs3, uint64_t& obs4, uint64_t& obs5) {
    PILOT_INTERFACES();
    BoomCoreState state;
    const uint8_t tag = 2;
    const uint8_t branch_mask = 1u << tag;
    MicroOp branch = pilot_uop(0, 0x34000000u | seed, 0, false);
    branch.branch.is_br = true;
    branch.branch.br_tag = tag;
    pilot_install(state, branch);
    MicroOp load = pilot_uop(1, 0x34010000u | seed, branch_mask, false);
    pilot_install(state, load);
    bool accepted = boom::lsu_accept_completion(
        state, load, true, false, false, 0x3000, 0, 0xff, 3);
    const uint8_t slot = state.rob.entries[1].uop.queue.ldq_idx;
    LoadQueueEntry& owner = state.lsu.ldq[slot];
    owner.transaction_id = 0x53000000u | seed;
    owner.response_pending = true;
    state.lsu.load_response_pending = true;
    state.lsu.pending_load_transaction_id = owner.transaction_id;
    state.lsu.pending_load_rob_idx = 1;
    state.lsu.pending_load_allocation_id = load.queue.rob_allocation_id;
    state.lsu.pending_load_lq_index = (LqIndex)slot;
    state.lsu.pending_load_lq_generation = owner.generation;
    state.rob.head = 0;
    state.rob.tail = 2;
    state.branch_state.active_mask = branch_mask;
    state.branch_state.tag_valid[tag] = true;
    state.branch_state.snapshot_valid[tag] = true;
    boom::branch_complete_event(state, branch, true, 0x4000);
    obs0 = accepted;
    obs1 = (uint64_t)state.lsu.ldq_count |
           ((uint64_t)state.lsu.ldq[slot].valid << 8);
    obs2 = state.lsu.load_response_pending;
    obs3 = state.rob.entries[1].valid;
    obs4 = (uint64_t)state.brupdate.valid |
           ((uint64_t)state.brupdate.mispredict << 1) |
           ((uint64_t)state.brupdate.mispredict_mask << 8);
    obs5 = state.branch_state.active_mask;
}

extern "C" void g6_l1r_pilot_sq_identity(
        uint32_t seed, uint64_t& obs0, uint64_t& obs1, uint64_t& obs2,
        uint64_t& obs3, uint64_t& obs4, uint64_t& obs5) {
    PILOT_INTERFACES();
    BoomCoreState state;
    const uint32_t allocation = 0x45000000u | seed;
    state.lsu.stq[0].generation = 0xfffeu;
    MicroOp store = pilot_uop(5, allocation, 0, true);
    pilot_install(state, store);
    bool accepted = boom::lsu_accept_completion(
        state, store, false, true, false, 0x5000, seed, 0xff, 3);
    const uint8_t slot = state.rob.entries[5].uop.queue.stq_idx;
    const uint16_t generation = state.rob.entries[5].uop.queue.stq_generation;
    bool exact_match = boom::lsu_store_owner_matches(
        state, (SqIndex)slot, generation, 5, allocation);
    bool stale_match = boom::lsu_store_owner_matches(
        state, (SqIndex)slot, generation + 1, 5, allocation);
    bool bad_reclaim = boom::lsu_reclaim_store(
        state, (SqIndex)slot, generation + 1, 5, allocation);
    const bool valid_after_bad = state.lsu.stq[slot].valid;
    const uint8_t count_after_bad = state.lsu.stq_count;
    bool good_reclaim = boom::lsu_reclaim_store(
        state, (SqIndex)slot, generation, 5, allocation);
    obs0 = accepted;
    obs1 = generation;
    obs2 = (uint64_t)exact_match | ((uint64_t)stale_match << 1);
    obs3 = (uint64_t)bad_reclaim | ((uint64_t)valid_after_bad << 1) |
           ((uint64_t)count_after_bad << 8);
    obs4 = good_reclaim;
    obs5 = (uint64_t)state.lsu.stq[slot].valid |
           ((uint64_t)state.lsu.stq_count << 8);
}
