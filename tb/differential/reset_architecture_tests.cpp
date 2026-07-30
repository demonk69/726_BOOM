#include "boom_interfaces.hpp"
#include "reset.hpp"
#include <cstdio>

extern void boom_core_step(BoomCoreState& state, PipeSignals& pipe);

static int tests_passed = 0;
static int tests_failed = 0;

#ifdef BOOM_HLS_GATE3_10_R1_RESET_ROB_PIPELINE
static const int EXPECTED_RESET_STEPS = 114;
#else
static const int EXPECTED_RESET_STEPS = 145;
#endif

#define TEST(name) std::printf("  [RESET] %-58s ... ", name)
#define PASS() do { std::printf("PASS\n"); tests_passed++; } while (0)
#define FAIL(message) do { std::printf("FAIL: %s\n", message); tests_failed++; return; } while (0)
#define CHECK(condition, message) do { if (!(condition)) FAIL(message); } while (0)

static int run_reset(BoomCoreState& state, ResetControllerState& reset_ctrl) {
    int steps = 0;
    while (!reset_ctrl.completed && steps < 200) {
        boom_core_reset_step(state, reset_ctrl);
        steps++;
    }
    return steps;
}

static void dirty_state(BoomCoreState& state) {
    state.frontend.pc = 0x80000000ull;
    state.frontend.reset_done = true;
    state.frontend.request_sent = true;
    state.frontend.response_received = true;
    state.frontend.fetch_packet_valid = true;
    state.frontend.fetch_id = 99;
    state.decode.dec_valids[0] = true;
    state.rename.renamed_valids[0] = true;
    state.issue.issued_valids[0] = true;
    state.execute.alu_results[0].valid = true;
    state.execute.alu_results[0].mispredict = true;
    state.rob.head = 3;
    state.rob.tail = 7;
    state.rob.maybe_full = true;
    state.rob.commit_valid = true;
    state.rob.entries[3].valid = true;
    state.rob.entries[3].busy = true;
    state.issue.alu_iq.head = 2;
    state.issue.alu_iq.tail = 6;
    state.issue.alu_iq.count = 4;
    state.issue.alu_iq.entries[2].valid = true;
    state.branch_state.active_mask = 0xff;
    state.branch_state.tag_valid[3] = true;
    state.branch_state.snapshot_valid[3] = true;
    state.rename.int_map_table.map_table[5] = 17;
    state.rename.int_map_table.committed_map_table[5] = 17;
    state.rename.int_free_list.head = 9;
    state.rename.int_free_list.count = 3;
    state.rename.int_free_list.busy_table[17] = true;
    state.lsu.ldq_count = 1;
    state.lsu.ldq[0].valid = true;
    state.lsu.ldq[0].response_pending = true;
    state.lsu.stq_count = 1;
    state.lsu.stq[0].valid = true;
    state.lsu.stq[0].committed = true;
    state.lsu.stq[0].issued_to_memory = true;
    state.lsu.load_response_pending = true;
    state.lsu.next_transaction_id = 77;
    state.csr.cycle = 1234;
    state.csr.instret = 99;
    state.io_success = true;
    state.io_trap = true;
    state.tohost = 1;
}

static void t_empty_reset() {
    TEST("empty core reset completes in the specified steps");
    BoomCoreState state;
    ResetControllerState ctrl;
    CHECK(run_reset(state, ctrl) == EXPECTED_RESET_STEPS, "unexpected initialization length");
    CHECK(ctrl.completed, "reset did not complete");
    CHECK(state.frontend.pc == RESET_VECTOR, "wrong reset vector");
    PASS();
}

static void t_rob_nonempty_reset() {
    TEST("nonempty ROB becomes logically empty");
    BoomCoreState state;
    dirty_state(state);
    ResetControllerState ctrl;
    run_reset(state, ctrl);
    CHECK(state.rob.head == 0 && state.rob.tail == 0 && !state.rob.maybe_full, "ROB metadata retained");
    for (int i = 0; i < ROB_DEPTH; i++) CHECK(!state.rob.entries[i].valid, "ROB valid retained");
    PASS();
}

