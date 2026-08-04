#include "completion.hpp"
#include "reset.hpp"
#include <cstdio>

namespace boom {
void rob_commit_module(BoomCoreState&, PipeSignals&);
void branch_complete_event(BoomCoreState&, const MicroOp&, bool, uint64_t);
}

static int passed = 0, failed = 0;
static uint8_t observed_peak_rob = 0, observed_peak_prf = 0, observed_peak_wakeup = 0;
static uint64_t observed_total_rob = 0, observed_total_prf = 0, observed_total_wakeup = 0;
#define TEST(n) std::printf("  [W4B multi ROB] %-46s ... ", n)
#define CHECK(c,m) do { if (!(c)) { std::printf("FAIL: %s\n",m); failed++; return; } } while (0)
#define PASS() do { std::printf("PASS\n"); passed++; } while (0)

static void observe(const BoomCoreState& s) {
    if (s.completion.peak_rob_completes > observed_peak_rob)
        observed_peak_rob = s.completion.peak_rob_completes;
    if (s.completion.peak_prf_writes > observed_peak_prf)
        observed_peak_prf = s.completion.peak_prf_writes;
    if (s.completion.peak_wakeups > observed_peak_wakeup)
        observed_peak_wakeup = s.completion.peak_wakeups;
    observed_total_rob += s.completion.total_rob_completes;
    observed_total_prf += s.completion.total_prf_writes;
    observed_total_wakeup += s.completion.total_wakeups;
}

static void owner(BoomCoreState& s, uint8_t index, uint32_t allocation) {
    RobEntry& e = s.rob.entries[index];
    e = RobEntry();
    e.valid = e.busy = true;
    e.uop.uopc = 1;
    e.uop.queue.rob_idx = index;
    e.uop.queue.rob_allocation_id = allocation;
}

static ExecuteState::AluResult result(uint8_t index, uint32_t allocation,
                                      uint8_t pdst = 0, uint64_t value = 0) {
    ExecuteState::AluResult r;
    r.valid = true;
    r.uop.uopc = 1;
    r.uop.queue.rob_idx = index;
    r.uop.queue.rob_allocation_id = allocation;
    r.uop.rename.pdst = pdst;
    r.uop.rename.dst_rtype = pdst ? DST_INT : DST_N;
    r.result = value;
    return r;
}

static void t01() {
    TEST("four ports, canonical integer subset only");
    BoomCoreState s;
    owner(s, 1, 1); owner(s, 2, 2); owner(s, 3, 3);
    boom::completion_from_execute(result(1, 1), COMPLETION_SOURCE_INT_EXECUTE,
                                  s.completion.int_execute);
    boom::completion_from_execute(result(2, 2), COMPLETION_SOURCE_MEM_EXECUTE,
                                  s.completion.mem_execute);
    s.completion.load_response.valid = true;
    s.completion.load_response.kind = COMPLETION_LOAD_RESPONSE;
    s.completion.load_response.source = COMPLETION_SOURCE_LSU_LOAD;
    RobCompleteEvent ports[NUM_ROB_COMPLETE_PORTS];
    boom::build_rob_complete_ports(s, ports);
    CHECK(NUM_ROB_COMPLETE_PORTS == 4 &&
          ports[ROB_COMPLETE_PORT_LSU_LOAD].valid &&
          ports[ROB_COMPLETE_PORT_MEM_EXECUTE].valid &&
          ports[ROB_COMPLETE_PORT_INT_EXECUTE].valid &&
          !ports[ROB_COMPLETE_PORT_UNSUPPORTED].valid,
          "canonical source missing or excluded response port asserted");
    PASS();
}

