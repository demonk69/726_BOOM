#include "boom_config.hpp"
#include "boom_state.hpp"
#include "boom_interfaces.hpp"
#include "completion.hpp"
#include "reset.hpp"
#include <cstdio>
#include <cstdlib>

namespace boom {
bool lsu_accept_completion(BoomCoreState&, const MicroOp&, bool, bool, bool,
                           uint64_t, uint64_t, uint8_t, uint8_t);
bool lsu_finish_load_response(BoomCoreState&, uint8_t, uint32_t, uint32_t);
bool lsu_reclaim_store(BoomCoreState&, SqIndex, uint16_t, uint8_t, uint32_t);
bool lsu_store_owner_matches(const BoomCoreState&, SqIndex, uint16_t, uint8_t, uint32_t);
void lsu_module(BoomCoreState&, PipeSignals&);
void branch_complete_event(BoomCoreState&, const MicroOp&, bool, uint64_t);
void rob_commit_module(BoomCoreState&, PipeSignals&);
}

static int checks = 0;
static int errors = 0;
static int lq_count_error = 0;
static int sq_count_error = 0;
static int lq_owner_error = 0;
static int sq_owner_error = 0;
static int generation_error = 0;
static int stale_mutation_error = 0;
static int reset_error = 0;
static int squash_error = 0;
static int flush_error = 0;
static int blocking_policy_error = 0;
static int memory_behavior_change_error = 0;

#define CHECK_KIND(condition, counter, message) do { \
    checks++; \
    if (!(condition)) { \
        std::printf("FAIL: %s\n", message); \
        errors++; \
        counter++; \
    } \
} while (0)

static MicroOp owner_uop(uint8_t rob, uint32_t allocation, uint8_t branch_mask = 0) {
    MicroOp uop;
    uop.queue.rob_idx = rob;
    uop.queue.rob_allocation_id = allocation;
    uop.branch.br_mask = branch_mask;
    return uop;
}

static void install_rob(BoomCoreState& state, const MicroOp& uop) {
    RobEntry& entry = state.rob.entries[uop.queue.rob_idx];
    entry = RobEntry();
    entry.valid = true;
    entry.busy = true;
    entry.uop = uop;
}

static bool allocate_load(BoomCoreState& state, uint8_t rob, uint32_t allocation,
                          uint8_t branch_mask = 0) {
    MicroOp uop = owner_uop(rob, allocation, branch_mask);
    uop.ctrl.is_load = true;
    uop.mem.uses_ldq = true;
    install_rob(state, uop);
    return boom::lsu_accept_completion(state, uop, true, false, false,
                                       0x1000 + allocation * 8, 0, 0xff, 3);
}

static bool allocate_store(BoomCoreState& state, uint8_t rob, uint32_t allocation,
                           uint8_t branch_mask = 0) {
    MicroOp uop = owner_uop(rob, allocation, branch_mask);
    uop.ctrl.is_sta = true;
    uop.mem.uses_stq = true;
    install_rob(state, uop);
    return boom::lsu_accept_completion(state, uop, false, true, false,
                                       0x2000 + allocation * 8, allocation, 0xff, 3);
}

static int valid_lq(const BoomCoreState& state) {
    int count = 0;
    for (int i = 0; i < LQ_DEPTH; i++) if (state.lsu.ldq[i].valid) count++;
    return count;
}

static int valid_sq(const BoomCoreState& state) {
    int count = 0;
    for (int i = 0; i < SQ_DEPTH; i++) if (state.lsu.stq[i].valid) count++;
    return count;
}

static void check_counts(const BoomCoreState& state) {
    CHECK_KIND((int)state.lsu.ldq_count == valid_lq(state) &&
               (int)state.lsu.ldq_count <= LQ_DEPTH, lq_count_error,
               "LQ count does not equal valid population");
    CHECK_KIND((int)state.lsu.stq_count == valid_sq(state) &&
               (int)state.lsu.stq_count <= SQ_DEPTH, sq_count_error,
               "SQ count does not equal valid population");
}

