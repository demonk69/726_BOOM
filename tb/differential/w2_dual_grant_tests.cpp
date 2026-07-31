#include "boom_config.hpp"
#include "boom_state.hpp"
#include "reset.hpp"

#include <cstdio>

namespace boom { void issue_module(BoomCoreState& state); }

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) std::printf("  [W2] %-62s ... ", name)
#define PASS() do { std::printf("PASS\n"); ++tests_passed; } while (0)
#define FAIL(message) do { std::printf("FAIL: %s\n", message); ++tests_failed; return; } while (0)
#define CHECK(condition, message) do { if (!(condition)) FAIL(message); } while (0)

static MicroOp make_uop(IssuePortClass port, uint8_t rob_idx, uint8_t branch_mask = 0) {
    MicroOp uop;
    uop.queue.rob_idx = rob_idx;
    uop.branch.br_mask = branch_mask;
    if (port == ISSUE_PORT_MEM) {
        uop.uopc = 39;
        uop.iq_type = IQ_MEM;
        uop.fu_code = FU_MEM;
    } else if (port == ISSUE_PORT_INT) {
        uop.uopc = 1;
        uop.iq_type = IQ_ALU;
        uop.fu_code = FU_ALU;
    } else {
        uop.uopc = 14;
        uop.iq_type = IQ_ALU;
        uop.fu_code = FU_ALU;
    }
    return uop;
}

static void seed(BoomCoreState& state, int index, IssuePortClass port, uint8_t rob_idx,
                 uint8_t branch_mask = 0, bool request = true) {
    IssueSlotEntry& entry = state.issue.alu_iq.entries[index];
    entry = IssueSlotEntry();
    entry.valid = true;
    entry.request = request;
    entry.uop = make_uop(port, rob_idx, branch_mask);
}

static void set_count(BoomCoreState& state, int count) {
    state.issue.alu_iq.head = 0;
    state.issue.alu_iq.tail = (uint8_t)(count % ISSUE_QUEUE_ALU_DEPTH);
    state.issue.alu_iq.count = (uint8_t)count;
}

static bool accounting_is(const BoomCoreState& state, int generated, int accepted, int retained) {
    return state.issue.grants_generated == generated &&
           state.issue.grants_accepted == accepted &&
           state.issue.grants_retained == retained &&
           state.issue.grants_dropped == 0;
}

static void t01_dual_mem_int_generated() {
    TEST("01 dual MEM+INT grants independently accept");
    BoomCoreState state;
    seed(state, 0, ISSUE_PORT_MEM, 10);
    seed(state, 1, ISSUE_PORT_INT, 11);
    set_count(state, 2);
    boom::issue_module(state);
    CHECK(accounting_is(state, 2, 2, 0), "wrong dual accounting");
    CHECK(state.issue.grants[MEM_ISSUE_LANE].valid && state.issue.grants[MEM_ISSUE_LANE].accepted,
          "MEM grant was not accepted");
    CHECK(state.issue.grants[INT_ISSUE_LANE].valid && state.issue.grants[INT_ISSUE_LANE].accepted,
          "INT grant was not accepted");
    CHECK(state.issue.grants[MEM_ISSUE_LANE].entry_index == 0 &&
          state.issue.grants[MEM_ISSUE_LANE].uop.queue.rob_idx == 10 &&
          state.issue.grants[MEM_ISSUE_LANE].port_class == ISSUE_PORT_MEM, "bad MEM grant metadata");
    CHECK(state.issue.grants[INT_ISSUE_LANE].entry_index == 1 &&
          state.issue.grants[INT_ISSUE_LANE].uop.queue.rob_idx == 11 &&
          state.issue.grants[INT_ISSUE_LANE].port_class == ISSUE_PORT_INT, "bad INT grant metadata");
    CHECK(state.issue.alu_iq.count == 0, "accepted entries remained queued");
    PASS();
}

