#include "boom_state.hpp"
#include "boom_interfaces.hpp"
#include "completion.hpp"
#include "issue.hpp"
#include "reset.hpp"

#include <climits>
#include <cstdint>
#include <cstdio>
#include <string>

namespace boom {
void issue_module(BoomCoreState& state);
void execute_module(BoomCoreState& state);
void branch_complete_event(BoomCoreState& state, const MicroOp& uop,
                           bool mispredict, uint64_t redirect_pc);
}

static unsigned checks = 0;
static unsigned failures = 0;

static void check(bool condition, const std::string& name) {
    ++checks;
    if (!condition) {
        ++failures;
        std::fprintf(stderr, "FAIL check=%u %s\n", checks, name.c_str());
    }
}

static uint64_t sext32(uint32_t value) {
    return (value & 0x80000000U) ? 0xffffffff00000000ULL | value : value;
}

static uint64_t divide_reference(unsigned operation, uint64_t lhs, uint64_t rhs) {
    const bool word = operation >= 4;
    const bool sign = operation == 0 || operation == 2 || operation == 4 || operation == 6;
    const bool rem = operation == 2 || operation == 3 || operation == 6 || operation == 7;
    if (word) {
        const uint32_t a = static_cast<uint32_t>(lhs);
        const uint32_t b = static_cast<uint32_t>(rhs);
        uint32_t result;
        if (b == 0) result = rem ? a : UINT32_MAX;
        else if (sign) {
            const int32_t sa = static_cast<int32_t>(a);
            const int32_t sb = static_cast<int32_t>(b);
            if (sa == INT32_MIN && sb == -1) result = rem ? 0 : a;
            else result = static_cast<uint32_t>(rem ? sa % sb : sa / sb);
        } else result = rem ? a % b : a / b;
        return sext32(result);
    }
    if (rhs == 0) return rem ? lhs : UINT64_MAX;
    if (sign) {
        const int64_t a = static_cast<int64_t>(lhs);
        const int64_t b = static_cast<int64_t>(rhs);
        if (a == INT64_MIN && b == -1) return rem ? 0 : lhs;
        return static_cast<uint64_t>(rem ? a % b : a / b);
    }
    return rem ? lhs % rhs : lhs / rhs;
}

static MicroOp make_uop(uint8_t uopc, uint8_t fu, uint8_t iq,
                        uint8_t rob_idx, uint32_t allocation_id, uint8_t pdst,
                        uint8_t prs1 = 0, uint8_t prs2 = 0) {
    MicroOp uop;
    uop.uopc = uopc;
    uop.fu_code = fu;
    uop.iq_type = iq;
    uop.queue.rob_idx = rob_idx;
    uop.queue.rob_allocation_id = allocation_id;
    uop.rename.pdst = pdst;
    uop.rename.prs1 = prs1;
    uop.rename.prs2 = prs2;
    uop.rename.dst_rtype = pdst ? DST_INT : DST_X0;
    uop.ctrl.op1_sel = OP1_RS1;
    uop.ctrl.op2_sel = OP2_RS2;
    return uop;
}

static void own(BoomCoreState& state, const MicroOp& uop) {
    RobEntry& entry = state.rob.entries[uop.queue.rob_idx];
    entry = RobEntry();
    entry.valid = true;
    entry.busy = true;
    entry.uop = uop;
    if (uop.rename.pdst) state.rename.int_free_list.busy_table[uop.rename.pdst] = true;
}

static void dispatch(BoomCoreState& state, const MicroOp& uop) {
    state.rename.dispatch_packets[0].valid = true;
    state.rename.dispatch_packets[0].rob_allocated = true;
    state.rename.dispatch_packets[0].uop = uop;
}

static void start_divide(BoomCoreState& state, const MicroOp& uop,
                         uint64_t lhs, uint64_t rhs) {
    own(state, uop);
    boom::prf_seed(state, uop.rename.prs1, lhs);
    boom::prf_seed(state, uop.rename.prs2, rhs);
    dispatch(state, uop);
    boom::issue_module(state);
    boom::execute_module(state);
}