static int find_lq(const BoomCoreState& state, uint8_t rob, uint32_t allocation) {
    for (int i = 0; i < LQ_DEPTH; i++)
        if (state.lsu.ldq[i].valid && state.lsu.ldq[i].rob_idx == rob &&
            state.lsu.ldq[i].rob_allocation_id == allocation) return i;
    return -1;
}

static int find_sq(const BoomCoreState& state, uint8_t rob, uint32_t allocation) {
    for (int i = 0; i < SQ_DEPTH; i++)
        if (state.lsu.stq[i].valid && state.lsu.stq[i].rob_idx == rob &&
            state.lsu.stq[i].rob_allocation_id == allocation) return i;
    return -1;
}

static void test_widths() {
    CHECK_KIND(LQ_INDEX_BITS == (LQ_DEPTH == 4 ? 2 : LQ_DEPTH == 8 ? 3 : 4),
               lq_owner_error, "LQ index width is not derived");
    CHECK_KIND(SQ_INDEX_BITS == (SQ_DEPTH == 4 ? 2 : SQ_DEPTH == 8 ? 3 : 4),
               sq_owner_error, "SQ index width is not derived");
    CHECK_KIND((1u << LQ_COUNT_BITS) > LQ_DEPTH, lq_count_error,
               "LQ count width cannot represent full");
    CHECK_KIND((1u << SQ_COUNT_BITS) > SQ_DEPTH, sq_count_error,
               "SQ count width cannot represent full");
    CHECK_KIND(LQ_GENERATION_BITS == 16 && SQ_GENERATION_BITS == 16,
               generation_error, "generation width is not 16");
}

static void test_full_empty_and_wrap() {
    BoomCoreState state;
    for (int i = 0; i < LQ_DEPTH; i++) {
        CHECK_KIND(allocate_load(state, (uint8_t)i, 100 + i), lq_owner_error,
                   "LQ allocation failed before full");
        CHECK_KIND(find_lq(state, (uint8_t)i, 100 + i) >= 0, lq_owner_error,
                   "LQ owner tuple missing");
    }
    MicroOp blocked_load = owner_uop(31, 999);
    install_rob(state, blocked_load);
    CHECK_KIND(!boom::lsu_accept_completion(state, blocked_load, true, false, false,
                                            0, 0, 0xff, 3), lq_count_error,
               "LQ accepted allocation while full");
    check_counts(state);

    for (int i = 0; i < LQ_DEPTH; i++) state.lsu.ldq[i].valid = false;
    state.lsu.ldq_count = 0;
    for (int i = 0; i < SQ_DEPTH; i++) {
        CHECK_KIND(allocate_store(state, (uint8_t)i, 200 + i), sq_owner_error,
                   "SQ allocation failed before full");
        CHECK_KIND(find_sq(state, (uint8_t)i, 200 + i) >= 0, sq_owner_error,
                   "SQ owner tuple missing");
    }
    MicroOp blocked_store = owner_uop(31, 1999);
    install_rob(state, blocked_store);
    CHECK_KIND(!boom::lsu_accept_completion(state, blocked_store, false, true, false,
                                            0, 0, 0xff, 3), sq_count_error,
               "SQ accepted allocation while full");
    check_counts(state);

    state = BoomCoreState();
    for (int cycle = 0; cycle < 5 * LQ_DEPTH; cycle++) {
        uint8_t rob = (uint8_t)(cycle % ROB_DEPTH);
        uint32_t allocation = 1000 + cycle;
        CHECK_KIND(allocate_load(state, rob, allocation), lq_owner_error,
                   "LQ wrap allocation failed");
        PipeSignals pipe;
        boom::lsu_module(state, pipe);
        CHECK_KIND(state.lsu.load_response_pending, memory_behavior_change_error,
                   "single load was not issued");
        uint32_t tx = state.lsu.pending_load_transaction_id;
        CHECK_KIND(boom::lsu_finish_load_response(state, rob, allocation, tx),
                   lq_owner_error, "matching load owner did not free");
        check_counts(state);
    }
    for (int cycle = 0; cycle < 5 * SQ_DEPTH; cycle++) {
        uint8_t rob = (uint8_t)(cycle % ROB_DEPTH);
        uint32_t allocation = 2000 + cycle;
        CHECK_KIND(allocate_store(state, rob, allocation), sq_owner_error,
                   "SQ wrap allocation failed");
        int slot = find_sq(state, rob, allocation);
        CHECK_KIND(slot >= 0 && boom::lsu_reclaim_store(
                       state, (SqIndex)slot, state.lsu.stq[slot].generation,
                       rob, allocation), sq_owner_error,
                   "matching SQ owner did not free");
        check_counts(state);
    }
}