static void t_iq_nonempty_reset() {
    TEST("nonempty IQ and issued slots become invalid");
    BoomCoreState state;
    dirty_state(state);
    ResetControllerState ctrl;
    run_reset(state, ctrl);
    CHECK(state.issue.alu_iq.count == 0, "IQ count retained");
    for (int i = 0; i < ISSUE_QUEUE_ALU_DEPTH; i++) CHECK(!state.issue.alu_iq.entries[i].valid, "IQ valid retained");
    for (int i = 0; i < ISSUE_WIDTH; i++) CHECK(!state.issue.issued_valids[i], "issued valid retained");
    PASS();
}

static void t_pending_load_reset() {
    TEST("pending load transaction becomes invisible");
    BoomCoreState state;
    dirty_state(state);
    ResetControllerState ctrl;
    run_reset(state, ctrl);
    CHECK(state.lsu.ldq_count == 0 && !state.lsu.load_response_pending, "load pending retained");
    for (int i = 0; i < LDQ_DEPTH; i++) CHECK(!state.lsu.ldq[i].valid, "LDQ valid retained");
    PASS();
}

static void t_pending_store_reset() {
    TEST("pending store transaction becomes invisible");
    BoomCoreState state;
    dirty_state(state);
    ResetControllerState ctrl;
    run_reset(state, ctrl);
    CHECK(state.lsu.stq_count == 0, "store count retained");
    for (int i = 0; i < STQ_DEPTH; i++) CHECK(!state.lsu.stq[i].valid, "STQ valid retained");
    PASS();
}

static void t_branch_recovery_reset() {
    TEST("branch recovery state releases every tag");
    BoomCoreState state;
    dirty_state(state);
    ResetControllerState ctrl;
    run_reset(state, ctrl);
    CHECK(state.branch_state.active_mask == 0, "active branch mask retained");
    for (int i = 0; i < MAX_BRANCH_COUNT; i++) {
        CHECK(!state.branch_state.tag_valid[i], "branch tag retained");
        CHECK(!state.branch_state.snapshot_valid[i], "branch snapshot retained");
    }
    CHECK(!state.brupdate.valid && !state.brupdate.mispredict, "branch update retained");
    PASS();
}

static void t_trace_stalled_reset() {
    TEST("pending commit/trace ownership is cleared");
    BoomCoreState state;
    dirty_state(state);
    ResetControllerState ctrl;
    run_reset(state, ctrl);
    CHECK(!state.rob.commit_valid, "commit valid retained");
    CHECK(!state.io_success && !state.io_trap && state.tohost == 0, "terminal output retained");
    PASS();
}

static void t_double_runtime_reset() {
    TEST("two consecutive runtime reset sequences complete");
    BoomCoreState state;
    dirty_state(state);
    ResetControllerState first;
    CHECK(run_reset(state, first) == EXPECTED_RESET_STEPS, "first reset length mismatch");
    dirty_state(state);
    ResetControllerState second;
    CHECK(run_reset(state, second) == EXPECTED_RESET_STEPS, "second reset length mismatch");
    CHECK(state.frontend.pc == RESET_VECTOR && state.rob.head == state.rob.tail, "second reset failed");
    PASS();
}

static uint32_t addi(unsigned imm, unsigned rs1, unsigned rd) {
    return ((imm & 0xfff) << 20) | (rs1 << 15) | (rd << 7) | 0x13;
}

