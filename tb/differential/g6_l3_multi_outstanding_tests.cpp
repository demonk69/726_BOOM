#include "boom_config.hpp"
#include "boom_types.hpp"
#include "boom_state.hpp"
#include "boom_interfaces.hpp"
#include "completion.hpp"

#include <cstdio>
#include <cstdint>

namespace boom {
void lsu_module(BoomCoreState& state, PipeSignals& pipe);
void completion_service_execute(BoomCoreState& state);
void completion_service_cycle(BoomCoreState& state, PipeSignals& pipe);
}

struct ModelOwner {
    bool active;
    uint32_t txid;
    uint8_t lq_idx;
    uint16_t lq_generation;
    uint8_t rob_idx;
    uint32_t allocation_id;
    uint8_t pdst;
    uint64_t value;
    ModelOwner() : active(false), txid(0), lq_idx(0), lq_generation(0),
        rob_idx(0), allocation_id(0), pdst(0), value(0) {}
};

static int failures = 0;
static int directed_checks = 0;
static int exhaustive_states = 0;
static int random_checks = 0;

static void check(bool condition, const char* name) {
    directed_checks++;
    if (!condition) {
        failures++;
        std::printf("CHECK_FAIL %s\n", name);
    }
}

static uint32_t xorshift32(uint32_t& x) {
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return x;
}

static int popcount_active(const BoomCoreState& state) {
    int count = 0;
    for (int i = 0; i < LQ_DEPTH; i++) {
        const LoadQueueEntry& owner = state.lsu.ldq[i];
        if (owner.valid && owner.response_pending) count++;
    }
    return count;
}

static bool txids_unique(const BoomCoreState& state) {
    for (int i = 0; i < LQ_DEPTH; i++) {
        const LoadQueueEntry& a = state.lsu.ldq[i];
        if (!a.valid || !a.response_pending) continue;
        for (int j = i + 1; j < LQ_DEPTH; j++) {
            const LoadQueueEntry& b = state.lsu.ldq[j];
            if (b.valid && b.response_pending && a.transaction_id == b.transaction_id)
                return false;
        }
    }
    return true;
}

static void check_invariants(const BoomCoreState& state, const char* name) {
    char buffer[160];
    std::snprintf(buffer, sizeof(buffer), "%s_count", name);
    check((int)state.lsu.outstanding_load_count == popcount_active(state), buffer);
    std::snprintf(buffer, sizeof(buffer), "%s_summary", name);
    check(state.lsu.load_response_pending == (popcount_active(state) != 0), buffer);
    std::snprintf(buffer, sizeof(buffer), "%s_unique", name);
    check(txids_unique(state), buffer);
    for (int i = 0; i < LQ_DEPTH; i++) {
        const LoadQueueEntry& owner = state.lsu.ldq[i];
        if (!owner.valid || !owner.response_pending) continue;
        std::snprintf(buffer, sizeof(buffer), "%s_owner_valid_%d", name, i);
        check(owner.rob_idx < ROB_DEPTH && state.rob.entries[owner.rob_idx].valid, buffer);
        const RobEntry& entry = state.rob.entries[owner.rob_idx];
        std::snprintf(buffer, sizeof(buffer), "%s_owner_tuple_%d", name, i);
        check(entry.uop.queue.rob_allocation_id == owner.rob_allocation_id &&
              entry.uop.queue.ldq_idx == i &&
              entry.uop.queue.ldq_generation == owner.generation &&
              entry.memory_transaction_id == owner.transaction_id &&
              entry.memory_request_sent, buffer);
    }
}