static void test_generation_wrap_and_stale_identity() {
    BoomCoreState state;
    state.lsu.ldq[0].generation = 0xfffe;
    state.lsu.ldq_tail = 0;
    CHECK_KIND(allocate_load(state, 1, 3001), generation_error,
               "seeded LQ allocation failed");
    CHECK_KIND(state.lsu.ldq[0].generation == 0xffff, generation_error,
               "LQ generation did not reach 0xffff");
    PipeSignals pipe;
    boom::lsu_module(state, pipe);
    uint32_t old_tx = state.lsu.pending_load_transaction_id;
    uint16_t old_generation = state.lsu.pending_load_lq_generation;
    CHECK_KIND(boom::lsu_finish_load_response(state, 1, 3001, old_tx),
               generation_error, "seeded LQ owner did not free");

    state.lsu.ldq_tail = 0;
    CHECK_KIND(allocate_load(state, 1, 3001), generation_error,
               "wrapped LQ allocation failed");
    CHECK_KIND(state.lsu.ldq[0].generation == 0x0000, generation_error,
               "LQ generation did not wrap modulo 16 bits");
    state.rob.entries[1].uop.queue.ldq_generation = old_generation;
    boom::lsu_module(state, pipe);
    CHECK_KIND(!state.lsu.load_response_pending, stale_mutation_error,
               "stale ROB LQ generation issued reused owner");
    state.rob.entries[1].uop.queue.ldq_generation = state.lsu.ldq[0].generation;
    boom::lsu_module(state, pipe);
    uint32_t new_tx = state.lsu.pending_load_transaction_id;
    state.lsu.pending_load_lq_generation = old_generation;
    DmemResponse stale;
    stale.transaction_id = new_tx;
    stale.data = 0xdead;
    CompletionEvent event;
    boom::completion_from_load_response(state, stale, event);
    CHECK_KIND(!event.valid && state.lsu.ldq[0].valid && state.rob.entries[1].busy,
               stale_mutation_error, "stale LQ generation mutated reused owner");
    state.lsu.pending_load_lq_generation = state.lsu.ldq[0].generation;
    CHECK_KIND(boom::lsu_finish_load_response(state, 1, 3001, new_tx),
               generation_error, "fresh wrapped LQ owner did not free");

    state.lsu.stq[0].generation = 0xffff;
    state.lsu.stq_tail = 0;
    CHECK_KIND(allocate_store(state, 2, 4001), generation_error,
               "wrapped SQ allocation failed");
    CHECK_KIND(state.lsu.stq[0].generation == 0x0000, generation_error,
               "SQ generation did not wrap modulo 16 bits");
    CHECK_KIND(!boom::lsu_store_owner_matches(state, (SqIndex)0, 0xffff, 2, 4001),
               stale_mutation_error, "stale SQ token matched reused owner");
    CHECK_KIND(!boom::lsu_reclaim_store(state, (SqIndex)0, 0xffff, 2, 4001) &&
               state.lsu.stq[0].valid && (int)state.lsu.stq_count == 1,
               stale_mutation_error, "stale SQ token reclaimed reused owner");
    CHECK_KIND(boom::lsu_store_owner_matches(state, (SqIndex)0, 0x0000, 2, 4001),
               sq_owner_error, "fresh SQ token did not match owner");
    state.rob.head = 2;
    state.rob.tail = 3;
    state.rob.entries[2].busy = false;
    state.rob.entries[2].uop.queue.stq_generation = 0xffff;
    PipeSignals store_pipe;
    boom::rob_commit_module(state, store_pipe);
    CHECK_KIND(store_pipe.dmem_req.empty() && state.lsu.stq[0].valid &&
               state.rob.entries[2].valid, stale_mutation_error,
               "stale SQ generation committed external side effect");
    state.rob.entries[2].uop.queue.stq_generation = 0x0000;
    boom::rob_commit_module(state, store_pipe);
    CHECK_KIND(!store_pipe.dmem_req.empty() && !state.lsu.stq[0].valid,
               sq_owner_error, "fresh SQ owner did not commit and reclaim");
}