static void t02() {
    TEST("branch and store complete together");
    BoomCoreState s;
    s.rob.head = 1; s.rob.tail = 3;
    owner(s, 1, 11); owner(s, 2, 12);
    s.branch_state.active_mask = 1;
    s.branch_state.tag_valid[0] = true;
    s.execute.alu_results[INT_ISSUE_LANE] = result(1, 11);
    s.execute.alu_results[INT_ISSUE_LANE].uop.branch.is_br = true;
    s.execute.alu_results[INT_ISSUE_LANE].uop.branch.br_tag = 0;
    s.execute.alu_results[MEM_ISSUE_LANE] = result(2, 12);
    s.execute.alu_results[MEM_ISSUE_LANE].is_store = true;
    s.execute.alu_results[MEM_ISSUE_LANE].memory_valid = true;
    s.execute.alu_results[MEM_ISSUE_LANE].memory_address = 0x100;
    boom::completion_service_execute(s);
    CHECK(!s.rob.entries[1].busy && s.rob.entries[2].busy && s.brupdate.valid &&
          s.lsu.stq_count == 0 && s.completion.rob_completes_this_cycle == 1 &&
          s.completion.mem_execute.valid,
          "younger store crossed branch completion cycle");
    boom::completion_service_execute(s);
    CHECK(!s.rob.entries[2].busy && s.lsu.stq_count == 1,
          "deferred store did not complete");
    observe(s);
    PASS();
}

static void t03() {
    TEST("two nonwriting entries complete");
    BoomCoreState s;
    owner(s, 4, 21); owner(s, 5, 22); s.rob.head = 4;
    s.execute.alu_results[MEM_ISSUE_LANE] = result(4, 21);
    s.execute.alu_results[INT_ISSUE_LANE] = result(5, 22);
    boom::completion_service_execute(s);
    CHECK(!s.rob.entries[4].busy && !s.rob.entries[5].busy &&
          s.completion.rob_completes_this_cycle == 2 &&
          s.completion.prf_writes_this_cycle == 0,
          "nonwriting completions were serialized");
    observe(s);
    PASS();
}

static void t04() {
    TEST("duplicate same ROB allocation rejected");
    BoomCoreState s;
    owner(s, 6, 31); s.rob.head = 6;
    s.execute.alu_results[MEM_ISSUE_LANE] = result(6, 31);
    s.execute.alu_results[INT_ISSUE_LANE] = result(6, 31);
    boom::completion_service_execute(s);
    CHECK(!s.rob.entries[6].busy && s.completion.rob_completes_this_cycle == 1 &&
          !s.completion.mem_execute.valid && !s.completion.int_execute.valid,
          "duplicate completion was accepted twice or retained");
    observe(s);
    PASS();
}

static void t05() {
    TEST("stale and branch-killed events rejected");
    BoomCoreState s;
    s.rob.head = 7; s.rob.tail = 9;
    owner(s, 7, 41); owner(s, 8, 42);
    s.branch_state.active_mask = 1;
    s.branch_state.tag_valid[0] = true;
    s.branch_state.snapshot_valid[0] = true;
    s.execute.alu_results[INT_ISSUE_LANE] = result(7, 41);
    s.execute.alu_results[INT_ISSUE_LANE].uop.branch.is_br = true;
    s.execute.alu_results[INT_ISSUE_LANE].uop.branch.br_tag = 0;
    s.execute.alu_results[INT_ISSUE_LANE].mispredict = true;
    s.execute.alu_results[MEM_ISSUE_LANE] = result(8, 42);
    s.execute.alu_results[MEM_ISSUE_LANE].uop.branch.br_mask = 1;
    boom::completion_service_execute(s);
    CHECK(!s.rob.entries[8].valid && !s.completion.mem_execute.valid &&
          s.completion.rob_completes_this_cycle == 1,
          "younger killed event completed");
    owner(s, 9, 44);
    s.execute.alu_results[INT_ISSUE_LANE] = result(9, 43);
    boom::completion_service_execute(s);
    CHECK(s.rob.entries[9].busy && !s.completion.int_execute.valid,
          "stale allocation survived");
    observe(s);
    PASS();
}