static void t02_single_mem() {
    TEST("02 single MEM class grants on fixed MEM lane");
    BoomCoreState state;
    seed(state, 0, ISSUE_PORT_MEM, 20);
    set_count(state, 1);
    boom::issue_module(state);
    CHECK(accounting_is(state, 1, 1, 0), "wrong single-MEM accounting");
    CHECK(state.issue.grants[0].valid && state.issue.grants[0].accepted &&
          state.issue.grants[0].entry_index == 0 && state.issue.grants[0].uop.queue.rob_idx == 20 &&
          state.issue.grants[0].port_class == ISSUE_PORT_MEM, "bad MEM grant");
    CHECK(!state.issue.grants[INT_ISSUE_LANE].valid && state.issue.alu_iq.count == 0,
          "single MEM affected INT lane or remained queued");
    PASS();
}

static void t03_single_int() {
    TEST("03 single INT class grants on fixed INT lane");
    BoomCoreState state;
    seed(state, 0, ISSUE_PORT_INT, 30);
    set_count(state, 1);
    boom::issue_module(state);
    CHECK(accounting_is(state, 1, 1, 0), "wrong single-INT accounting");
    CHECK(state.issue.grants[1].valid && state.issue.grants[1].accepted &&
          state.issue.grants[1].entry_index == 0 && state.issue.grants[1].uop.queue.rob_idx == 30 &&
          state.issue.grants[1].port_class == ISSUE_PORT_INT, "bad INT grant");
    CHECK(!state.issue.grants[MEM_ISSUE_LANE].valid && state.issue.alu_iq.count == 0,
          "single INT affected MEM lane or remained queued");
    PASS();
}

static void t04_two_mem_only_one() {
    TEST("04 two MEM entries produce only oldest MEM grant");
    BoomCoreState state;
    seed(state, 0, ISSUE_PORT_MEM, 40);
    seed(state, 1, ISSUE_PORT_MEM, 41);
    set_count(state, 2);
    boom::issue_module(state);
    CHECK(accounting_is(state, 1, 1, 0), "two MEM entries generated multiple grants");
    CHECK(state.issue.grants[0].entry_index == 0 && state.issue.grants[0].uop.queue.rob_idx == 40,
          "oldest MEM was not selected");
    CHECK(state.issue.alu_iq.count == 1 && state.issue.alu_iq.entries[0].uop.queue.rob_idx == 41,
          "younger MEM did not survive");
    PASS();
}

static void t05_two_int_only_one() {
    TEST("05 two INT entries produce only oldest INT grant");
    BoomCoreState state;
    seed(state, 0, ISSUE_PORT_INT, 50);
    seed(state, 1, ISSUE_PORT_INT, 51);
    set_count(state, 2);
    boom::issue_module(state);
    CHECK(accounting_is(state, 1, 1, 0), "two INT entries generated multiple grants");
    CHECK(state.issue.grants[1].entry_index == 0 && state.issue.grants[1].uop.queue.rob_idx == 50,
          "oldest INT was not selected");
    CHECK(state.issue.alu_iq.count == 1 && state.issue.alu_iq.entries[0].uop.queue.rob_idx == 51,
          "younger INT did not survive");
    PASS();
}

static void t06_two_mem_one_int_mix() {
    TEST("06 2 MEM + 1 INT mix generates one grant per class");
    BoomCoreState state;
    seed(state, 0, ISSUE_PORT_MEM, 60);
    seed(state, 1, ISSUE_PORT_MEM, 61);
    seed(state, 2, ISSUE_PORT_INT, 62);
    set_count(state, 3);
    boom::issue_module(state);
    CHECK(accounting_is(state, 2, 2, 0), "wrong 2+1 accounting");
    CHECK(state.issue.grants[0].entry_index == 0 && state.issue.grants[0].uop.queue.rob_idx == 60,
          "wrong MEM selected from 2+1 mix");
    CHECK(state.issue.grants[1].entry_index == 2 && state.issue.grants[1].uop.queue.rob_idx == 62,
          "wrong INT selected from 2+1 mix");
    CHECK(state.issue.alu_iq.count == 1 && state.issue.alu_iq.entries[0].uop.queue.rob_idx == 61,
          "2+1 survivor order changed");
    PASS();
}