static void t_restart_program() {
    TEST("reset core restarts and commits a complete program");
    BoomCoreState state;
    dirty_state(state);
    ResetControllerState ctrl;
    run_reset(state, ctrl);
    PipeSignals pipe;
    int commits = 0;
    for (int cycle = 0; cycle < 100 && !state.io_success; cycle++) {
        if (!pipe.imem_req.empty()) {
            ImemRequest request = pipe.imem_req.read();
            ImemResponse response;
            response.address = request.address;
            response.fetch_id = request.fetch_id;
            response.instruction = request.address == RESET_VECTOR ? addi(5, 0, 1) : 0x00000073;
            pipe.imem_resp.write(response);
        }
        boom_core_step(state, pipe);
        while (!pipe.commit_trace.empty()) {
            pipe.commit_trace.read();
            commits++;
        }
    }
    CHECK(commits >= 1, "no post-reset commit");
    CHECK(state.io_success, "program did not reach success");
    PASS();
}

static void t_free_list_integrity() {
    TEST("free list restores physical registers 1 through 51");
    BoomCoreState state;
    dirty_state(state);
    ResetControllerState ctrl;
    run_reset(state, ctrl);
    CHECK(state.rename.int_free_list.head == 1 && state.rename.int_free_list.count == INT_PHYS_REGS - 1,
          "free-list metadata wrong");
    for (int i = 0; i < INT_PHYS_REGS; i++) {
        CHECK(state.rename.int_free_list.free_list[i] == i, "free-list entry wrong");
        CHECK(!state.rename.int_free_list.busy_table[i], "busy bit retained");
    }
    PASS();
}

static void t_map_table_consistency() {
    TEST("speculative and committed maps both restore to p0");
    BoomCoreState state;
    dirty_state(state);
    ResetControllerState ctrl;
    run_reset(state, ctrl);
    for (int i = 0; i < LOGICAL_REG_COUNT; i++) {
        CHECK(state.rename.int_map_table.map_table[i] == 0, "speculative map retained");
        CHECK(state.rename.int_map_table.committed_map_table[i] == 0, "committed map retained");
    }
    PASS();
}

static void t_branch_tag_no_leak() {
    TEST("new branch allocation starts with an empty tag pool");
    BoomCoreState state;
    dirty_state(state);
    ResetControllerState ctrl;
    run_reset(state, ctrl);
    CHECK(state.branch_state.allocations == 0 && state.branch_state.releases == 0, "branch counters retained");
    CHECK(state.branch_state.mispredicts == 0 && state.branch_state.rollbacks == 0, "recovery counters retained");
    PASS();
}

static void t_no_old_writeback() {
    TEST("pre-reset execute result cannot write back");
    BoomCoreState state;
    dirty_state(state);
    state.execute.alu_results[0].uop.rename.pdst = 9;
    state.execute.alu_results[0].result = 0xdeadbeef;
    ResetControllerState ctrl;
    run_reset(state, ctrl);
    PipeSignals pipe;
    boom_core_step(state, pipe);
    CHECK(state.int_rf[9] != 0xdeadbeef, "stale execute result wrote back");
    PASS();
}

static void t_no_old_memory_side_effect() {
    TEST("pre-reset LSU state emits no memory request");
    BoomCoreState state;
    dirty_state(state);
    ResetControllerState ctrl;
    run_reset(state, ctrl);
    PipeSignals pipe;
    boom_core_step(state, pipe);
    CHECK(pipe.dmem_req.empty(), "stale memory request emitted");
    CHECK(state.tohost == 0 && !state.io_success, "stale tohost side effect retained");
    PASS();
}

int main() {
    std::printf("=== Gate 3.9 Fine-Grain Reset Architecture Tests ===\n");
    t_empty_reset();
    t_rob_nonempty_reset();
    t_iq_nonempty_reset();
    t_pending_load_reset();
    t_pending_store_reset();
    t_branch_recovery_reset();
    t_trace_stalled_reset();
    t_double_runtime_reset();
    t_restart_program();
    t_free_list_integrity();
    t_map_table_consistency();
    t_branch_tag_no_leak();
    t_no_old_writeback();
    t_no_old_memory_side_effect();
    std::printf("\n=== %d passed, %d failed ===\n", tests_passed, tests_failed);
    return tests_failed == 0 ? 0 : 1;
}