static void seed_load(BoomCoreState& state, uint8_t rob_idx, uint8_t lq_idx,
                      uint32_t allocation_id, uint8_t pdst, uint64_t address,
                      uint8_t branch_mask = 0) {
    LoadQueueEntry& lq = state.lsu.ldq[lq_idx];
    uint16_t generation = (uint16_t)(lq.generation + 1);
    lq = LoadQueueEntry();
    lq.valid = true;
    lq.generation = generation;
    lq.rob_idx = rob_idx;
    lq.rob_allocation_id = allocation_id;
    lq.address = address;
    lq.size = 3;
    lq.signed_load = false;
    lq.branch_mask = branch_mask;

    RobEntry& entry = state.rob.entries[rob_idx];
    entry = RobEntry();
    entry.valid = true;
    entry.busy = true;
    entry.memory_valid = true;
    entry.is_load = true;
    entry.signed_load = false;
    entry.memory_address = address;
    entry.memory_mask = 0xff;
    entry.memory_size = 3;
    entry.uop.uopc = 1;
    entry.uop.ctrl.is_load = true;
    entry.uop.queue.rob_idx = rob_idx;
    entry.uop.queue.rob_allocation_id = allocation_id;
    entry.uop.queue.ldq_idx = lq_idx;
    entry.uop.queue.ldq_generation = generation;
    entry.uop.rename.dst_rtype = DST_INT;
    entry.uop.rename.pdst = pdst;
    entry.uop.branch.br_mask = branch_mask;

    if (!lq.response_pending) state.lsu.ldq_count++;
    state.lsu.ldq_tail = (LqIndex)((lq_idx + 1) % LQ_DEPTH);
    state.rob.tail = (uint8_t)((rob_idx + 1) % ROB_DEPTH);
}

static bool issue_one(BoomCoreState& state, PipeSignals& pipe,
                      ModelOwner& model, const char* name) {
    boom::lsu_module(state, pipe);
    if (pipe.dmem_req.empty()) {
        check(false, name);
        return false;
    }
    DmemRequest req = pipe.dmem_req.read();
    uint8_t lq_idx = state.rob.entries[req.rob_idx].uop.queue.ldq_idx;
    const LoadQueueEntry& owner = state.lsu.ldq[lq_idx];
    model.active = true;
    model.txid = req.transaction_id;
    model.lq_idx = lq_idx;
    model.lq_generation = owner.generation;
    model.rob_idx = req.rob_idx;
    model.allocation_id = owner.rob_allocation_id;
    model.pdst = state.rob.entries[req.rob_idx].uop.rename.pdst;
    model.value = 0x100000000ull + req.transaction_id;
    check(req.command == DMEM_LOAD && !req.is_store, name);
    return true;
}

static bool response_event_valid(const BoomCoreState& state, uint32_t txid) {
    DmemResponse response;
    response.transaction_id = txid;
    response.data = response.read_data = 0x55aa55aa55aa55aaull;
    CompletionEvent event;
    boom::completion_from_load_response(state, response, event);
    return event.valid;
}

static bool complete_response(BoomCoreState& state, const ModelOwner& owner,
                              const char* name) {
    char buffer[160];
    DmemResponse response;
    response.transaction_id = owner.txid;
    response.data = response.read_data = owner.value;
    state.completion.load_response = CompletionEvent();
    boom::completion_from_load_response(state, response,
                                        state.completion.load_response);
    if (!state.completion.load_response.valid) {
        std::snprintf(buffer, sizeof(buffer), "%s_event", name);
        check(false, buffer);
        return false;
    }
    boom::completion_service_execute(state);
    std::snprintf(buffer, sizeof(buffer), "%s_hold_clear", name);
    check(!state.completion.load_response.valid, buffer);
    std::snprintf(buffer, sizeof(buffer), "%s_lq_reclaim", name);
    check(!state.lsu.ldq[owner.lq_idx].valid, buffer);
    std::snprintf(buffer, sizeof(buffer), "%s_prf", name);
    check(boom::prf_read(state, owner.pdst) == owner.value, buffer);
    return true;
}