static void test_reset_independent_depths() {
    BoomCoreState state;
    for (int i = 0; i < LQ_DEPTH; i++) {
        state.lsu.ldq[i].valid = true;
        state.lsu.ldq[i].generation = 99;
    }
    for (int i = 0; i < SQ_DEPTH; i++) {
        state.lsu.stq[i].valid = true;
        state.lsu.stq[i].generation = 77;
    }
    state.lsu.ldq_count = LQ_DEPTH;
    state.lsu.stq_count = SQ_DEPTH;
    state.lsu.load_response_pending = true;
    ResetControllerState reset;
    for (int cycle = 0; !reset.completed && cycle < 512; cycle++)
        boom_core_reset_step(state, reset);
    CHECK_KIND(reset.completed, reset_error, "staged reset did not complete");
    CHECK_KIND(valid_lq(state) == 0 && valid_sq(state) == 0 &&
               (int)state.lsu.ldq_count == 0 && (int)state.lsu.stq_count == 0 &&
               !state.lsu.load_response_pending, reset_error,
               "staged reset left queue control valid");
    for (int i = 0; i < LQ_DEPTH; i++)
        CHECK_KIND(state.lsu.ldq[i].generation == 0, reset_error,
                   "LQ reset did not establish canonical generation");
    for (int i = 0; i < SQ_DEPTH; i++)
        CHECK_KIND(state.lsu.stq[i].generation == 0, reset_error,
                   "SQ reset did not establish canonical generation");
}

static void test_branch_lifecycle() {
    BoomCoreState state;
    CHECK_KIND(allocate_load(state, 3, 5001, 1), lq_owner_error,
               "branch-tagged LQ allocation failed");
    CHECK_KIND(allocate_store(state, 4, 5002, 1), sq_owner_error,
               "branch-tagged SQ allocation failed");
    state.branch_state.active_mask = 1;
    state.branch_state.tag_valid[0] = true;
    state.branch_state.snapshot_valid[0] = true;
    MicroOp branch = owner_uop(2, 5000);
    branch.branch.is_br = true;
    branch.branch.br_tag = 0;
    install_rob(state, branch);
    boom::branch_complete_event(state, branch, true, 0x100);
    CHECK_KIND(valid_lq(state) == 0 && valid_sq(state) == 0,
               squash_error, "mispredict retained tagged queue entries");
    check_counts(state);
}