static void t06() {
    TEST("ROB wrap age orders two writers");
    BoomCoreState s;
    s.rob.head = 31;
    owner(s, 31, 51); owner(s, 0, 52);
    s.rename.int_free_list.busy_table[10] = true;
    s.rename.int_free_list.busy_table[11] = true;
    s.execute.alu_results[MEM_ISSUE_LANE] = result(0, 52, 11, 0xbb);
    s.execute.alu_results[INT_ISSUE_LANE] = result(31, 51, 10, 0xaa);
    boom::completion_service_execute(s);
    CHECK(!s.rob.entries[31].busy && !s.rob.entries[0].busy &&
          boom::prf_read(s,10) == 0xaa && boom::prf_read(s,11) == 0xbb &&
          !s.completion.mem_execute.valid && s.completion.prf_writes_this_cycle == 2 &&
          s.completion.writebacks[0].rob_idx == 31,
          "wrap-safe writer priority or dual write failed");
    observe(s);
    PASS();
}

static void t07() {
    TEST("trace backpressure independent of completion");
    BoomCoreState s;
    s.rob.head = 1; s.rob.tail = 3;
    owner(s, 1, 61); owner(s, 2, 62);
    s.execute.alu_results[MEM_ISSUE_LANE] = result(1, 61);
    s.execute.alu_results[INT_ISSUE_LANE] = result(2, 62);
    PipeSignals p; CommitEntry c;
    for (int i = 0; i < 1024; i++) p.commit_trace.write(c);
    boom::completion_service_execute(s);
    boom::rob_commit_module(s, p);
    CHECK(!s.rob.entries[1].busy && !s.rob.entries[2].busy &&
          s.rob.entries[1].valid && !s.rob.commit_valid &&
          s.completion.rob_completes_this_cycle == 2,
          "trace backpressure blocked completion or allowed commit");
    observe(s);
    PASS();
}

static void t08() {
    TEST("two writing completions use two PRF slots");
    BoomCoreState s;
    s.rob.head = 1; s.rob.tail = 3;
    owner(s, 1, 71); owner(s, 2, 72);
    s.rob.entries[1].uop = result(1, 71, 12).uop;
    s.rob.entries[2].uop = result(2, 72, 13).uop;
    s.rename.int_free_list.busy_table[12] = true;
    s.rename.int_free_list.busy_table[13] = true;
    s.execute.alu_results[MEM_ISSUE_LANE] = result(1, 71, 12, 0x12);
    s.execute.alu_results[INT_ISSUE_LANE] = result(2, 72, 13, 0x13);
    boom::completion_service_execute(s);
    CHECK(!s.rob.entries[1].busy && !s.rob.entries[2].busy &&
          boom::prf_read(s,12) == 0x12 && boom::prf_read(s,13) == 0x13 &&
          !s.completion.int_execute.valid &&
          s.completion.prf_writes_this_cycle == 2 &&
           s.completion.wakeups_this_cycle == 2,
          "dual writer completion failed");
    boom::completion_service_execute(s);
    CHECK(s.completion.prf_writes_this_cycle == 0 &&
          s.completion.rob_completes_this_cycle == 0,
          "completed writer repeated next cycle");
    observe(s);
    PASS();
}

static void t09() {
    TEST("lane 2 is invalid and cannot complete");
    BoomCoreState s;
    owner(s, 3, 81);
    s.execute.alu_results[FP_ISSUE_LANE] = result(3, 81, 14, 0xff);
    boom::completion_service_execute(s);
    CHECK(s.rob.entries[3].busy && boom::prf_read(s,14) == 0 &&
          !s.execute.alu_results[FP_ISSUE_LANE].valid &&
          s.completion.rob_completes_this_cycle == 0,
          "reserved FP lane asserted an integer completion");
    observe(s);
    PASS();
}