static void kill_owner(BoomCoreState& state, ModelOwner& owner) {
    if (!owner.active) return;
    LoadQueueEntry& lq = state.lsu.ldq[owner.lq_idx];
    if (lq.valid && lq.response_pending && state.lsu.outstanding_load_count != 0)
        state.lsu.outstanding_load_count--;
    lq.valid = false;
    lq.response_pending = false;
    lq.transaction_id = 0;
    if (state.lsu.ldq_count != 0) state.lsu.ldq_count--;
    state.rob.entries[owner.rob_idx] = RobEntry();
    state.lsu.load_response_pending = state.lsu.outstanding_load_count != 0;
    owner.active = false;
}

static void run_issue_response_permutations() {
    BoomCoreState state;
    PipeSignals pipe;
    ModelOwner owners[3];
    for (int i = 0; i < 3; i++)
        seed_load(state, (uint8_t)i, (uint8_t)i, 100 + i, 10 + i,
                  0x80000000ull + 8ull * i);
    issue_one(state, pipe, owners[0], "issue_a");
    issue_one(state, pipe, owners[1], "issue_b");
    issue_one(state, pipe, owners[2], "issue_c");
    check((int)state.lsu.outstanding_load_count == 3, "three_outstanding");
    check_invariants(state, "three_issue");
    complete_response(state, owners[2], "complete_c_first");
    complete_response(state, owners[0], "complete_a_second");
    complete_response(state, owners[1], "complete_b_third");
    check((int)state.lsu.outstanding_load_count == 0, "three_done");
}

static void run_two_response_orders() {
    for (int reverse = 0; reverse < 2; reverse++) {
        BoomCoreState state;
        PipeSignals pipe;
        ModelOwner owners[2];
        seed_load(state, 0, 0, 200, 20, 0x80001000ull);
        seed_load(state, 1, 1, 201, 21, 0x80001008ull);
        issue_one(state, pipe, owners[0], "two_issue_a");
        issue_one(state, pipe, owners[1], "two_issue_b");
        if (reverse) {
            complete_response(state, owners[1], "two_reverse_b");
            complete_response(state, owners[0], "two_reverse_a");
        } else {
            complete_response(state, owners[0], "two_inorder_a");
            complete_response(state, owners[1], "two_inorder_b");
        }
        check((int)state.lsu.outstanding_load_count == 0, "two_done");
    }
}

static void run_max_outstanding() {
    BoomCoreState state;
    PipeSignals pipe;
    ModelOwner owners[LQ_DEPTH];
    for (int i = 0; i < LQ_DEPTH; i++)
        seed_load(state, (uint8_t)i, (uint8_t)i, 300 + i, 24 + (i % 16),
                  0x80002000ull + 8ull * i);
    for (int i = 0; i < LQ_DEPTH; i++)
        issue_one(state, pipe, owners[i], "max_issue");
    check((int)state.lsu.outstanding_load_count == LQ_DEPTH, "max_count");
    check_invariants(state, "max_active");
    complete_response(state, owners[LQ_DEPTH / 2], "max_free_one");
    ModelOwner extra;
    seed_load(state, (uint8_t)(ROB_DEPTH - 1), owners[LQ_DEPTH / 2].lq_idx,
              900, 40, 0x80003000ull);
    issue_one(state, pipe, extra, "max_reissue_after_free");
    check((int)state.lsu.outstanding_load_count == LQ_DEPTH, "max_refilled");
}

static void run_stale_and_duplicate_cases() {
    BoomCoreState state;
    PipeSignals pipe;
    ModelOwner a, b;
    seed_load(state, 0, 0, 400, 30, 0x80004000ull);
    seed_load(state, 1, 1, 401, 31, 0x80004008ull);
    issue_one(state, pipe, a, "stale_issue_a");
    issue_one(state, pipe, b, "stale_issue_b");
    kill_owner(state, b);
    check(!response_event_valid(state, b.txid), "late_killed_reject");
    complete_response(state, a, "survivor_complete");
    check(!response_event_valid(state, a.txid), "duplicate_reject");

    seed_load(state, 2, 1, 402, 32, 0x80004010ull);
    ModelOwner c;
    issue_one(state, pipe, c, "reuse_issue_c");
    check(!response_event_valid(state, b.txid), "lq_reuse_old_reject");
    complete_response(state, c, "reuse_new_complete");
}