static void pipeline_cycle(BoomCoreState& state) {
    boom::issue_module(state);
    boom::execute_module(state);
    boom::completion_service_execute(state);
}

static unsigned finish_divide(BoomCoreState& state, uint8_t rob_idx) {
    unsigned cycles = 0;
    while (state.rob.entries[rob_idx].busy && cycles < 70) {
        pipeline_cycle(state);
        ++cycles;
    }
    return cycles;
}

static ExecuteState::AluResult result_for(const MicroOp& uop, uint64_t value) {
    ExecuteState::AluResult result;
    result.valid = true;
    result.uop = uop;
    result.result = value;
    return result;
}

static RobCompleteEvent event_for(const MicroOp& uop, uint64_t value,
                                  CompletionSourceId source) {
    RobCompleteEvent event;
    event.valid = true;
    event.kind = COMPLETION_EXECUTE;
    event.source = source;
    event.uop = uop;
    event.writes_prf = uop.rename.pdst != 0;
    event.value = value;
    return event;
}

static void test_all_operations() {
    const uint64_t lhs[8] = {
        static_cast<uint64_t>(-100LL), UINT64_MAX, static_cast<uint64_t>(-100LL),
        UINT64_MAX, 0x00000000ffffff9cULL, 0x00000000ffffffffULL,
        0x00000000ffffff9cULL, 0x00000000ffffffffULL
    };
    const uint64_t rhs[8] = {7, 17, 7, 17, 7, 17, 7, 17};
    for (unsigned operation = 0; operation < 8; ++operation) {
        BoomCoreState state;
        const uint8_t pdst = static_cast<uint8_t>(10 + operation);
        const MicroOp div = make_uop(static_cast<uint8_t>(21 + operation), FU_DIV,
                                     IQ_ALU, 0, 100 + operation, pdst, 1, 2);
        check(classify_issue_port(div) == ISSUE_PORT_INT,
              "all operations classify on INT lane op " + std::to_string(operation));
        start_divide(state, div, lhs[operation], rhs[operation]);
        check(state.issue.issued_valids[INT_ISSUE_LANE] &&
              state.issue.grants[INT_ISSUE_LANE].accepted,
              "divide request accepted op " + std::to_string(operation));
        check(!state.issue.issued_valids[MEM_ISSUE_LANE] &&
              !state.issue.issued_valids[FP_ISSUE_LANE],
              "divide uses only INT lane op " + std::to_string(operation));
        check(state.execute.divider.token_valid &&
              state.execute.divider.allocation_id == 100 + operation &&
              state.execute.divider.pdst == pdst,
              "divider captures owner token op " + std::to_string(operation));
        check(state.execute.divider.arithmetic.busy,
              "non-fast vector enters iterative busy state op " + std::to_string(operation));
        const unsigned cycles = finish_divide(state, 0);
        check(cycles > 0 && cycles <= (operation >= 4 ? 34U : 66U),
              "bounded integrated latency op " + std::to_string(operation));
        check(!state.rob.entries[0].busy &&
              boom::prf_read(state, pdst) == divide_reference(operation, lhs[operation], rhs[operation]),
              "architectural result op " + std::to_string(operation));
        check(!state.rename.int_free_list.busy_table[pdst],
              "destination becomes ready op " + std::to_string(operation));
        check(state.completion.total_completion_accepts == 1 &&
              state.completion.total_rob_completes == 1,
              "exactly one completion op " + std::to_string(operation));
        check(state.completion.total_prf_writes == 1 &&
              state.completion.total_wakeups == 1 &&
              state.completion.total_bypass == 1,
              "exactly one publication op " + std::to_string(operation));
        const uint64_t accepts = state.completion.total_completion_accepts;
        const uint64_t writes = state.completion.total_prf_writes;
        pipeline_cycle(state);
        pipeline_cycle(state);
        check(state.completion.total_completion_accepts == accepts &&
              state.completion.total_prf_writes == writes,
              "no duplicate after completion op " + std::to_string(operation));
    }
}