static void t07_one_mem_two_int_mix() {
    TEST("07 1 MEM + 2 INT mix generates one grant per class");
    BoomCoreState state;
    seed(state, 0, ISSUE_PORT_INT, 70);
    seed(state, 1, ISSUE_PORT_INT, 71);
    seed(state, 2, ISSUE_PORT_MEM, 72);
    set_count(state, 3);
    boom::issue_module(state);
    CHECK(accounting_is(state, 2, 2, 0), "wrong 1+2 accounting");
    CHECK(state.issue.grants[1].entry_index == 0 && state.issue.grants[1].uop.queue.rob_idx == 70,
          "wrong INT selected from 1+2 mix");
    CHECK(state.issue.grants[0].entry_index == 2 && state.issue.grants[0].uop.queue.rob_idx == 72,
          "wrong MEM selected from 1+2 mix");
    CHECK(state.issue.alu_iq.count == 1 && state.issue.alu_iq.entries[0].uop.queue.rob_idx == 71,
          "1+2 survivor order changed");
    PASS();
}

static void t08_unique_grant_entries() {
    TEST("08 dual grants identify distinct queue entries and ROBs");
    BoomCoreState state;
    seed(state, 0, ISSUE_PORT_INT, 80);
    seed(state, 1, ISSUE_PORT_MEM, 81);
    set_count(state, 2);
    boom::issue_module(state);
    const IssueGrant& mem = state.issue.grants[MEM_ISSUE_LANE];
    const IssueGrant& integer = state.issue.grants[INT_ISSUE_LANE];
    CHECK(mem.valid && integer.valid && mem.entry_index != integer.entry_index,
          "dual grants alias one queue entry");
    CHECK(mem.uop.queue.rob_idx == 81 && integer.uop.queue.rob_idx == 80 &&
          mem.uop.queue.rob_idx != integer.uop.queue.rob_idx, "dual grants alias one ROB");
    CHECK(integer.accepted && mem.accepted && accounting_is(state, 2, 2, 0),
          "unique entries were not independently accepted");
    PASS();
}

static void t09_oldest_mem_per_port() {
    TEST("09 oldest ready MEM is selected independently of INT");
    BoomCoreState state;
    seed(state, 0, ISSUE_PORT_INT, 90);
    seed(state, 1, ISSUE_PORT_MEM, 91);
    seed(state, 2, ISSUE_PORT_MEM, 92);
    set_count(state, 3);
    boom::issue_module(state);
    CHECK(state.issue.grants[0].entry_index == 1 && state.issue.grants[0].uop.queue.rob_idx == 91 &&
          state.issue.grants[0].port_class == ISSUE_PORT_MEM, "younger MEM won");
    CHECK(state.issue.grants[1].entry_index == 0 && state.issue.grants[1].uop.queue.rob_idx == 90,
          "INT selection interfered with oldest MEM");
    CHECK(accounting_is(state, 2, 2, 0), "wrong oldest-MEM accounting");
    PASS();
}

static void t10_oldest_int_per_port() {
    TEST("10 oldest ready INT is selected independently of MEM");
    BoomCoreState state;
    seed(state, 0, ISSUE_PORT_MEM, 100);
    seed(state, 1, ISSUE_PORT_INT, 101);
    seed(state, 2, ISSUE_PORT_INT, 102);
    set_count(state, 3);
    boom::issue_module(state);
    CHECK(state.issue.grants[1].entry_index == 1 && state.issue.grants[1].uop.queue.rob_idx == 101 &&
          state.issue.grants[1].port_class == ISSUE_PORT_INT, "younger INT won");
    CHECK(state.issue.grants[0].entry_index == 0 && state.issue.grants[0].uop.queue.rob_idx == 100,
          "MEM selection interfered with oldest INT");
    CHECK(accounting_is(state, 2, 2, 0), "wrong oldest-INT accounting");
    PASS();
}

static void t11_raw_blocked() {
    TEST("11 RAW-busy source blocks grant and preserves old entry");
    BoomCoreState state;
    seed(state, 0, ISSUE_PORT_INT, 110);
    state.issue.alu_iq.entries[0].uop.rename.prs1 = 7;
    state.rename.int_free_list.busy_table[7] = true;
    seed(state, 1, ISSUE_PORT_MEM, 111);
    set_count(state, 2);
    boom::issue_module(state);
    CHECK(!state.issue.grants[INT_ISSUE_LANE].valid, "RAW-blocked INT granted");
    CHECK(state.issue.grants[MEM_ISSUE_LANE].valid && state.issue.grants[0].entry_index == 1 &&
          state.issue.grants[0].uop.queue.rob_idx == 111, "ready MEM did not grant");
    CHECK(accounting_is(state, 1, 1, 0), "RAW accounting wrong");
    CHECK(state.issue.alu_iq.count == 1 && state.issue.alu_iq.entries[0].uop.queue.rob_idx == 110 &&
          state.issue.alu_iq.entries[0].prs1_busy, "blocked old entry was not retained busy");
    PASS();
}

