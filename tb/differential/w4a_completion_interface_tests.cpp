#include "completion.hpp"
#include "reset.hpp"
#include <cstdio>

namespace boom {
void lsu_module(BoomCoreState&, PipeSignals&);
}

static int passed = 0, failed = 0;
#define TEST(n) std::printf("  [W4A completion] %-48s ... ", n)
#define CHECK(c,m) do { if (!(c)) { std::printf("FAIL: %s\n",m); failed++; return; } } while (0)
#define PASS() do { std::printf("PASS\n"); passed++; } while (0)

static void owner(BoomCoreState& state, uint8_t index, uint32_t allocation) {
    state.rob.entries[index] = RobEntry();
    state.rob.entries[index].valid = true;
    state.rob.entries[index].busy = true;
    state.rob.entries[index].uop.queue.rob_idx = index;
    state.rob.entries[index].uop.queue.rob_allocation_id = allocation;
}

static ExecuteState::AluResult result(uint8_t index, uint32_t allocation) {
    ExecuteState::AluResult value;
    value.valid = true;
    value.uop.uopc = 1;
    value.uop.queue.rob_idx = index;
    value.uop.queue.rob_allocation_id = allocation;
    return value;
}

static void seed_load(BoomCoreState& state, uint8_t index,
                      uint32_t allocation, uint32_t transaction) {
    owner(state, index, allocation);
    RobEntry& entry = state.rob.entries[index];
    entry.is_load = true;
    entry.memory_valid = true;
    entry.memory_request_sent = true;
    entry.memory_transaction_id = transaction;
    entry.memory_size = 3;
    entry.uop.rename.pdst = 8;
    entry.uop.rename.dst_rtype = DST_INT;
    state.rename.int_free_list.busy_table[8] = true;
    state.lsu.load_response_pending = true;
    state.lsu.pending_load_transaction_id = transaction;
    state.lsu.pending_load_rob_idx = index;
    state.lsu.pending_load_allocation_id = allocation;
    state.lsu.ldq_count = 1;
    state.lsu.ldq[0].valid = true;
    state.lsu.ldq[0].rob_idx = index;
    state.lsu.ldq[0].rob_allocation_id = allocation;
}

static void t01() {
    TEST("fixed source IDs");
    CHECK(COMPLETION_SOURCE_LSU_LOAD == 0 &&
          COMPLETION_SOURCE_MEM_EXECUTE == 1 &&
          COMPLETION_SOURCE_INT_EXECUTE == 2 &&
          COMPLETION_SOURCE_COUNT == 3, "source contract changed");
    PASS();
}

static void t02() {
    TEST("execute conversion");
    ExecuteState::AluResult r = result(1, 7);
    r.result = 9;
    CompletionEvent e;
    boom::completion_from_execute(r, COMPLETION_SOURCE_INT_EXECUTE, e);
    CHECK(e.valid && e.kind == COMPLETION_EXECUTE &&
          e.source == COMPLETION_SOURCE_INT_EXECUTE && e.value == 9,
          "execute conversion wrong");
    PASS();
}

static void t03() {
    TEST("store conversion");
    ExecuteState::AluResult r = result(1, 7);
    r.is_store = r.memory_valid = true;
    CompletionEvent e;
    boom::completion_from_execute(r, COMPLETION_SOURCE_MEM_EXECUTE, e);
    CHECK(e.kind == COMPLETION_STORE && e.is_store, "store kind wrong");
    PASS();
}

static void t04() {
    TEST("load address conversion");
    ExecuteState::AluResult r = result(1, 7);
    r.is_load = r.memory_valid = true;
    CompletionEvent e;
    boom::completion_from_execute(r, COMPLETION_SOURCE_MEM_EXECUTE, e);
    CHECK(e.kind == COMPLETION_MEMORY_ADDRESS && e.is_load, "load address kind wrong");
    PASS();
}