static void test_edge_semantics() {
    struct Edge { unsigned op; uint64_t lhs; uint64_t rhs; const char* name; };
    const Edge edges[] = {
        {0, 1ULL << 63, UINT64_MAX, "div overflow"},
        {2, 1ULL << 63, UINT64_MAX, "rem overflow"},
        {0, 0x1234, 0, "signed divide by zero"},
        {3, 0x123456789abcdef0ULL, 0, "unsigned remainder by zero"},
        {4, 0x80000000ULL, UINT32_MAX, "divw overflow"},
        {6, 0x80000000ULL, UINT32_MAX, "remw overflow"},
        {5, 0xabcdef01ffffffffULL, 0, "divuw zero and high ignored"},
        {7, 0xabcdef0180000001ULL, 0, "remuw zero sign extension"}
    };
    for (unsigned i = 0; i < sizeof(edges) / sizeof(edges[0]); ++i) {
        BoomCoreState state;
        MicroOp div = make_uop(static_cast<uint8_t>(21 + edges[i].op), FU_DIV,
                               IQ_ALU, 0, 200 + i, 20, 1, 2);
        start_divide(state, div, edges[i].lhs, edges[i].rhs);
        check(state.execute.divider.token_valid &&
              state.execute.divider.arithmetic.result_pending &&
              !state.execute.divider.arithmetic.busy,
              std::string(edges[i].name) + " fast pending response");
        finish_divide(state, 0);
        check(boom::prf_read(state, 20) ==
              divide_reference(edges[i].op, edges[i].lhs, edges[i].rhs),
              std::string(edges[i].name) + " result");
        check(state.completion.total_prf_writes == 1 &&
              state.completion.total_rob_completes == 1,
              std::string(edges[i].name) + " exactly once");
    }
}