static void t12_branch_kills() {
    TEST("12 branch mispredict kills matching entries in stable order");
    BoomCoreState state;
    seed(state, 0, ISSUE_PORT_MEM, 120, 1, false);
    seed(state, 1, ISSUE_PORT_INT, 121, 0, false);
    seed(state, 2, ISSUE_PORT_MEM, 122, 1, false);
    seed(state, 3, ISSUE_PORT_INT, 123, 2, false);
    set_count(state, 4);
    state.brupdate.valid = true;
    state.brupdate.mispredict = true;
    state.brupdate.mispredict_mask = 1;
    state.brupdate.resolve_mask = 1;
    boom::issue_module(state);
    CHECK(accounting_is(state, 0, 0, 0), "branch-kill cycle generated a grant");
    CHECK(state.issue.alu_iq.count == 2 && state.issue.alu_iq.entries[0].uop.queue.rob_idx == 121 &&
          state.issue.alu_iq.entries[1].uop.queue.rob_idx == 123, "branch-kill survivor order wrong");
    CHECK(state.issue.alu_iq.entries[1].uop.branch.br_mask == 2, "unresolved branch bit changed");
    PASS();
}

static void t13_kill_plus_dual() {
    TEST("13 branch kill compacts survivors then generates dual grants");
    BoomCoreState state;
    seed(state, 0, ISSUE_PORT_INT, 130, 1);
    seed(state, 1, ISSUE_PORT_MEM, 131, 0);
    seed(state, 2, ISSUE_PORT_INT, 132, 0);
    set_count(state, 3);
    state.brupdate.valid = true;
    state.brupdate.mispredict = true;
    state.brupdate.mispredict_mask = 1;
    boom::issue_module(state);
    CHECK(accounting_is(state, 2, 2, 0), "kill+dual accounting wrong");
    CHECK(state.issue.grants[0].entry_index == 0 && state.issue.grants[0].uop.queue.rob_idx == 131 &&
          state.issue.grants[0].accepted, "post-kill MEM grant wrong");
    CHECK(state.issue.grants[1].entry_index == 1 && state.issue.grants[1].uop.queue.rob_idx == 132 &&
          state.issue.grants[1].accepted, "post-kill INT grant wrong");
    CHECK(state.issue.alu_iq.count == 0, "kill+dual accepted entry remained");
    PASS();
}

static void t14_resolve_mask_plus_dual() {
    TEST("14 correct resolve clears mask while dual grants proceed");
    BoomCoreState state;
    seed(state, 0, ISSUE_PORT_MEM, 140, 3);
    seed(state, 1, ISSUE_PORT_INT, 141, 1);
    set_count(state, 2);
    state.brupdate.valid = true;
    state.brupdate.resolve_mask = 1;
    boom::issue_module(state);
    CHECK(accounting_is(state, 2, 2, 0), "resolve+dual accounting wrong");
    CHECK(state.issue.grants[0].uop.branch.br_mask == 2 &&
          state.issue.grants[1].uop.branch.br_mask == 0, "resolved mask not reflected in grants");
    CHECK(state.issue.grants[0].entry_index == 0 && state.issue.grants[1].entry_index == 1,
          "resolve changed grant indices");
    CHECK(state.issue.alu_iq.count == 0, "resolved accepted entry remained");
    PASS();
}

