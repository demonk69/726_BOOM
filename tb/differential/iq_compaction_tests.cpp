#include "boom_config.hpp"
#include "boom_types.hpp"
#include "boom_state.hpp"
#include "boom_interfaces.hpp"
#include <cstdio>

namespace boom { void issue_module(BoomCoreState& state); }

static int tests_passed=0, tests_failed=0;
#define TEST(n) printf("  [IQ-COMPACT] %-58s ... ", n)
#define PASS() do { printf("PASS\n"); tests_passed++; } while(0)
#define FAIL(m) do { printf("FAIL: %s\n", m); tests_failed++; } while(0)
#define CHECK(c,m) do { if(!(c)) { FAIL(m); return; } } while(0)

static MicroOp make_uop(uint8_t id, uint8_t br_mask=0) {
    MicroOp u;
    u.uopc = 1;
    u.debug_inst = id;
    u.iq_type = IQ_ALU;
    u.fu_code = FU_ALU;
    u.branch.br_mask = br_mask;
    u.debug_pc = RESET_VECTOR + (uint64_t)id * 4;
    return u;
}

static void seed_entry(BoomCoreState& s, int idx, uint8_t id, uint8_t br_mask, bool ready) {
    IssueSlotEntry& e = s.issue.alu_iq.entries[idx];
    e = IssueSlotEntry();
    e.valid = true;
    e.request = true;
    e.killed = false;
    e.granted = false;
    e.uop = make_uop(id, br_mask);
    if (!ready) {
        e.uop.rename.prs1 = 40;
        e.prs1_busy = true;
        s.rename.int_free_list.busy_table[40] = true;
    }
}

static void set_mispredict(BoomCoreState& s, uint8_t mask) {
    s.brupdate.valid = true;
    s.brupdate.mispredict = true;
    s.brupdate.resolve_mask = mask;
    s.brupdate.mispredict_mask = mask;
}

static void set_resolve(BoomCoreState& s, uint8_t mask) {
    s.brupdate.valid = true;
    s.brupdate.mispredict = false;
    s.brupdate.resolve_mask = mask;
    s.brupdate.mispredict_mask = 0;
}

static void set_count(BoomCoreState& s, int count) {
    s.issue.alu_iq.head = 0;
    s.issue.alu_iq.tail = (uint8_t)(count % ISSUE_QUEUE_ALU_DEPTH);
    s.issue.alu_iq.count = (uint8_t)count;
}

void kill_none() { TEST("kill none preserves all busy survivors");
    BoomCoreState s; set_mispredict(s, 1);
    for (int i=0; i<4; i++) seed_entry(s, i, (uint8_t)i, 0, false);
    set_count(s, 4); boom::issue_module(s);
    CHECK(s.issue.alu_iq.count == 4, "count changed");
    for (int i=0; i<4; i++) CHECK(s.issue.alu_iq.entries[i].uop.debug_inst == (uint32_t)i, "order changed"); PASS(); }

void kill_all() { TEST("kill all removes all entries");
    BoomCoreState s; set_mispredict(s, 1);
    for (int i=0; i<4; i++) seed_entry(s, i, (uint8_t)i, 1, false);
    set_count(s, 4); boom::issue_module(s);
    CHECK(s.issue.alu_iq.count == 0, "entries survived kill all"); PASS(); }

void kill_alternating() { TEST("kill alternating keeps stable survivor order");
    BoomCoreState s; set_mispredict(s, 1);
    for (int i=0; i<6; i++) seed_entry(s, i, (uint8_t)i, (i & 1) ? 0 : 1, false);
    set_count(s, 6); boom::issue_module(s);
    CHECK(s.issue.alu_iq.count == 3, "wrong survivor count");
    CHECK(s.issue.alu_iq.entries[0].uop.debug_inst == 1, "survivor 0 wrong");
    CHECK(s.issue.alu_iq.entries[1].uop.debug_inst == 3, "survivor 1 wrong");
    CHECK(s.issue.alu_iq.entries[2].uop.debug_inst == 5, "survivor 2 wrong"); PASS(); }

void issue_one_kill_younger() { TEST("issue one plus kill younger");
    BoomCoreState s; set_mispredict(s, 1);
    seed_entry(s, 0, 10, 0, true);
    seed_entry(s, 1, 11, 1, true);
    seed_entry(s, 2, 12, 1, true);
    set_count(s, 3); boom::issue_module(s);
    CHECK(s.issue.issued_valids[INT_ISSUE_LANE], "nothing issued");
    CHECK(s.issue.issued_uops[INT_ISSUE_LANE].debug_inst == 10, "wrong issued uop");
    CHECK(s.issue.alu_iq.count == 0, "younger killed entries survived"); PASS(); }