static void test_busy_backpressure_and_other_units() {
    BoomCoreState state;
    MicroOp first = make_uop(22, FU_DIV, IQ_ALU, 0, 301, 10, 1, 2);
    start_divide(state, first, UINT64_MAX, 19);
    check(state.execute.divider.token_valid && state.execute.divider.arithmetic.busy,
          "first divide establishes busy state");

    MicroOp second = make_uop(21, FU_DIV, IQ_ALU, 1, 302, 11, 3, 4);
    own(state, second);
    boom::prf_seed(state, 3, 99);
    boom::prf_seed(state, 4, 7);
    dispatch(state, second);
    boom::issue_module(state);
    check(!state.issue.issued_valids[INT_ISSUE_LANE] && state.issue.alu_iq.count == 1,
          "busy divider backpressures and retains second request");
    const uint32_t first_allocation = state.execute.divider.allocation_id;
    boom::issue_module(state);
    check(!state.issue.issued_valids[INT_ISSUE_LANE] && state.issue.alu_iq.count == 1 &&
          state.execute.divider.allocation_id == first_allocation,
          "repeated busy request neither issues nor replaces owner");

    MicroOp alu = make_uop(1, FU_ALU, IQ_ALU, 2, 303, 12, 5, 6);
    own(state, alu);
    boom::prf_seed(state, 5, 40);
    boom::prf_seed(state, 6, 2);
    dispatch(state, alu);
    boom::issue_module(state);
    check(state.issue.issued_valids[INT_ISSUE_LANE], "ordinary ALU issues while divide busy");
    boom::execute_module(state);
    boom::completion_service_execute(state);
    check(boom::prf_read(state, 12) == 42 && !state.rob.entries[2].busy,
          "ordinary ALU completes while divide busy");
    check(state.execute.divider.token_valid, "ordinary ALU does not disturb divider");

    MicroOp mul = make_uop(16, FU_MUL, IQ_ALU, 3, 304, 13, 5, 6);
    own(state, mul);
    dispatch(state, mul);
    boom::issue_module(state);
    boom::execute_module(state);
    boom::completion_service_execute(state);
    check(boom::prf_read(state, 13) == 80 && !state.rob.entries[3].busy,
          "multiply issues and completes while divide busy");
    check(state.execute.divider.token_valid, "multiply does not disturb divider");

    MicroOp branch = make_uop(31, FU_ALU, IQ_ALU, 4, 305, 0, 5, 6);
    branch.branch.is_br = true;
    branch.branch.br_tag = 3;
    own(state, branch);
    dispatch(state, branch);
    boom::issue_module(state);
    boom::execute_module(state);
    boom::completion_service_execute(state);
    check(state.brupdate.valid && !state.brupdate.mispredict && !state.rob.entries[4].busy,
          "ordinary branch resolves while divide busy");
    check(state.execute.divider.token_valid, "correct branch preserves older divider");

    MicroOp load = make_uop(39, FU_MEM, IQ_MEM, 5, 306, 14, 7, 0);
    load.ctrl.is_load = true;
    load.mem.uses_ldq = true;
    load.mem.mem_size = 3;
    own(state, load);
    boom::prf_seed(state, 7, 0x80001000ULL);
    dispatch(state, load);
    boom::issue_module(state);
    check(state.issue.issued_valids[MEM_ISSUE_LANE] &&
          !state.issue.issued_valids[FP_ISSUE_LANE],
          "MEM lane issues concurrently with busy divider");
    boom::execute_module(state);
    boom::completion_service_execute(state);
    check(state.rob.entries[5].memory_valid && state.rob.entries[5].is_load &&
          state.lsu.ldq_count == 1 && state.rob.entries[5].busy,
          "load address enters LSU while divider remains active");
    check(state.execute.divider.token_valid, "load concurrency preserves divider");
}

static void test_pending_hold_and_collision() {
    BoomCoreState state;
    MicroOp div = make_uop(21, FU_DIV, IQ_ALU, 1, 401, 20, 1, 2);
    start_divide(state, div, 123, 0);
    MicroOp blocker = make_uop(1, FU_ALU, IQ_ALU, 0, 400, 19);
    own(state, blocker);
    state.execute.alu_results[INT_ISSUE_LANE] = result_for(blocker, 77);
    MicroOp waiting = make_uop(22, FU_DIV, IQ_ALU, 2, 402, 21, 3, 4);
    own(state, waiting);
    dispatch(state, waiting);
    boom::issue_module(state);
    check(!state.issue.issued_valids[INT_ISSUE_LANE] && state.issue.alu_iq.count == 1,
          "pending divider response backpressures and retains a new request");
    boom::execute_module(state);
    check(state.execute.divider.token_valid &&
          state.execute.divider.arithmetic.result_pending,
          "pending divider response holds behind occupied INT result slot");
    check(!boom::divider_request_ready(state.execute.divider.arithmetic),
          "held response backpressures new requests");
    boom::completion_service_execute(state);
    check(boom::prf_read(state, 19) == 77 && state.rob.entries[1].busy,
          "blocking completion drains without losing divider response");
    pipeline_cycle(state);
    check(boom::prf_read(state, 20) == UINT64_MAX && !state.rob.entries[1].busy,
          "held divider response completes after slot opens");
    check(state.completion.total_rob_completes == 2 &&
          state.completion.total_prf_writes == 2,
          "held response and blocker each publish exactly once");

    BoomCoreState collision;
    MicroOp fast = make_uop(23, FU_DIV, IQ_ALU, 1, 411, 22, 1, 2);
    start_divide(collision, fast, static_cast<uint64_t>(-55LL), 0);
    MicroOp load = make_uop(39, FU_MEM, IQ_MEM, 0, 410, 23, 3);
    load.ctrl.is_load = true;
    load.mem.uses_ldq = true;
    load.mem.mem_size = 3;
    own(collision, load);
    boom::prf_seed(collision, 3, 0x80002000ULL);
    dispatch(collision, load);
    boom::issue_module(collision);
    boom::execute_module(collision);
    check(collision.execute.alu_results[MEM_ISSUE_LANE].valid &&
          collision.execute.alu_results[INT_ISSUE_LANE].valid,
          "divider response collides with MEM result in fixed slots");
    boom::completion_service_execute(collision);
    check(collision.completion.completion_accepts_this_cycle == 2,
          "collision accepts both MEM address and divider response");
    check(collision.completion.rob_completes_this_cycle == 1 &&
          collision.completion.prf_writes_this_cycle == 1,
          "collision counts only divider ROB completion/write");
    check(boom::prf_read(collision, 22) == static_cast<uint64_t>(-55LL) &&
          collision.rob.entries[0].busy && collision.rob.entries[0].memory_valid,
          "collision preserves load and publishes divider result");
}