static void t15_full_iq_frees_slot() {
    TEST("15 full IQ dual acceptance frees both entry slots");
    BoomCoreState state;
    for (int i = 0; i < ISSUE_QUEUE_ALU_DEPTH; ++i)
        seed(state, i, (i & 1) ? ISSUE_PORT_INT : ISSUE_PORT_MEM, (uint8_t)(150 + i));
    set_count(state, ISSUE_QUEUE_ALU_DEPTH);
    boom::issue_module(state);
    CHECK(accounting_is(state, 2, 2, 0), "full-IQ accounting wrong");
    CHECK(state.issue.alu_iq.count == ISSUE_QUEUE_ALU_DEPTH - 2 &&
          state.issue.alu_iq.tail == ISSUE_QUEUE_ALU_DEPTH - 2, "accepted grants did not free two slots");
    for (int i = 0; i < ISSUE_QUEUE_ALU_DEPTH - 2; ++i)
        CHECK(state.issue.alu_iq.entries[i].uop.queue.rob_idx == 152 + i, "full-IQ survivor order wrong");
    PASS();
}

static void t16_dispatch_insertion() {
    TEST("16 blocked same-class dispatch inserts after old acceptance");
    BoomCoreState state;
    seed(state, 0, ISSUE_PORT_INT, 160);
    set_count(state, 1);
    state.rename.dispatch_packets[0].valid = true;
    state.rename.dispatch_packets[0].rob_allocated = true;
    state.rename.dispatch_packets[0].uop = make_uop(ISSUE_PORT_INT, 161);
    boom::issue_module(state);
    CHECK(accounting_is(state, 1, 1, 0), "dispatch insertion accounting wrong");
    CHECK(state.issue.grants[1].valid && !state.issue.grants[1].from_dispatch &&
          state.issue.grants[1].uop.queue.rob_idx == 160, "old INT did not take lane");
    CHECK(state.issue.alu_iq.count == 1 && state.issue.alu_iq.entries[0].uop.queue.rob_idx == 161 &&
          state.issue.alu_iq.entries[0].request, "dispatch uop was not inserted");
    PASS();
}

static void t17_dispatch_bypass() {
    TEST("17 empty-lane dispatch bypass grants without IQ insertion");
    BoomCoreState state;
    state.rename.dispatch_packets[0].valid = true;
    state.rename.dispatch_packets[0].rob_allocated = true;
    state.rename.dispatch_packets[0].uop = make_uop(ISSUE_PORT_MEM, 170);
    boom::issue_module(state);
    const IssueGrant& grant = state.issue.grants[MEM_ISSUE_LANE];
    CHECK(accounting_is(state, 1, 1, 0), "dispatch bypass accounting wrong");
    CHECK(grant.valid && grant.accepted && grant.from_dispatch && grant.entry_index == 0xff &&
          grant.uop.queue.rob_idx == 170 && grant.port_class == ISSUE_PORT_MEM,
          "dispatch bypass metadata wrong");
    CHECK(state.issue.issued_valids[0] && state.issue.alu_iq.count == 0,
          "accepted bypass was inserted into IQ");
    PASS();
}

static void t18_stable_compaction() {
    TEST("18 sparse IQ compacts valid survivors in physical order");
    BoomCoreState state;
    seed(state, 0, ISSUE_PORT_INT, 180, 0, false);
    seed(state, 3, ISSUE_PORT_MEM, 181, 0, false);
    seed(state, 6, ISSUE_PORT_INT, 182, 0, false);
    state.issue.alu_iq.head = 4;
    state.issue.alu_iq.tail = 7;
    state.issue.alu_iq.count = 3;
    boom::issue_module(state);
    CHECK(accounting_is(state, 0, 0, 0), "non-requesting compact entries granted");
    CHECK(state.issue.alu_iq.head == 0 && state.issue.alu_iq.tail == 3 &&
          state.issue.alu_iq.count == 3, "compaction metadata wrong");
    CHECK(state.issue.alu_iq.entries[0].uop.queue.rob_idx == 180 &&
          state.issue.alu_iq.entries[1].uop.queue.rob_idx == 181 &&
          state.issue.alu_iq.entries[2].uop.queue.rob_idx == 182, "compaction was not stable");
    PASS();
}