void issue_one_kill_older() { TEST("issue one after killing older entry");
    BoomCoreState s; set_mispredict(s, 1);
    seed_entry(s, 0, 20, 1, true);
    seed_entry(s, 1, 21, 0, true);
    set_count(s, 2); boom::issue_module(s);
    CHECK(s.issue.issued_valids[INT_ISSUE_LANE], "survivor not issued");
    CHECK(s.issue.issued_uops[INT_ISSUE_LANE].debug_inst == 21, "wrong survivor issued");
    CHECK(s.issue.alu_iq.count == 0, "issued survivor not removed"); PASS(); }

void dispatch_issue_kill_same_cycle() { TEST("dispatch plus issue plus kill same cycle");
    BoomCoreState s; set_mispredict(s, 1);
    seed_entry(s, 0, 30, 1, true);
    set_count(s, 1);
    s.rename.dispatch_packets[0].valid = true;
    s.rename.dispatch_packets[0].rob_allocated = true;
    s.rename.dispatch_packets[0].uop = make_uop(31, 0);
    boom::issue_module(s);
    CHECK(s.issue.issued_valids[INT_ISSUE_LANE], "dispatch survivor not issued");
    CHECK(s.issue.issued_uops[INT_ISSUE_LANE].debug_inst == 31, "wrong dispatch issued");
    CHECK(s.issue.alu_iq.count == 0, "queue not compacted after issue"); PASS(); }

void iq_full_compact() { TEST("IQ full compacts killed entries");
    BoomCoreState s; set_mispredict(s, 1);
    for (int i=0; i<ISSUE_QUEUE_ALU_DEPTH; i++) seed_entry(s, i, (uint8_t)(40+i), (i & 1) ? 0 : 1, false);
    set_count(s, ISSUE_QUEUE_ALU_DEPTH); boom::issue_module(s);
    CHECK(s.issue.alu_iq.count == ISSUE_QUEUE_ALU_DEPTH/2, "full IQ survivor count wrong");
    CHECK(s.issue.alu_iq.tail == ISSUE_QUEUE_ALU_DEPTH/2, "tail not compacted"); PASS(); }

void iq_wrap_compact() { TEST("IQ wrap state resets after stable compact");
    BoomCoreState s; set_mispredict(s, 1);
    for (int i=0; i<4; i++) seed_entry(s, i, (uint8_t)(50+i), (i == 1) ? 1 : 0, false);
    s.issue.alu_iq.head = 6; s.issue.alu_iq.tail = 2; s.issue.alu_iq.count = 4;
    boom::issue_module(s);
    CHECK(s.issue.alu_iq.head == 0, "head not reset");
    CHECK(s.issue.alu_iq.count == 3, "wrap survivor count wrong"); PASS(); }

void nested_branch_masks() { TEST("nested branch mask clears resolved bit only");
    BoomCoreState s; set_resolve(s, 1);
    seed_entry(s, 0, 60, 3, false);
    set_count(s, 1); boom::issue_module(s);
    CHECK(s.issue.alu_iq.count == 1, "entry killed on correct resolve");
    CHECK(s.issue.alu_iq.entries[0].uop.branch.br_mask == 2, "resolved bit not cleared"); PASS(); }

void oldest_ready_kept() { TEST("oldest ready selection preserved");
    BoomCoreState s; set_mispredict(s, 1);
    for (int i=0; i<3; i++) seed_entry(s, i, (uint8_t)(70+i), 0, true);
    set_count(s, 3); boom::issue_module(s);
    CHECK(s.issue.issued_valids[INT_ISSUE_LANE], "no issue");
    CHECK(s.issue.issued_uops[INT_ISSUE_LANE].debug_inst == 70, "oldest ready not selected");
    CHECK(s.issue.alu_iq.count == 2, "issued entry not removed"); PASS(); }

int main() {
    printf("=== BOOM-HLS Gate 3.5 IQ Compaction Tests ===\n");
    kill_none(); kill_all(); kill_alternating(); issue_one_kill_younger(); issue_one_kill_older();
    dispatch_issue_kill_same_cycle(); iq_full_compact(); iq_wrap_compact(); nested_branch_masks(); oldest_ready_kept();
    printf("\n=== %d passed, %d failed ===\n", tests_passed, tests_failed);
    return tests_failed ? 1 : 0;
}