static void run_txid_wrap_cases() {
    BoomCoreState state;
    PipeSignals pipe;
    ModelOwner a, b, c;
    state.lsu.next_transaction_id = 0xfffffffeu;
    seed_load(state, 0, 0, 500, 33, 0x80005000ull);
    seed_load(state, 1, 1, 501, 34, 0x80005008ull);
    seed_load(state, 2, 2, 502, 35, 0x80005010ull);
    issue_one(state, pipe, a, "wrap_issue_a");
    issue_one(state, pipe, b, "wrap_issue_b");
    issue_one(state, pipe, c, "wrap_issue_c");
    check(a.txid == 0xfffffffeu, "wrap_tx_fe");
    check(b.txid == 0xffffffffu, "wrap_tx_ff");
    check(c.txid == 0u, "wrap_tx_00");
    complete_response(state, c, "wrap_complete_zero_first");
    complete_response(state, a, "wrap_complete_a");
    complete_response(state, b, "wrap_complete_b");

    BoomCoreState skip_state;
    PipeSignals skip_pipe;
    ModelOwner first, second;
    skip_state.lsu.next_transaction_id = 42;
    seed_load(skip_state, 0, 0, 510, 36, 0x80005100ull);
    seed_load(skip_state, 1, 1, 511, 37, 0x80005108ull);
    issue_one(skip_state, skip_pipe, first, "skip_issue_first");
    skip_state.lsu.next_transaction_id = first.txid;
    issue_one(skip_state, skip_pipe, second, "skip_issue_second");
    check(second.txid != first.txid, "active_txid_skip");
}

static void run_forwarded_and_backpressure_cases() {
    BoomCoreState state;
    PipeSignals pipe;

    RobEntry& store_rob = state.rob.entries[0];
    store_rob.valid = true;
    store_rob.memory_valid = true;
    store_rob.is_store = true;
    store_rob.uop.ctrl.is_sta = true;
    store_rob.uop.queue.rob_idx = 0;
    store_rob.uop.queue.rob_allocation_id = 600;
    store_rob.uop.queue.stq_idx = 0;
    store_rob.uop.queue.stq_generation = 1;
    state.lsu.stq_count = 1;
    state.lsu.stq[0].valid = true;
    state.lsu.stq[0].address_valid = true;
    state.lsu.stq[0].data_valid = true;
    state.lsu.stq[0].rob_idx = 0;
    state.lsu.stq[0].rob_allocation_id = 600;
    state.lsu.stq[0].generation = 1;
    state.lsu.stq[0].mask = 0xff;
    state.lsu.stq_address[0] = 0x80006000ull;
    state.lsu.stq_data[0] = 0x123456789abcdef0ull;
    seed_load(state, 1, 0, 601, 38, 0x80006000ull);
    boom::lsu_module(state, pipe);
    check(pipe.dmem_req.empty(), "forward_no_dmem");
    check((int)state.lsu.outstanding_load_count == 0, "forward_no_slot");
    check(state.completion.load_response.valid &&
          state.completion.load_response.forwarded_load, "forward_completion");
    boom::completion_service_execute(state);
    check(!state.lsu.ldq[0].valid, "forward_reclaim");

    BoomCoreState bp_state;
    PipeSignals bp_pipe;
    seed_load(bp_state, 0, 0, 700, 39, 0x80007000ull);
    DmemRequest filler;
    for (int i = 0; i < 1024; i++) bp_pipe.dmem_req.write(filler);
    bool request_full_observable = bp_pipe.dmem_req.full();
    if (request_full_observable) {
        boom::lsu_module(bp_state, bp_pipe);
        check(!bp_state.rob.entries[0].memory_request_sent, "req_bp_no_sent");
        check((int)bp_state.lsu.outstanding_load_count == 0, "req_bp_no_owner");
    }
    while (!bp_pipe.dmem_req.empty()) bp_pipe.dmem_req.read();
    ModelOwner issued;
    issue_one(bp_state, bp_pipe, issued, "req_bp_later_issue");

    BoomCoreState hold_state;
    PipeSignals hold_pipe;
    ModelOwner h0, h1;
    seed_load(hold_state, 0, 0, 710, 41, 0x80007100ull);
    seed_load(hold_state, 1, 1, 711, 42, 0x80007108ull);
    issue_one(hold_state, hold_pipe, h0, "hold_issue_0");
    issue_one(hold_state, hold_pipe, h1, "hold_issue_1");
    DmemResponse r0;
    r0.transaction_id = h0.txid;
    r0.data = r0.read_data = h0.value;
    boom::completion_from_load_response(hold_state, r0,
                                        hold_state.completion.load_response);
    DmemResponse r1;
    r1.transaction_id = h1.txid;
    r1.data = r1.read_data = h1.value;
    hold_pipe.dmem_resp.write(r1);
    boom::completion_service_cycle(hold_state, hold_pipe);
    check(!hold_pipe.dmem_resp.empty(), "hold_keeps_second_response");
    boom::completion_service_cycle(hold_state, hold_pipe);
    check(hold_pipe.dmem_resp.empty(), "hold_drains_second_response");
    check((int)hold_state.lsu.outstanding_load_count == 0, "hold_completes_both");
}