static void t26_dispatch_raw_blocked() {
    TEST("26 RAW-busy dispatch cannot bypass issue readiness");
    BoomCoreState state;
    state.rename.dispatch_packets[0].valid = true;
    state.rename.dispatch_packets[0].rob_allocated = true;
    state.rename.dispatch_packets[0].uop = make_uop(ISSUE_PORT_INT, 252);
    state.rename.dispatch_packets[0].uop.rename.prs1 = 7;
    state.rename.int_free_list.busy_table[7] = true;
    boom::issue_module(state);
    CHECK(accounting_is(state, 0, 0, 0), "busy dispatch generated a grant");
    CHECK(!state.issue.issued_valids[0], "busy dispatch reached execute intake");
    CHECK(state.issue.alu_iq.count == 1 && state.issue.alu_iq.entries[0].uop.queue.rob_idx == 252 &&
          state.issue.alu_iq.entries[0].prs1_busy, "busy dispatch was not retained in IQ");
    PASS();
}

static void t19_compact_wrap_metadata() {
    TEST("19 wrapped metadata normalizes after stable compaction");
    BoomCoreState state;
    seed(state, 0, ISSUE_PORT_INT, 190, 0, false);
    seed(state, 1, ISSUE_PORT_MEM, 191, 0, false);
    seed(state, 4, ISSUE_PORT_INT, 192, 0, false);
    seed(state, 7, ISSUE_PORT_MEM, 193, 0, false);
    state.issue.alu_iq.head = 6;
    state.issue.alu_iq.tail = 2;
    state.issue.alu_iq.count = 4;
    boom::issue_module(state);
    CHECK(state.issue.alu_iq.head == 0 && state.issue.alu_iq.tail == 4 &&
          state.issue.alu_iq.count == 4, "wrapped IQ metadata not normalized");
    CHECK(state.issue.alu_iq.entries[0].uop.queue.rob_idx == 190 &&
          state.issue.alu_iq.entries[1].uop.queue.rob_idx == 191 &&
          state.issue.alu_iq.entries[2].uop.queue.rob_idx == 192 &&
          state.issue.alu_iq.entries[3].uop.queue.rob_idx == 193, "wrapped survivor order wrong");
    CHECK(accounting_is(state, 0, 0, 0), "wrapped non-requesting entries granted");
    PASS();
}

static void t20_reset_pending_grants() {
    TEST("20 reset controller clears pending IQ and dual-grant state");
    BoomCoreState state;
    seed(state, 0, ISSUE_PORT_MEM, 200);
    seed(state, 1, ISSUE_PORT_INT, 201);
    set_count(state, 2);
    boom::issue_module(state);
    CHECK(state.issue.grants_generated == 2, "test did not establish pending dual grants");
    ResetControllerState controller;
    for (int steps = 0; !controller.completed && steps < 200; ++steps)
        boom_core_reset_step(state, controller);
    CHECK(controller.completed, "reset controller did not complete");
    CHECK(state.issue.alu_iq.count == 0 && state.issue.alu_iq.head == 0 &&
          state.issue.alu_iq.tail == 0, "reset retained IQ metadata");
    for (int i = 0; i < ISSUE_QUEUE_ALU_DEPTH; ++i)
        CHECK(!state.issue.alu_iq.entries[i].valid, "reset retained an IQ entry");
    CHECK(accounting_is(state, 0, 0, 0), "reset retained grant accounting");
    for (int lane = 0; lane < ISSUE_WIDTH; ++lane)
        CHECK(!state.issue.grants[lane].valid && !state.issue.issued_valids[lane],
              "reset retained pending lane state");
    CHECK(state.issue.port_ready[0] && state.issue.port_ready[1] && !state.issue.port_ready[2],
          "reset restored wrong lane readiness");
    PASS();
}

static void t21_unsupported_uop() {
    TEST("21 unsupported ALU encoding neither grants nor drops");
    BoomCoreState state;
    seed(state, 0, ISSUE_PORT_UNSUPPORTED, 210);
    set_count(state, 1);
    boom::issue_module(state);
    CHECK(accounting_is(state, 0, 0, 0), "unsupported uop affected accounting");
    CHECK(!state.issue.grants[0].valid && !state.issue.grants[1].valid,
          "unsupported uop granted");
    CHECK(state.issue.alu_iq.count == 1 && state.issue.alu_iq.entries[0].uop.queue.rob_idx == 210,
          "unsupported uop was dropped");
    PASS();
}