static void t05() {
    TEST("self-contained branch conversion");
    ExecuteState::AluResult r = result(1, 7);
    r.uop.branch.is_br = true;
    r.uop.branch.br_tag = 3;
    r.uop.branch.br_mask = 5;
    r.mispredict = true;
    r.redirect_pc = 0x200;
    CompletionEvent e;
    boom::completion_from_execute(r, COMPLETION_SOURCE_INT_EXECUTE, e);
    r.uop.branch = BranchInfo();
    r.mispredict = false;
    r.redirect_pc = 0xdead;
    CHECK(e.kind == COMPLETION_BRANCH && e.uop.branch.is_br &&
          e.uop.branch.br_tag == 3 && e.uop.branch.br_mask == 5 &&
          e.mispredict && e.redirect_pc == 0x200,
          "branch metadata was not captured");
    PASS();
}

static void t06() {
    TEST("allocation ownership");
    BoomCoreState s;
    owner(s, 2, 10);
    CompletionEvent e;
    boom::completion_from_execute(result(2, 10),
                                  COMPLETION_SOURCE_INT_EXECUTE, e);
    CHECK(boom::completion_has_rob_owner(s, e) && boom::completion_is_valid(s, e),
          "owner rejected");
    PASS();
}

static void t07() {
    TEST("stale allocation rejected");
    BoomCoreState s;
    owner(s, 2, 10);
    CompletionEvent e;
    boom::completion_from_execute(result(2, 11),
                                  COMPLETION_SOURCE_INT_EXECUTE, e);
    CHECK(!boom::completion_has_rob_owner(s, e) && !boom::completion_is_valid(s, e),
          "stale owner accepted");
    PASS();
}

static void t08() {
    TEST("wrap-safe ROB age selection");
    BoomCoreState s;
    s.rob.head = 31;
    owner(s, 31, 1);
    owner(s, 0, 2);
    CompletionEvent mem_event;
    CompletionEvent int_event;
    boom::completion_from_execute(result(0, 2), COMPLETION_SOURCE_MEM_EXECUTE,
                                  mem_event);
    boom::completion_from_execute(result(31, 1), COMPLETION_SOURCE_INT_EXECUTE,
                                  int_event);
    bool valid = false;
    CHECK(boom::select_completion_serial(s, mem_event, int_event, valid) ==
              COMPLETION_SOURCE_INT_EXECUTE && valid,
          "ROB age selection wrong");
    PASS();
}

static void t09() {
    TEST("source priority tie break");
    BoomCoreState s;
    owner(s, 1, 1);
    CompletionEvent mem_event;
    CompletionEvent int_event;
    boom::completion_from_execute(result(1, 1), COMPLETION_SOURCE_MEM_EXECUTE,
                                  mem_event);
    boom::completion_from_execute(result(1, 1), COMPLETION_SOURCE_INT_EXECUTE,
                                  int_event);
    bool valid = false;
    CHECK(boom::select_completion_serial(s, mem_event, int_event, valid) ==
              COMPLETION_SOURCE_MEM_EXECUTE && valid,
          "source priority wrong");
    PASS();
}

static void t10() {
    TEST("load response unified side effects");
    BoomCoreState s;
    seed_load(s, 1, 31, 7);
    PipeSignals p;
    DmemResponse d;
    d.transaction_id = 7;
    d.data = d.read_data = 0x1234;
    p.dmem_resp.write(d);
    boom::completion_service_cycle(s, p);
    CHECK(!s.rob.entries[1].busy && s.rob.entries[1].memory_completed &&
          boom::prf_read(s,8) == 0x1234 && !s.rename.int_free_list.busy_table[8] &&
          !s.lsu.load_response_pending && s.lsu.ldq_count == 0,
          "load side effects diverged");
    PASS();
}

static void t11() {
    TEST("duplicate event has no side effects");
    BoomCoreState s;
    owner(s, 1, 11);
    ExecuteState::AluResult r = result(1, 11);
    r.uop.rename.pdst = 6;
    r.uop.rename.dst_rtype = DST_INT;
    r.result = 0x1111;
    CompletionEvent e;
    boom::completion_from_execute(r, COMPLETION_SOURCE_INT_EXECUTE, e);
    CHECK(boom::apply_completion(s, e), "first event rejected");
    e.value = 0x2222;
    CHECK(!boom::apply_completion(s, e) && boom::prf_read(s,6) == 0x1111 &&
          !s.rob.entries[1].busy, "duplicate event repeated side effects");
    PASS();
}