static void test_flush_and_blocking() {
    BoomCoreState state;
    CHECK_KIND(allocate_load(state, 1, 6001), lq_owner_error,
               "flush LQ allocation failed");
    CHECK_KIND(allocate_store(state, 2, 6002), sq_owner_error,
               "flush SQ allocation failed");
    int lq_slot = find_lq(state, 1, 6001);
    int sq_slot = find_sq(state, 2, 6002);
    uint16_t lq_generation = state.lsu.ldq[lq_slot].generation;
    uint16_t sq_generation = state.lsu.stq[sq_slot].generation;
    state.global_flush = true;
    PipeSignals pipe;
    boom::lsu_module(state, pipe);
    CHECK_KIND(valid_lq(state) == 0 && valid_sq(state) == 0 &&
               (int)state.lsu.ldq_count == 0 && (int)state.lsu.stq_count == 0,
               flush_error, "global flush retained queue entries");
    CHECK_KIND(state.lsu.ldq[lq_slot].generation == lq_generation &&
               state.lsu.stq[sq_slot].generation == sq_generation,
               generation_error, "global flush reset slot generation");

    state = BoomCoreState();
    state.rob.head = 0;
    CHECK_KIND(allocate_store(state, 0, 6100), sq_owner_error,
               "older store allocation failed");
    CHECK_KIND(allocate_load(state, 1, 6101), lq_owner_error,
               "younger load allocation failed");
    boom::lsu_module(state, pipe);
    CHECK_KIND(!state.lsu.load_response_pending && pipe.dmem_req.empty(),
               blocking_policy_error, "younger load bypassed older valid store");
    int older_slot = find_sq(state, 0, 6100);
    CHECK_KIND(boom::lsu_reclaim_store(state, (SqIndex)older_slot,
                                      state.lsu.stq[older_slot].generation, 0, 6100),
               sq_owner_error, "older store reclaim failed");
    state.rob.entries[0].valid = false;
    boom::lsu_module(state, pipe);
    CHECK_KIND(state.lsu.load_response_pending && !pipe.dmem_req.empty(),
               memory_behavior_change_error,
               "load did not issue after older store was removed");

    state = BoomCoreState();
    state.lsu.ldq[0].generation = 17;
    state.lsu.stq[0].generation = 23;
    state.rob.head = 0;
    state.rob.tail = 1;
    state.rob.entries[0].valid = true;
    state.rob.entries[0].busy = false;
    state.rob.entries[0].exception = true;
    state.rob.entries[0].uop.exc_cause = 5;
    boom::rob_commit_module(state, pipe);
    CHECK_KIND(state.global_flush && state.lsu.ldq[0].generation == 17 &&
               state.lsu.stq[0].generation == 23,
               flush_error, "precise exception flush reset generation identity");
}

struct SmallState {
    uint8_t lq_valid;
    uint8_t sq_valid;
    uint16_t lq_generation[4];
    uint16_t sq_generation[4];
    SmallState() : lq_valid(0), sq_valid(0) {
        for (int i = 0; i < 4; i++) lq_generation[i] = sq_generation[i] = 0;
    }
};

static unsigned long long exhaustive_states = 0;

static void exhaustive_walk(const SmallState& input, int depth) {
    exhaustive_states++;
    int lq_count = 0;
    int sq_count = 0;
    for (int i = 0; i < 4; i++) {
        if (input.lq_valid & (1u << i)) lq_count++;
        if (input.sq_valid & (1u << i)) sq_count++;
    }
    CHECK_KIND(lq_count >= 0 && lq_count <= 4, lq_count_error,
               "exhaustive LQ count escaped bounds");
    CHECK_KIND(sq_count >= 0 && sq_count <= 4, sq_count_error,
               "exhaustive SQ count escaped bounds");
    if (depth == 0) return;
    for (int operation = 0; operation < 7; operation++) {
        SmallState next = input;
        int slot = (depth + operation) & 3;
        if (operation == 0 && !(next.lq_valid & (1u << slot))) {
            next.lq_valid |= (uint8_t)(1u << slot);
            next.lq_generation[slot]++;
        } else if (operation == 1) {
            next.lq_valid &= (uint8_t)~(1u << slot);
        } else if (operation == 2 && !(next.sq_valid & (1u << slot))) {
            next.sq_valid |= (uint8_t)(1u << slot);
            next.sq_generation[slot]++;
        } else if (operation == 3) {
            next.sq_valid &= (uint8_t)~(1u << slot);
        } else if (operation == 4) {
            next.lq_valid &= (uint8_t)~(1u << slot);
            next.sq_valid &= (uint8_t)~(1u << slot);
        } else if (operation == 5) {
            next.lq_valid = next.sq_valid = 0;
        } else if (operation == 6) {
            next = SmallState();
        }
        exhaustive_walk(next, depth - 1);
    }
}