static void test_three_way_retention_and_forwarding() {
    BoomCoreState state;
    state.rob.head = 0;
    MicroOp older0 = make_uop(1, FU_ALU, IQ_ALU, 0, 500, 30);
    MicroOp older1 = make_uop(1, FU_ALU, IQ_ALU, 1, 501, 31);
    MicroOp div = make_uop(22, FU_DIV, IQ_ALU, 2, 502, 32, 1, 2);
    own(state, older0);
    own(state, older1);
    start_divide(state, div, 100, 1);
    state.completion.load_response = event_for(older0, 10, COMPLETION_SOURCE_LSU_LOAD);
    state.execute.alu_results[MEM_ISSUE_LANE] = result_for(older1, 11);
    boom::issue_module(state);
    boom::execute_module(state);
    boom::completion_service_execute(state);
    check(state.completion.prf_writes_this_cycle == 2 &&
          state.completion.rob_completes_this_cycle == 2,
          "three-way collision services exactly two write slots");
    check(state.completion.int_execute.valid && state.rob.entries[2].busy,
          "young divider completion retained in pending INT slot");
    check(state.completion.total_wakeups == 3 && state.completion.total_bypass == 3,
          "all three collision values forward once before retention");
    uint64_t value = 0;
    bool conflict = false;
    check(boom::wakeup_lookup(state, 32, value, conflict) && !conflict && value == 100,
          "retained divider value is visible on wakeup network");
    MicroOp dependent = make_uop(1, FU_ALU, IQ_ALU, 3, 503, 33, 32, 0);
    own(state, dependent);
    dispatch(state, dependent);
    boom::issue_module(state);
    check(state.issue.issued_valids[INT_ISSUE_LANE] &&
          state.issue.issued_prs1_data[INT_ISSUE_LANE] == 100,
          "dependent issues from retained divider forwarding");
    boom::execute_module(state);
    boom::completion_service_execute(state);
    check(boom::prf_read(state, 32) == 100 && !state.rob.entries[2].busy,
          "retained divider completes on next service cycle");
    check(state.rob.entries[3].busy &&
          state.execute.alu_results[INT_ISSUE_LANE].valid &&
          state.execute.alu_results[INT_ISSUE_LANE].result == 100,
          "dependent result holds while retained divider owns completion slot");
    pipeline_cycle(state);
    check(boom::prf_read(state, 33) == 100 && !state.rob.entries[3].busy,
          "dependent consumes forwarded divider result after slot opens");
    check(state.completion.total_rob_completes == 4 &&
          state.completion.total_prf_writes == 4 &&
          state.completion.total_wakeups == 4 &&
          state.completion.total_bypass == 4,
          "PRF/wakeup/bypass/ROB counters are exactly once across retention");
    pipeline_cycle(state);
    check(state.completion.total_rob_completes == 4 &&
          state.completion.total_prf_writes == 4 &&
          state.completion.total_wakeups == 4 &&
          state.completion.total_bypass == 4,
          "empty cycle cannot duplicate retained publications");
}