static void t12() {
    TEST("stale load allocation does not reclaim");
    BoomCoreState s;
    seed_load(s, 1, 31, 7);
    s.rob.entries[1].uop.queue.rob_allocation_id = 32;
    PipeSignals p;
    DmemResponse d;
    d.transaction_id = 7;
    d.data = d.read_data = 0xaaaa;
    p.dmem_resp.write(d);
    boom::completion_service_cycle(s, p);
    CHECK(s.lsu.load_response_pending && s.lsu.ldq_count == 1 &&
          s.rob.entries[1].busy && boom::prf_read(s,8) == 0,
          "stale allocation changed load ownership");
    PASS();
}

static void t13() {
    TEST("stale load transaction does not reclaim");
    BoomCoreState s;
    seed_load(s, 1, 31, 7);
    PipeSignals p;
    DmemResponse d;
    d.transaction_id = 8;
    d.data = d.read_data = 0xbbbb;
    p.dmem_resp.write(d);
    boom::completion_service_cycle(s, p);
    CHECK(s.lsu.load_response_pending && s.lsu.pending_load_transaction_id == 7 &&
          s.lsu.ldq_count == 1 && s.rob.entries[1].busy && boom::prf_read(s,8) == 0,
          "stale transaction changed load ownership");
    PASS();
}

static void t14() {
    TEST("killed owner rejects held event");
    BoomCoreState s;
    owner(s, 1, 14);
    s.rob.entries[1].valid = false;
    s.execute.alu_results[INT_ISSUE_LANE] = result(1, 14);
    s.execute.alu_results[INT_ISSUE_LANE].uop.rename.pdst = 9;
    s.execute.alu_results[INT_ISSUE_LANE].uop.rename.dst_rtype = DST_INT;
    s.execute.alu_results[INT_ISSUE_LANE].result = 0xcccc;
    boom::completion_service_execute(s);
    CHECK(!s.execute.alu_results[INT_ISSUE_LANE].valid && boom::prf_read(s,9) == 0,
          "killed owner produced side effects");
    PASS();
}

static void t15() {
    TEST("exceptional execute preserves W3 writeback");
    BoomCoreState s;
    owner(s, 1, 15);
    s.rename.int_free_list.busy_table[10] = true;
    ExecuteState::AluResult r = result(1, 15);
    r.exception = true;
    r.exc_cause = 2;
    r.result = 0xdddd;
    r.uop.rename.pdst = 10;
    r.uop.rename.dst_rtype = DST_INT;
    CompletionEvent e;
    boom::completion_from_execute(r, COMPLETION_SOURCE_INT_EXECUTE, e);
    CHECK(boom::apply_completion(s, e) && s.rob.entries[1].exception &&
          s.rob.entries[1].uop.exc_cause == 2 && !s.rob.entries[1].busy &&
          boom::prf_read(s,10) == 0xdddd && !s.rename.int_free_list.busy_table[10],
          "exceptional execute no longer matches W3");
    PASS();
}

static void t16() {
    TEST("reset collision rejects stale response");
    BoomCoreState s;
    seed_load(s, 1, 41, 9);
    PipeSignals p;
    DmemResponse d;
    d.transaction_id = 9;
    d.data = d.read_data = 0xeeee;
    p.dmem_resp.write(d);
    ResetControllerState reset;
    for (int i = 0; i < 256 && !reset.completed; i++) boom_core_reset_step(s, reset);
    boom::completion_service_cycle(s, p);
    CHECK(reset.completed && !s.lsu.load_response_pending && s.lsu.ldq_count == 0 &&
          boom::prf_read(s,8) == 0 && !s.rob.entries[1].valid,
          "response crossed reset ownership");
    PASS();
}