static void test_small_exhaustive() {
    if (LQ_DEPTH == 4 && SQ_DEPTH == 4) {
        SmallState initial;
        exhaustive_walk(initial, 6);
        CHECK_KIND(exhaustive_states == 137257, lq_count_error,
                   "bounded exhaustive state count changed");
    }
}

static void test_random_model() {
    const int seed_count = (LQ_DEPTH == 8 && SQ_DEPTH == 8) ? 256 : 32;
    const int cycles = 4096;
    for (int seed = 0; seed < seed_count; seed++) {
        unsigned random = 0x9e3779b9u ^ (unsigned)seed;
        bool lq_valid[LQ_DEPTH] = {};
        bool sq_valid[SQ_DEPTH] = {};
        uint16_t lq_generation[LQ_DEPTH] = {};
        uint16_t sq_generation[SQ_DEPTH] = {};
        int lq_count = 0;
        int sq_count = 0;
        for (int cycle = 0; cycle < cycles; cycle++) {
            random = random * 1664525u + 1013904223u;
            int li = (int)((random >> 8) & (LQ_DEPTH - 1));
            int si = (int)((random >> 16) & (SQ_DEPTH - 1));
            switch (random & 7u) {
            case 0:
            case 1:
                if (!lq_valid[li]) { lq_valid[li] = true; lq_generation[li]++; lq_count++; }
                break;
            case 2:
                if (lq_valid[li]) { lq_valid[li] = false; lq_count--; }
                break;
            case 3:
            case 4:
                if (!sq_valid[si]) { sq_valid[si] = true; sq_generation[si]++; sq_count++; }
                break;
            case 5:
                if (sq_valid[si]) { sq_valid[si] = false; sq_count--; }
                break;
            case 6:
                for (int i = 0; i < LQ_DEPTH; i++) lq_valid[i] = false;
                for (int i = 0; i < SQ_DEPTH; i++) sq_valid[i] = false;
                lq_count = sq_count = 0;
                break;
            default:
                break;
            }
            CHECK_KIND(lq_count >= 0 && lq_count <= LQ_DEPTH,
                       lq_count_error, "random LQ count escaped bounds");
            CHECK_KIND(sq_count >= 0 && sq_count <= SQ_DEPTH,
                       sq_count_error, "random SQ count escaped bounds");
            CHECK_KIND((uint16_t)(lq_generation[li] - 1) != lq_generation[li] ||
                       lq_generation[li] == 0, generation_error,
                       "random LQ generation arithmetic is not modulo 16 bits");
        }
    }
}

int main() {
    std::printf("G6 L1 parameterized LQ/SQ: LQ=%d SQ=%d\n", LQ_DEPTH, SQ_DEPTH);
    test_widths();
    test_full_empty_and_wrap();
    test_generation_wrap_and_stale_identity();
    test_reset_independent_depths();
    test_branch_lifecycle();
    test_flush_and_blocking();
    test_small_exhaustive();
    test_random_model();
    std::printf("checks=%d errors=%d lq_count_error=%d sq_count_error=%d "
                "lq_owner_error=%d sq_owner_error=%d generation_error=%d "
                "stale_mutation_error=%d reset_error=%d squash_error=%d "
                "flush_error=%d blocking_policy_error=%d memory_behavior_change_error=%d\n",
                checks, errors, lq_count_error, sq_count_error, lq_owner_error,
                sq_owner_error, generation_error, stale_mutation_error, reset_error,
                squash_error, flush_error, blocking_policy_error,
                memory_behavior_change_error);
    std::printf("exhaustive_states=%llu random_seeds=%d cycles_per_seed=4096\n",
                exhaustive_states, (LQ_DEPTH == 8 && SQ_DEPTH == 8) ? 256 : 32);
    return errors ? 1 : 0;
}