static MicroOp branch_owner(BoomCoreState& state, uint8_t index,
                            uint32_t allocation, uint8_t tag) {
    MicroOp branch = make_uop(31, FU_ALU, IQ_ALU, index, allocation, 0);
    branch.branch.is_br = true;
    branch.branch.br_tag = tag;
    own(state, branch);
    return branch;
}

static void test_branch_kill_and_preservation() {
    BoomCoreState active;
    active.rob.head = 0;
    MicroOp older = make_uop(1, FU_ALU, IQ_ALU, 0, 600, 8);
    own(active, older);
    MicroOp branch = branch_owner(active, 1, 601, 0);
    MicroOp div = make_uop(22, FU_DIV, IQ_ALU, 2, 602, 9, 1, 2);
    div.branch.br_mask = 1;
    start_divide(active, div, 999, 13);
    boom::branch_complete_event(active, branch, true, 0x4444);
    check(!active.execute.divider.token_valid &&
          boom::divider_request_ready(active.execute.divider.arithmetic),
          "mispredict kills active iterative divider");
    check(active.rob.entries[0].valid && active.rob.entries[0].busy,
          "mispredict preserves older ROB owner");
    check(active.rob.entries[1].valid && !active.rob.entries[2].valid && active.rob.tail == 2,
          "mispredict preserves branch and removes younger divider ROB entry");
    check(active.frontend.pc == (0x4444ULL & ~3ULL),
          "divider kill accompanies canonical redirect");

    BoomCoreState pending;
    branch = branch_owner(pending, 0, 610, 1);
    div = make_uop(21, FU_DIV, IQ_ALU, 1, 611, 10, 1, 2);
    div.branch.br_mask = 2;
    start_divide(pending, div, 17, 0);
    check(pending.execute.divider.arithmetic.result_pending,
          "fast divider response pending before branch kill");
    boom::branch_complete_event(pending, branch, true, 0x5555);
    check(!pending.execute.divider.token_valid &&
          !pending.execute.divider.arithmetic.result_pending,
          "mispredict kills pending divider response");
    pipeline_cycle(pending);
    check(pending.completion.total_prf_writes == 0 && boom::prf_read(pending, 10) == 0,
          "killed pending response never publishes");

    BoomCoreState correct;
    branch = branch_owner(correct, 0, 620, 2);
    div = make_uop(22, FU_DIV, IQ_ALU, 1, 621, 11, 1, 2);
    div.branch.br_mask = 4;
    start_divide(correct, div, 1000, 9);
    boom::branch_complete_event(correct, branch, false, 0);
    check(correct.execute.divider.token_valid && correct.execute.divider.branch_mask == 0 &&
          correct.execute.divider.uop.branch.br_mask == 0,
          "correct resolution clears divider dependency without killing it");
    finish_divide(correct, 1);
    check(boom::prf_read(correct, 11) == 111 && !correct.rob.entries[1].busy,
          "resolved divider completes normally");
}