static void t17() {
    TEST("branch recovery precedes branch exception");
    BoomCoreState s;
    s.rob.head = 1;
    s.rob.tail = 3;
    owner(s, 1, 51);
    owner(s, 2, 52);
    s.branch_state.active_mask = 1;
    s.branch_state.tag_valid[0] = true;
    s.branch_state.snapshot_valid[0] = true;
    s.execute.alu_results[MEM_ISSUE_LANE] = result(2, 52);
    s.execute.alu_results[MEM_ISSUE_LANE].exception = true;
    s.execute.alu_results[MEM_ISSUE_LANE].uop.branch.br_mask = 1;
    ExecuteState::AluResult branch = result(1, 51);
    branch.uop.branch.is_br = true;
    branch.uop.branch.br_tag = 0;
    branch.mispredict = true;
    branch.redirect_pc = 0x300;
    branch.exception = true;
    branch.exc_cause = 3;
    CompletionEvent e;
    boom::completion_from_execute(branch, COMPLETION_SOURCE_INT_EXECUTE, e);
    s.execute.alu_results[INT_ISSUE_LANE] = ExecuteState::AluResult();
    CHECK(boom::apply_completion(s, e) && s.brupdate.valid && s.brupdate.mispredict &&
          s.frontend.pc == 0x300 && s.rob.entries[1].exception &&
          s.rob.entries[1].uop.exc_cause == 3 && !s.rob.entries[2].valid &&
          !s.execute.alu_results[MEM_ISSUE_LANE].valid,
          "W3 branch/exception ordering changed");
    PASS();
}

static void t18() {
    TEST("load response owns simultaneous execute cycle");
    BoomCoreState s;
    s.rob.head = 0;
    owner(s, 0, 61);
    s.execute.alu_results[INT_ISSUE_LANE] = result(0, 61);
    s.execute.alu_results[INT_ISSUE_LANE].uop.rename.pdst = 11;
    s.execute.alu_results[INT_ISSUE_LANE].uop.rename.dst_rtype = DST_INT;
    s.execute.alu_results[INT_ISSUE_LANE].result = 0xffff;
    seed_load(s, 1, 62, 10);
    PipeSignals p;
    DmemResponse d;
    d.transaction_id = 10;
    d.data = d.read_data = 0x1234;
    p.dmem_resp.write(d);
    boom::completion_service_cycle(s, p);
    CHECK(!s.rob.entries[1].busy && s.rob.entries[0].busy &&
          s.execute.alu_results[INT_ISSUE_LANE].valid && boom::prf_read(s,8) == 0x1234 &&
          boom::prf_read(s,11) == 0, "load response did not retain W3 cycle ownership");
    boom::completion_service_cycle(s, p);
    CHECK(!s.rob.entries[0].busy && !s.execute.alu_results[INT_ISSUE_LANE].valid &&
          boom::prf_read(s,11) == 0xffff, "held execute did not complete next cycle");
    PASS();
}

static void t19() {
    TEST("exceptional load suppresses PRF and wakeup");
    BoomCoreState s;
    seed_load(s, 1, 71, 12);
    boom::prf_seed(s,8,0x5555);
    PipeSignals p;
    DmemResponse d;
    d.transaction_id = 12;
    d.exception = true;
    d.exception_cause = 5;
    d.data = d.read_data = 0xaaaa;
    p.dmem_resp.write(d);
    boom::completion_service_cycle(s, p);
    CHECK(!s.rob.entries[1].busy && s.rob.entries[1].exception &&
          s.rob.entries[1].uop.exc_cause == 5 &&
          s.rob.entries[1].memory_completed && boom::prf_read(s,8) == 0x5555 &&
          s.rename.int_free_list.busy_table[8] &&
          !s.lsu.load_response_pending && s.lsu.ldq_count == 0,
          "exceptional load wrote PRF or generated wakeup");
    PASS();
}

int main() {
    std::printf("=== Gate 4.0 W4A Completion Interface Tests ===\n");
    t01(); t02(); t03(); t04(); t05(); t06(); t07(); t08(); t09();
    t10(); t11(); t12(); t13(); t14(); t15(); t16(); t17(); t18(); t19();
    std::printf("W4A completion interfaces: %d passed, %d failed\n", passed, failed);
    return failed ? 1 : 0;
}