static void run_same_slot_double_issue_case() {
    BoomCoreState state;
    PipeSignals pipe;
    ModelOwner owner;
    seed_load(state, 0, 0, 800, 43, 0x80008000ull);
    issue_one(state, pipe, owner, "double_issue_first");
    boom::lsu_module(state, pipe);
    check(pipe.dmem_req.empty(), "same_lq_no_second_request");
    check((int)state.lsu.outstanding_load_count == 1, "same_lq_count_one");
}

static void run_bounded_exhaustive() {
    if (LQ_DEPTH != 4 || SQ_DEPTH != 4) return;
    for (int mask = 0; mask < (1 << LQ_DEPTH); mask++) {
        for (int resp = 0; resp < LQ_DEPTH; resp++) {
            BoomCoreState state;
            int active = 0;
            for (int i = 0; i < LQ_DEPTH; i++) {
                if ((mask & (1 << i)) == 0) continue;
                seed_load(state, (uint8_t)i, (uint8_t)i, 1000 + i,
                          8 + i, 0x80009000ull + 8ull * i);
                state.rob.entries[i].memory_request_sent = true;
                state.rob.entries[i].memory_transaction_id = 2000 + i;
                state.lsu.ldq[i].response_pending = true;
                state.lsu.ldq[i].transaction_id = 2000 + i;
                active++;
            }
            state.lsu.outstanding_load_count = (LqCount)active;
            state.lsu.load_response_pending = active != 0;
            exhaustive_states++;
            uint32_t txid = 2000 + resp;
            bool expected = (mask & (1 << resp)) != 0;
            bool actual = response_event_valid(state, txid);
            check(actual == expected, "exhaustive_response_match");
            check_invariants(state, "exhaustive_invariant");
        }
    }
}

static void compare_model(const BoomCoreState& state, const ModelOwner model[LQ_DEPTH],
                          const char* name) {
    int model_count = 0;
    for (int i = 0; i < LQ_DEPTH; i++) {
        if (model[i].active) model_count++;
        char buffer[160];
        std::snprintf(buffer, sizeof(buffer), "%s_model_%d", name, i);
        check(model[i].active ==
              (state.lsu.ldq[i].valid && state.lsu.ldq[i].response_pending), buffer);
        if (!model[i].active) continue;
        const LoadQueueEntry& owner = state.lsu.ldq[i];
        std::snprintf(buffer, sizeof(buffer), "%s_tuple_%d", name, i);
        check(owner.transaction_id == model[i].txid &&
              owner.generation == model[i].lq_generation &&
              owner.rob_idx == model[i].rob_idx &&
              owner.rob_allocation_id == model[i].allocation_id, buffer);
    }
    check(model_count == (int)state.lsu.outstanding_load_count, name);
}