static void seed_load(BoomCoreState& s, uint8_t index, uint32_t allocation,
                      uint32_t transaction) {
    owner(s, index, allocation);
    RobEntry& e = s.rob.entries[index];
    e.is_load = e.memory_valid = e.memory_request_sent = true;
    e.memory_transaction_id = transaction;
    e.memory_size = 3;
    e.uop.rename.pdst = 15;
    e.uop.rename.dst_rtype = DST_INT;
    s.rename.int_free_list.busy_table[15] = true;
    s.lsu.load_response_pending = true;
    s.lsu.pending_load_transaction_id = transaction;
    s.lsu.pending_load_rob_idx = index;
    s.lsu.pending_load_allocation_id = allocation;
    s.lsu.ldq_count = 1;
    s.lsu.ldq[0].valid = true;
    s.lsu.ldq[0].rob_idx = index;
    s.lsu.ldq[0].rob_allocation_id = allocation;
}

static void t10() {
    TEST("load response plus two execute completions");
    BoomCoreState s;
    s.rob.head = 1;
    seed_load(s, 1, 91, 7);
    owner(s, 2, 92); owner(s, 3, 93);
    s.execute.alu_results[MEM_ISSUE_LANE] = result(2, 92);
    s.execute.alu_results[INT_ISSUE_LANE] = result(3, 93);
    PipeSignals p; DmemResponse response;
    response.transaction_id = 7;
    response.data = response.read_data = 0x5151;
    p.dmem_resp.write(response);
    boom::completion_service_cycle(s, p);
    CHECK(!s.rob.entries[1].busy && !s.rob.entries[2].busy &&
          !s.rob.entries[3].busy && boom::prf_read(s,15) == 0x5151 &&
          !s.lsu.load_response_pending && s.lsu.ldq_count == 0 &&
          s.completion.rob_completes_this_cycle == 3 &&
          s.completion.prf_writes_this_cycle == 1,
          "three fixed pending sources were not serviced safely");
    observe(s);
    PASS();
}

static void t11() {
    TEST("reset clears all retained completions");
    BoomCoreState s;
    s.completion.load_response.valid = true;
    s.completion.mem_execute.valid = true;
    s.completion.int_execute.valid = true;
    ResetControllerState reset;
    boom_core_reset_step(s, reset);
    CHECK(!s.completion.load_response.valid && !s.completion.mem_execute.valid &&
          !s.completion.int_execute.valid && s.completion.peak_rob_completes == 0,
          "pending completion crossed reset control phase");
    PASS();
}

static void t12() {
    TEST("branch recovery precedes exception and kill");
    BoomCoreState s;
    s.rob.head = 1; s.rob.tail = 3;
    owner(s, 1, 101); owner(s, 2, 102);
    s.branch_state.active_mask = 1;
    s.branch_state.tag_valid[0] = true;
    s.branch_state.snapshot_valid[0] = true;
    s.execute.alu_results[INT_ISSUE_LANE] = result(1, 101);
    s.execute.alu_results[INT_ISSUE_LANE].uop.branch.is_br = true;
    s.execute.alu_results[INT_ISSUE_LANE].uop.branch.br_tag = 0;
    s.execute.alu_results[INT_ISSUE_LANE].mispredict = true;
    s.execute.alu_results[INT_ISSUE_LANE].exception = true;
    s.execute.alu_results[INT_ISSUE_LANE].exc_cause = 9;
    s.execute.alu_results[MEM_ISSUE_LANE] = result(2, 102);
    s.execute.alu_results[MEM_ISSUE_LANE].uop.branch.br_mask = 1;
    boom::completion_service_execute(s);
    CHECK(s.brupdate.valid && s.brupdate.mispredict &&
          s.rob.entries[1].exception && s.rob.entries[1].uop.exc_cause == 9 &&
          !s.rob.entries[1].busy && !s.rob.entries[2].valid &&
          !s.completion.mem_execute.valid,
          "branch exception ran before recovery or younger kill");
    observe(s);
    PASS();
}