static void t22_fp_uop_unsupported() {
    TEST("22 FP uop remains unsupported by W2 integer issue");
    BoomCoreState state;
    MicroOp fp;
    fp.uopc = 1;
    fp.iq_type = IQ_FPU;
    fp.fu_code = FU_FPU;
    fp.queue.rob_idx = 220;
    state.issue.alu_iq.entries[0].valid = true;
    state.issue.alu_iq.entries[0].request = true;
    state.issue.alu_iq.entries[0].uop = fp;
    set_count(state, 1);
    boom::issue_module(state);
    CHECK(accounting_is(state, 0, 0, 0), "FP uop affected W2 accounting");
    CHECK(!state.issue.grants[0].valid && !state.issue.grants[1].valid &&
          !state.issue.grants[2].valid, "FP uop received a grant");
    CHECK(state.issue.alu_iq.count == 1 && state.issue.alu_iq.entries[0].uop.queue.rob_idx == 220,
          "FP uop was dropped");
    PASS();
}

static void t23_lane2_always_invalid() {
    TEST("23 reserved lane 2 is cleared and remains invalid on dual issue");
    BoomCoreState state;
    state.issue.grants[FP_ISSUE_LANE].valid = true;
    state.issue.issued_valids[FP_ISSUE_LANE] = true;
    seed(state, 0, ISSUE_PORT_MEM, 230);
    seed(state, 1, ISSUE_PORT_INT, 231);
    set_count(state, 2);
    boom::issue_module(state);
    CHECK(accounting_is(state, 2, 2, 0), "lane-2 test dual accounting wrong");
    CHECK(!state.issue.grants[FP_ISSUE_LANE].valid &&
          state.issue.grants[FP_ISSUE_LANE].port_class == ISSUE_PORT_UNSUPPORTED &&
          !state.issue.issued_valids[FP_ISSUE_LANE], "reserved lane 2 became active");
    CHECK(state.issue.grants[0].valid && state.issue.grants[1].valid,
          "lane-2 clearing disturbed dual grants");
    PASS();
}

static void t24_per_lane_ready_backpressure() {
    TEST("24 each lane's ready backpressure independently redirects acceptance");
    BoomCoreState mem_blocked;
    seed(mem_blocked, 0, ISSUE_PORT_MEM, 240);
    seed(mem_blocked, 1, ISSUE_PORT_INT, 241);
    set_count(mem_blocked, 2);
    mem_blocked.issue.port_ready[MEM_ISSUE_LANE] = false;
    boom::issue_module(mem_blocked);
    CHECK(accounting_is(mem_blocked, 2, 1, 1), "MEM-blocked accounting wrong");
    CHECK(!mem_blocked.issue.grants[0].accepted && mem_blocked.issue.grants[1].accepted &&
          !mem_blocked.issue.issued_valids[0] && mem_blocked.issue.issued_valids[1],
          "MEM backpressure did not select INT");
    CHECK(mem_blocked.issue.alu_iq.count == 1 &&
          mem_blocked.issue.alu_iq.entries[0].uop.queue.rob_idx == 240,
          "MEM-blocked survivor wrong");

    BoomCoreState int_blocked;
    seed(int_blocked, 0, ISSUE_PORT_INT, 242);
    seed(int_blocked, 1, ISSUE_PORT_MEM, 243);
    set_count(int_blocked, 2);
    int_blocked.issue.port_ready[INT_ISSUE_LANE] = false;
    boom::issue_module(int_blocked);
    CHECK(accounting_is(int_blocked, 2, 1, 1), "INT-blocked accounting wrong");
    CHECK(int_blocked.issue.grants[0].accepted && !int_blocked.issue.grants[1].accepted &&
          int_blocked.issue.issued_valids[0] && !int_blocked.issue.issued_valids[1],
          "INT backpressure did not select MEM");
    CHECK(int_blocked.issue.alu_iq.count == 1 &&
          int_blocked.issue.alu_iq.entries[0].uop.queue.rob_idx == 242,
          "INT-blocked survivor wrong");
    PASS();
}