static void run_random_differential() {
    const int seeds = (LQ_DEPTH == 8 && SQ_DEPTH == 8) ? 256 : 32;
    const int cycles = (LQ_DEPTH == 8 && SQ_DEPTH == 8) ? 8192 : 1024;
    for (int seed = 0; seed < seeds; seed++) {
        BoomCoreState state;
        PipeSignals pipe;
        ModelOwner model[LQ_DEPTH];
        uint32_t rng = 0x9e3779b9u ^ (uint32_t)(seed * 0x1020304u);
        uint32_t allocation = 3000 + seed * 100;
        if ((seed & 31) == 0) state.lsu.next_transaction_id = 0xfffffff0u;
        for (int cycle = 0; cycle < cycles; cycle++) {
            uint32_t r = xorshift32(rng);
            int op = (int)(r % 8);
            if (op <= 2) {
                int slot = -1;
                for (int i = 0; i < LQ_DEPTH; i++) {
                    if (!model[i].active && !state.lsu.ldq[i].valid) {
                        slot = i;
                        break;
                    }
                }
                if (slot >= 0) {
                    uint8_t rob = (uint8_t)slot;
                    seed_load(state, rob, (uint8_t)slot, allocation++,
                              (uint8_t)(1 + slot),
                              0x8000a000ull + 8ull * (uint64_t)(cycle + slot));
                    ModelOwner issued;
                    if (issue_one(state, pipe, issued, "random_issue"))
                        model[issued.lq_idx] = issued;
                }
            } else if (op <= 4) {
                int slot = (int)((r >> 8) & (LQ_DEPTH - 1));
                if (model[slot].active) {
                    ModelOwner owner = model[slot];
                    complete_response(state, owner, "random_response");
                    model[slot].active = false;
                }
            } else if (op == 5) {
                int slot = (int)((r >> 8) & (LQ_DEPTH - 1));
                if (model[slot].active) {
                    uint32_t txid = model[slot].txid;
                    kill_owner(state, model[slot]);
                    check(!response_event_valid(state, txid), "random_late_kill");
                }
            } else if (op == 6) {
                int slot = (int)((r >> 8) & (LQ_DEPTH - 1));
                if (model[slot].active) {
                    uint32_t txid = model[slot].txid ^ 0x100u;
                    check(!response_event_valid(state, txid), "random_tx_mismatch");
                }
            } else if (popcount_active(state) == 0) {
                state.lsu.next_transaction_id = 0xfffffffcu + (r & 3u);
            }
            compare_model(state, model, "random_model");
            check_invariants(state, "random_inv");
            random_checks++;
        }
    }
}

int main() {
    std::printf("G6_L3_CONFIG LQ=%d SQ=%d\n", LQ_DEPTH, SQ_DEPTH);
    run_two_response_orders();
    run_issue_response_permutations();
    run_max_outstanding();
    run_stale_and_duplicate_cases();
    run_txid_wrap_cases();
    run_forwarded_and_backpressure_cases();
    run_same_slot_double_issue_case();
    run_bounded_exhaustive();
    run_random_differential();
    std::printf("G6_L3_COUNTS directed_checks=%d exhaustive_states=%d random_checks=%d failures=%d\n",
                directed_checks, exhaustive_states, random_checks, failures);
    if (failures == 0) {
        std::printf("G6_L3_MULTI_OUTSTANDING_PASS LQ=%d SQ=%d failures=0\n",
                    LQ_DEPTH, SQ_DEPTH);
        return 0;
    }
    std::printf("G6_L3_MULTI_OUTSTANDING_FAIL LQ=%d SQ=%d failures=%d\n",
                LQ_DEPTH, SQ_DEPTH, failures);
    return 1;
}