static void seed_writer_response(BoomCoreState& s, PipeSignals& p,
                                 uint8_t index, uint32_t allocation,
                                 uint32_t transaction) {
    seed_load(s, index, allocation, transaction);
    DmemResponse response;
    response.transaction_id = transaction;
    response.data = response.read_data = 0x7777;
    p.dmem_resp.write(response);
}

static void control_fence(bool jalr) {
    BoomCoreState s;
    PipeSignals p;
    s.rob.head = 1; s.rob.tail = 4;
    seed_writer_response(s, p, 1, 111, 11);
    owner(s, 2, 112); owner(s, 3, 113);
    s.branch_state.active_mask = 1;
    s.branch_state.tag_valid[0] = true;
    s.execute.alu_results[INT_ISSUE_LANE] = result(2, 112, 16, 0x2222);
    s.execute.alu_results[INT_ISSUE_LANE].uop.branch.br_tag = 0;
    s.execute.alu_results[INT_ISSUE_LANE].uop.branch.is_jal = !jalr;
    s.execute.alu_results[INT_ISSUE_LANE].uop.branch.is_jalr = jalr;
    s.execute.alu_results[MEM_ISSUE_LANE] = result(3, 113);
    s.execute.alu_results[MEM_ISSUE_LANE].is_store = true;
    s.execute.alu_results[MEM_ISSUE_LANE].memory_valid = true;
    s.execute.alu_results[MEM_ISSUE_LANE].memory_address = 0x180;
    boom::completion_service_cycle(s, p);
    CHECK(!s.rob.entries[1].busy && !s.rob.entries[2].busy &&
          s.rob.entries[3].busy && !s.completion.int_execute.valid &&
          s.completion.mem_execute.valid && s.lsu.stq_count == 0 &&
          s.brupdate.valid && s.completion.prf_writes_this_cycle == 2 &&
          boom::prf_read(s,16) == 0x2222,
          "younger store crossed control completion cycle");
    boom::completion_service_execute(s);
    CHECK(!s.rob.entries[3].busy && s.lsu.stq_count == 1,
          "deferred control/store did not resume");
    observe(s);
}

static void t13() {
    TEST("older writing JAL fences younger store");
    control_fence(false);
    if (failed) return;
    PASS();
}

static void t14() {
    TEST("older writing JALR fences younger store");
    control_fence(true);
    if (failed) return;
    PASS();
}

static void t15() {
    TEST("older precise exception fences younger store");
    BoomCoreState s;
    PipeSignals p;
    s.rob.head = 1; s.rob.tail = 4;
    seed_writer_response(s, p, 1, 121, 12);
    owner(s, 2, 122); owner(s, 3, 123);
    s.execute.alu_results[INT_ISSUE_LANE] = result(2, 122, 17, 0x1717);
    s.execute.alu_results[INT_ISSUE_LANE].exception = true;
    s.execute.alu_results[INT_ISSUE_LANE].exc_cause = 13;
    s.execute.alu_results[MEM_ISSUE_LANE] = result(3, 123);
    s.execute.alu_results[MEM_ISSUE_LANE].is_store = true;
    s.execute.alu_results[MEM_ISSUE_LANE].memory_valid = true;
    boom::completion_service_cycle(s, p);
    CHECK(!s.rob.entries[2].busy && s.rob.entries[2].exception &&
          s.rob.entries[3].busy && s.lsu.stq_count == 0,
          "exception did not complete or younger side effect crossed fence");
    boom::completion_service_execute(s);
    CHECK(s.rob.entries[2].uop.exc_cause == 13 && s.rob.entries[3].busy &&
          s.lsu.stq_count == 0 && s.completion.mem_execute.valid,
          "younger store crossed persistent precise exception fence");
    observe(s);
    PASS();
}