static void t25_both_lanes_busy() {
    TEST("25 both busy lanes retain both generated old grants");
    BoomCoreState state;
    seed(state, 0, ISSUE_PORT_MEM, 250);
    seed(state, 1, ISSUE_PORT_INT, 251);
    set_count(state, 2);
    state.issue.port_ready[MEM_ISSUE_LANE] = false;
    state.issue.port_ready[INT_ISSUE_LANE] = false;
    boom::issue_module(state);
    CHECK(accounting_is(state, 2, 0, 2), "both-busy accounting wrong");
    CHECK(state.issue.grants[0].valid && !state.issue.grants[0].accepted &&
          state.issue.grants[1].valid && !state.issue.grants[1].accepted,
          "busy lane accepted or lost a grant");
    CHECK(!state.issue.issued_valids[0] && !state.issue.issued_valids[1],
          "busy lane issued a uop");
    CHECK(state.issue.alu_iq.count == 2 && state.issue.alu_iq.entries[0].uop.queue.rob_idx == 250 &&
          state.issue.alu_iq.entries[1].uop.queue.rob_idx == 251, "both-busy survivor order wrong");
    PASS();
}

static void t27_exception_bypasses_iq() {
    TEST("27 exception uops bypass selection storage");
    BoomCoreState state;
    seed(state, 0, ISSUE_PORT_INT, 253);
    state.issue.alu_iq.entries[0].uop.exception = true;
    set_count(state, 1);
    state.rename.dispatch_packets[0].valid = true;
    state.rename.dispatch_packets[0].rob_allocated = true;
    state.rename.dispatch_packets[0].uop = make_uop(ISSUE_PORT_INT, 254);
    state.rename.dispatch_packets[0].uop.exception = true;
    boom::issue_module(state);
    CHECK(accounting_is(state, 0, 0, 0), "exception affected grant accounting");
    CHECK(state.issue.alu_iq.count == 0, "exception remained in selection storage");
    CHECK(!state.issue.issued_valids[0], "exception reached execute intake");
    PASS();
}

static void t28_full_blocked_iq_does_not_grant_dispatch() {
    TEST("28 full blocked IQ does not generate unretainable dispatch grant");
    BoomCoreState state;
    for (int i = 0; i < ISSUE_QUEUE_ALU_DEPTH; ++i)
        seed(state, i, ISSUE_PORT_MEM, (uint8_t)(200 + i));
    set_count(state, ISSUE_QUEUE_ALU_DEPTH);
    state.issue.port_ready[MEM_ISSUE_LANE] = false;
    state.issue.port_ready[INT_ISSUE_LANE] = false;
    state.rename.dispatch_packets[0].valid = true;
    state.rename.dispatch_packets[0].rob_allocated = true;
    state.rename.dispatch_packets[0].uop = make_uop(ISSUE_PORT_INT, 255);
    boom::issue_module(state);
    CHECK(accounting_is(state, 1, 0, 1), "unretainable dispatch entered grant accounting");
    CHECK(state.issue.grants[MEM_ISSUE_LANE].valid &&
          !state.issue.grants[INT_ISSUE_LANE].valid, "blocked dispatch generated a grant");
    CHECK(state.issue.alu_iq.count == ISSUE_QUEUE_ALU_DEPTH,
          "full blocked IQ lost a queue-resident grant");
    PASS();
}

int main() {
    std::printf("=== BOOM-HLS W2 Dual Grant Directed Tests ===\n");
    t01_dual_mem_int_generated();
    t02_single_mem();
    t03_single_int();
    t04_two_mem_only_one();
    t05_two_int_only_one();
    t06_two_mem_one_int_mix();
    t07_one_mem_two_int_mix();
    t08_unique_grant_entries();
    t09_oldest_mem_per_port();
    t10_oldest_int_per_port();
    t11_raw_blocked();
    t12_branch_kills();
    t13_kill_plus_dual();
    t14_resolve_mask_plus_dual();
    t15_full_iq_frees_slot();
    t16_dispatch_insertion();
    t17_dispatch_bypass();
    t18_stable_compaction();
    t19_compact_wrap_metadata();
    t20_reset_pending_grants();
    t21_unsupported_uop();
    t22_fp_uop_unsupported();
    t23_lane2_always_invalid();
    t24_per_lane_ready_backpressure();
    t25_both_lanes_busy();
    t26_dispatch_raw_blocked();
    t27_exception_bypasses_iq();
    t28_full_blocked_iq_does_not_grant_dispatch();
    std::printf("\n=== %d passed, %d failed, %d total ===\n",
                tests_passed, tests_failed, tests_passed + tests_failed);
    return tests_failed == 0 ? 0 : 1;
}