static void test_stale_flush_reset_and_lane2() {
    BoomCoreState stale;
    MicroOp div = make_uop(22, FU_DIV, IQ_ALU, 0, 700, 12, 1, 2);
    start_divide(stale, div, 100, 7);
    stale.rob.entries[0].uop.queue.rob_allocation_id = 701;
    pipeline_cycle(stale);
    check(!stale.execute.divider.token_valid &&
          boom::divider_request_ready(stale.execute.divider.arithmetic),
          "ROB reuse cancels stale active divider token");
    check(stale.completion.total_prf_writes == 0 && boom::prf_read(stale, 12) == 0,
          "stale active token cannot publish");

    BoomCoreState stale_pending;
    div = make_uop(21, FU_DIV, IQ_ALU, 0, 705, 18, 1, 2);
    start_divide(stale_pending, div, 100, 0);
    stale_pending.rob.entries[0].uop.queue.rob_allocation_id = 706;
    pipeline_cycle(stale_pending);
    check(!stale_pending.execute.divider.token_valid &&
          !stale_pending.execute.divider.arithmetic.result_pending,
          "ROB reuse cancels stale pending divider token");
    check(stale_pending.completion.total_rob_completes == 0 &&
          boom::prf_read(stale_pending, 18) == 0,
          "stale pending response cannot complete reused ROB entry");

    BoomCoreState stale_issue;
    div = make_uop(21, FU_DIV, IQ_ALU, 0, 710, 13, 1, 2);
    own(stale_issue, div);
    stale_issue.rob.entries[0].uop.queue.rob_allocation_id = 711;
    dispatch(stale_issue, div);
    boom::issue_module(stale_issue);
    check(!stale_issue.issue.issued_valids[INT_ISSUE_LANE] &&
          stale_issue.issue.alu_iq.count == 1,
          "stale allocation is refused at divider issue");

    BoomCoreState flush;
    div = make_uop(22, FU_DIV, IQ_ALU, 0, 720, 14, 1, 2);
    start_divide(flush, div, 100, 7);
    flush.global_flush = true;
    boom::issue_module(flush);
    boom::execute_module(flush);
    check(!flush.execute.divider.token_valid &&
          boom::divider_request_ready(flush.execute.divider.arithmetic),
          "global flush cancels active divider and restores ready");
    check(flush.issue.alu_iq.count == 0, "global flush also clears queued requests");

    BoomCoreState reset_busy;
    div = make_uop(22, FU_DIV, IQ_ALU, 0, 730, 15, 1, 2);
    start_divide(reset_busy, div, 100, 7);
    ResetControllerState controller;
    boom_core_reset_step(reset_busy, controller);
    check(!reset_busy.execute.divider.token_valid &&
          boom::divider_request_ready(reset_busy.execute.divider.arithmetic),
          "reset control phase kills busy divider");
    check(!reset_busy.completion.int_execute.valid &&
          reset_busy.completion.total_prf_writes == 0,
          "reset clears divider completion evidence");

    BoomCoreState reset_pending;
    div = make_uop(21, FU_DIV, IQ_ALU, 0, 740, 16, 1, 2);
    start_divide(reset_pending, div, 100, 0);
    controller = ResetControllerState();
    boom_core_reset_step(reset_pending, controller);
    check(!reset_pending.execute.divider.token_valid &&
          !reset_pending.execute.divider.arithmetic.result_pending,
          "reset control phase kills pending divider response");
    for (unsigned cycle = 0; cycle < 200 && !controller.completed; ++cycle)
        boom_core_reset_step(reset_pending, controller);
    check(controller.completed && boom::divider_request_ready(reset_pending.execute.divider.arithmetic),
          "full reset leaves divider idle and ready");

    BoomCoreState lane2;
    MicroOp fp_slot = make_uop(21, FU_DIV, IQ_ALU, 0, 750, 17);
    own(lane2, fp_slot);
    lane2.execute.alu_results[FP_ISSUE_LANE] = result_for(fp_slot, 1234);
    boom::completion_service_execute(lane2);
    check(lane2.rob.entries[0].busy && boom::prf_read(lane2, 17) == 0,
          "lane2 cannot complete a divider result");
    check(!lane2.execute.alu_results[FP_ISSUE_LANE].valid &&
          lane2.completion.total_rob_completes == 0,
          "lane2 input is discarded without completion accounting");
}

int main() {
    test_all_operations();
    test_edge_semantics();
    test_busy_backpressure_and_other_units();
    test_pending_hold_and_collision();
    test_three_way_retention_and_forwarding();
    test_branch_kill_and_preservation();
    test_stale_flush_reset_and_lane2();
    std::printf("divider_integration_checks=%u\nfailures=%u\n", checks, failures);
    return failures == 0 && checks >= 60 ? 0 : 1;
}