static void t16() {
    TEST("resolved mask clears retained completion");
    BoomCoreState s;
    PipeSignals p;
    s.rob.head = 1; s.rob.tail = 4;
    seed_writer_response(s, p, 1, 131, 14);
    owner(s, 2, 132); owner(s, 3, 133);
    s.branch_state.active_mask = 1;
    s.branch_state.tag_valid[0] = true;
    s.execute.alu_results[INT_ISSUE_LANE] = result(2, 132);
    s.execute.alu_results[INT_ISSUE_LANE].uop.branch.is_br = true;
    s.execute.alu_results[INT_ISSUE_LANE].uop.branch.br_tag = 0;
    s.execute.alu_results[MEM_ISSUE_LANE] = result(3, 133, 18, 0x1818);
    s.execute.alu_results[MEM_ISSUE_LANE].uop.branch.br_mask = 1;
    boom::completion_service_cycle(s, p);
    CHECK(s.brupdate.valid && !s.brupdate.mispredict &&
          s.completion.mem_execute.valid && s.rob.entries[3].busy &&
          s.completion.mem_execute.uop.branch.br_mask == 0 &&
          boom::prf_read(s,18) == 0,
          "correct resolution did not retain younger writer");
    s.branch_state.active_mask = 1;
    s.branch_state.tag_valid[0] = true;
    owner(s, 4, 134);
    s.rob.tail = 5;
    MicroOp reused;
    reused.queue.rob_idx = 4;
    reused.queue.rob_allocation_id = 134;
    reused.branch.is_br = true;
    reused.branch.br_tag = 0;
    boom::branch_complete_event(s, reused, true, 0x400);
    CHECK(s.rob.entries[3].valid && s.completion.mem_execute.valid,
          "reused branch tag falsely killed retained completion");
    boom::completion_service_execute(s);
    CHECK(!s.completion.mem_execute.valid && !s.rob.entries[3].busy &&
          boom::prf_read(s,18) == 0x1818,
          "retained completion failed after branch-tag reuse");
    observe(s);
    PASS();
}

static void t17() {
    TEST("load AGU never consumes PRF or wakeup");
    BoomCoreState s;
    s.rob.head = 1; s.rob.tail = 3;
    owner(s, 1, 141); owner(s, 2, 142);
    s.rename.int_free_list.busy_table[19] = true;
    s.rename.int_free_list.busy_table[20] = true;
    s.execute.alu_results[MEM_ISSUE_LANE] = result(1, 141, 19, 0xaaaa);
    s.execute.alu_results[MEM_ISSUE_LANE].is_load = true;
    s.execute.alu_results[MEM_ISSUE_LANE].memory_valid = true;
    s.execute.alu_results[MEM_ISSUE_LANE].memory_address = 0x200;
    s.execute.alu_results[INT_ISSUE_LANE] = result(2, 142, 20, 0xbbbb);
    boom::completion_service_execute(s);
    CHECK(s.rob.entries[1].busy && s.rob.entries[1].memory_valid &&
          !s.rob.entries[2].busy && boom::prf_read(s,19) == 0 &&
          s.rename.int_free_list.busy_table[19] && boom::prf_read(s,20) == 0xbbbb &&
          s.completion.prf_writes_this_cycle == 1 &&
          s.completion.wakeups_this_cycle == 1,
          "AGU consumed writeback/wakeup or wrote PRF");
    observe(s);
    PASS();
}

int main() {
    std::printf("=== Gate 4.0 W4B Multi ROB Complete Tests ===\n");
    t01(); t02(); t03(); t04(); t05(); t06(); t07(); t08(); t09();
    t10(); t11(); t12(); t13(); t14(); t15(); t16(); t17();
    std::printf("W4B multi ROB complete: %d passed, %d failed\n", passed, failed);
    std::printf("W4B_METRICS peak_rob_complete=%u peak_prf_writes=%u peak_wakeups=%u total_rob_complete=%llu total_prf_writes=%llu total_wakeups=%llu\n",
                observed_peak_rob, observed_peak_prf, observed_peak_wakeup,
                (unsigned long long)observed_total_rob,
                (unsigned long long)observed_total_prf,
                (unsigned long long)observed_total_wakeup);
    return failed ? 1 : 0;
}
