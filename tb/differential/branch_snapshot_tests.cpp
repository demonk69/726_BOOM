#include "boom_config.hpp"
#include "boom_types.hpp"
#include "boom_state.hpp"
#include "boom_interfaces.hpp"
#include <cstdio>

namespace boom {
void rename_module(BoomCoreState& state);
void rob_allocate(BoomCoreState& state);
void issue_module(BoomCoreState& state);
void execute_module(BoomCoreState& state);
void branch_module(BoomCoreState& state);
void lsu_module(BoomCoreState& state, PipeSignals& pipe);
void frontend_module(BoomCoreState& state, PipeSignals& pipe);
void rob_commit_module(BoomCoreState& state, PipeSignals& pipe);
}

static int tests_passed=0, tests_failed=0;
#define TEST(n) printf("  [BR-SNAP] %-62s ... ", n)
#define PASS() do { printf("PASS\n"); tests_passed++; } while(0)
#define FAIL(m) do { printf("FAIL: %s\n", m); tests_failed++; } while(0)
#define CHECK(c,m) do { if(!(c)) { FAIL(m); return; } } while(0)

static uint8_t bit(uint8_t tag) { return (uint8_t)(1u << tag); }

static MicroOp make_add(uint8_t ldst, uint8_t lrs1=0, uint8_t lrs2=0) {
    MicroOp u;
    u.uopc = 50;
    u.iq_type = IQ_ALU;
    u.fu_code = FU_ALU;
    u.rename.ldst = ldst;
    u.rename.lrs1 = lrs1;
    u.rename.lrs2 = lrs2;
    u.rename.dst_rtype = (ldst != 0) ? DST_INT : DST_X0;
    u.debug_pc = RESET_VECTOR;
    u.inst = 0x00100093u;
    return u;
}

static MicroOp make_branch() {
    MicroOp u;
    u.uopc = 31;
    u.iq_type = IQ_ALU;
    u.fu_code = FU_ALU;
    u.rename.dst_rtype = DST_N;
    u.branch.is_br = true;
    u.debug_pc = RESET_VECTOR;
    u.inst = 0x00000063u;
    return u;
}

static MicroOp make_store(uint8_t rs1=1, uint8_t rs2=2) {
    MicroOp u;
    u.uopc = 49;
    u.iq_type = IQ_MEM;
    u.fu_code = FU_MEM;
    u.rename.dst_rtype = DST_N;
    u.rename.lrs1 = rs1;
    u.rename.lrs2 = rs2;
    u.ctrl.is_sta = true;
    u.mem.uses_stq = true;
    u.mem.mem_size = 3;
    return u;
}

static bool rename_one(BoomCoreState& s, const MicroOp& in, MicroOp& out) {
    s.decode.dec_valids[0] = true;
    s.decode.dec_uops[0] = in;
    boom::rename_module(s);
    s.decode.dec_valids[0] = false;
    if (!s.rename.dispatch_packets[0].valid) return false;
    out = s.rename.dispatch_packets[0].uop;
    return true;
}

static bool rename_alloc(BoomCoreState& s, const MicroOp& in, MicroOp& out) {
    if (!rename_one(s, in, out)) return false;
    boom::rob_allocate(s);
    out = s.rename.dispatch_packets[0].uop;
    s.rename.dispatch_packets[0] = RenameDispatchPacket();
    return true;
}

static void resolve_branch(BoomCoreState& s, const MicroOp& br, bool mispredict, uint64_t target=RESET_VECTOR+16) {
    s.execute.alu_results[0] = ExecuteState::AluResult();
    s.execute.alu_results[0].valid = true;
    s.execute.alu_results[0].uop = br;
    s.execute.alu_results[0].mispredict = mispredict;
    s.execute.alu_results[0].redirect_pc = target;
    boom::branch_module(s);
}

static int valid_rob_count(const BoomCoreState& s) {
    int n=0;
    for (int i=0; i<ROB_DEPTH; i++) if (s.rob.entries[i].valid) n++;
    return n;
}

static int valid_ldq_count(const BoomCoreState& s) {
    int n=0;
    for (int i=0; i<LDQ_DEPTH; i++) if (s.lsu.ldq[i].valid) n++;
    return n;
}

static int valid_stq_count(const BoomCoreState& s) {
    int n=0;
    for (int i=0; i<STQ_DEPTH; i++) if (s.lsu.stq[i].valid) n++;
    return n;
}

void t1_single_branch_correct() { TEST("single pending branch correct releases tag");
    BoomCoreState s; MicroOp br;
    CHECK(rename_alloc(s, make_branch(), br), "branch rename failed");
    CHECK(s.branch_state.active_mask == bit(br.branch.br_tag), "tag not active");
    resolve_branch(s, br, false);
    CHECK(s.branch_state.active_mask == 0, "tag not released");
    CHECK(!s.branch_state.tag_valid[br.branch.br_tag], "tag_valid leaked"); PASS(); }

void t2_single_branch_mispredict() { TEST("single branch mispredict restores map and releases tag");
    BoomCoreState s; MicroOp br, wrong;
    CHECK(rename_alloc(s, make_branch(), br), "branch rename failed");
    uint8_t old_x1 = s.rename.int_map_table.map_table[1];
    CHECK(rename_alloc(s, make_add(1), wrong), "wrong-path rename failed");
    CHECK(s.rename.int_map_table.map_table[1] != old_x1, "x1 did not rename");
    resolve_branch(s, br, true);
    CHECK(s.rename.int_map_table.map_table[1] == old_x1, "map did not restore");
    CHECK(s.branch_state.active_mask == 0, "active mask not cleared"); PASS(); }

void t3_two_nested_correct() { TEST("two nested branches both correct release independently");
    BoomCoreState s; MicroOp b0,b1;
    CHECK(rename_alloc(s, make_branch(), b0), "b0 rename failed");
    CHECK(rename_alloc(s, make_branch(), b1), "b1 rename failed");
    CHECK((b1.branch.br_mask & bit(b0.branch.br_tag)) != 0, "inner did not inherit outer mask");
    resolve_branch(s, b1, false);
    CHECK(s.branch_state.active_mask == bit(b0.branch.br_tag), "inner release cleared wrong mask");
    resolve_branch(s, b0, false);
    CHECK(s.branch_state.active_mask == 0, "outer tag not released"); PASS(); }

void t4_inner_mispredict() { TEST("inner branch mispredict keeps outer branch active");
    BoomCoreState s; MicroOp b0,b1,wrong;
    CHECK(rename_alloc(s, make_branch(), b0), "b0 rename failed");
    CHECK(rename_alloc(s, make_branch(), b1), "b1 rename failed");
    CHECK(rename_alloc(s, make_add(2), wrong), "wrong rename failed");
    resolve_branch(s, b1, true);
    CHECK(s.branch_state.active_mask == bit(b0.branch.br_tag), "outer tag not preserved");
    CHECK(!s.branch_state.tag_valid[b1.branch.br_tag], "inner tag leaked"); PASS(); }

void t5_outer_mispredict() { TEST("outer branch mispredict kills inner branch state");
    BoomCoreState s; MicroOp b0,b1,wrong;
    CHECK(rename_alloc(s, make_branch(), b0), "b0 rename failed");
    CHECK(rename_alloc(s, make_branch(), b1), "b1 rename failed");
    CHECK(rename_alloc(s, make_add(2), wrong), "wrong rename failed");
    resolve_branch(s, b0, true);
    CHECK(s.branch_state.active_mask == 0, "active mask not cleared");
    CHECK(!s.branch_state.tag_valid[b1.branch.br_tag], "inner tag leaked after outer miss"); PASS(); }

void t6_outer_mispredict_after_inner_resolved() { TEST("outer mispredict after inner resolved stays precise");
    BoomCoreState s; MicroOp b0,b1,wrong;
    CHECK(rename_alloc(s, make_branch(), b0), "b0 rename failed");
    CHECK(rename_alloc(s, make_branch(), b1), "b1 rename failed");
    resolve_branch(s, b1, false);
    CHECK(rename_alloc(s, make_add(3), wrong), "wrong rename failed");
    resolve_branch(s, b0, true);
    CHECK(s.branch_state.active_mask == 0, "outer miss did not clear active mask"); PASS(); }

void t7_branch_tag_pool_exhaustion() { TEST("branch tag pool exhaustion backpressures rename");
    BoomCoreState s; MicroOp br;
    for (int i=0; i<MAX_BRANCH_COUNT; i++) CHECK(rename_alloc(s, make_branch(), br), "branch allocation before full failed");
    CHECK(!rename_one(s, make_branch(), br), "branch renamed with full tag pool"); PASS(); }

void t8_allocate_release_same_cycle_model() { TEST("branch tag allocate after release reuses slot safely");
    BoomCoreState s; MicroOp b0,b1;
    CHECK(rename_alloc(s, make_branch(), b0), "b0 rename failed");
    uint8_t tag = b0.branch.br_tag;
    resolve_branch(s, b0, false);
    CHECK(rename_alloc(s, make_branch(), b1), "b1 rename failed after release");
    CHECK(b1.branch.br_tag == tag, "released tag was not reused first"); PASS(); }

void t9_branch_tag_wrap() { TEST("branch tag wrap after freeing tag zero");
    BoomCoreState s; MicroOp brs[MAX_BRANCH_COUNT], bnew;
    for (int i=0; i<MAX_BRANCH_COUNT; i++) CHECK(rename_alloc(s, make_branch(), brs[i]), "initial branch allocation failed");
    resolve_branch(s, brs[0], false);
    CHECK(rename_alloc(s, make_branch(), bnew), "new branch after release failed");
    CHECK(bnew.branch.br_tag == brs[0].branch.br_tag, "tag zero did not wrap/reuse"); PASS(); }

void t10_multi_rename_after_branch() { TEST("multiple post-branch pdst allocations tracked");
    BoomCoreState s; MicroOp br,a,b;
    CHECK(rename_alloc(s, make_branch(), br), "branch rename failed");
    CHECK(rename_alloc(s, make_add(1), a), "a rename failed");
    CHECK(rename_alloc(s, make_add(2), b), "b rename failed");
    CHECK(s.branch_state.br_alloc_lists[br.branch.br_tag][a.rename.pdst], "first pdst not tracked");
    CHECK(s.branch_state.br_alloc_lists[br.branch.br_tag][b.rename.pdst], "second pdst not tracked"); PASS(); }

void t11_map_table_restore() { TEST("map table restore uses branch snapshot");
    BoomCoreState s; MicroOp br,a;
    CHECK(rename_alloc(s, make_add(1), a), "initial rename failed");
    uint8_t correct = s.rename.int_map_table.map_table[1];
    CHECK(rename_alloc(s, make_branch(), br), "branch rename failed");
    CHECK(rename_alloc(s, make_add(1), a), "wrong rename failed");
    resolve_branch(s, br, true);
    CHECK(s.rename.int_map_table.map_table[1] == correct, "snapshot restore wrong"); PASS(); }

void t12_free_list_rollback() { TEST("free-list rollback recovers wrong-path pdst");
    BoomCoreState s; MicroOp br,a;
    CHECK(rename_alloc(s, make_branch(), br), "branch rename failed");
    uint8_t count_before = s.rename.int_free_list.count;
    CHECK(rename_alloc(s, make_add(1), a), "wrong rename failed");
    CHECK(s.rename.int_free_list.count < count_before, "free count did not decrease");
    resolve_branch(s, br, true);
    CHECK(s.rename.int_free_list.count == count_before, "free count not restored"); PASS(); }

void t13_busy_table_recovery() { TEST("busy table clears wrong pdst and preserves correct busy pdst");
    BoomCoreState s; MicroOp older,br,wrong;
    CHECK(rename_alloc(s, make_add(5), older), "older rename failed");
    s.rob.entries[older.queue.rob_idx].busy = true;
    uint8_t correct_pdst = older.rename.pdst;
    CHECK(rename_alloc(s, make_branch(), br), "branch rename failed");
    CHECK(rename_alloc(s, make_add(1), wrong), "wrong rename failed");
    uint8_t wrong_pdst = wrong.rename.pdst;
    resolve_branch(s, br, true);
    CHECK(s.rename.int_free_list.busy_table[correct_pdst], "correct busy pdst lost");
    CHECK(!s.rename.int_free_list.busy_table[wrong_pdst], "wrong busy pdst leaked"); PASS(); }

void t14_rob_young_clear() { TEST("ROB younger entries clear on mispredict");
    BoomCoreState s; MicroOp br,wrong;
    CHECK(rename_alloc(s, make_branch(), br), "branch rename failed");
    CHECK(rename_alloc(s, make_add(1), wrong), "wrong rename failed");
    CHECK(valid_rob_count(s) == 2, "setup ROB count wrong");
    resolve_branch(s, br, true);
    CHECK(s.rob.entries[br.queue.rob_idx].valid, "branch entry killed");
    CHECK(valid_rob_count(s) == 1, "younger ROB entry not killed"); PASS(); }

void t15_iq_young_clear() { TEST("IQ younger entries clear on mispredict");
    BoomCoreState s; MicroOp br,wrong;
    CHECK(rename_alloc(s, make_branch(), br), "branch rename failed");
    CHECK(rename_one(s, make_add(1), wrong), "wrong rename failed");
    boom::issue_module(s);
    CHECK(s.issue.alu_iq.count <= 1, "unexpected IQ setup");
    if (s.issue.issued_valids[0]) { s.issue.alu_iq.entries[0].valid=true; s.issue.alu_iq.entries[0].uop=wrong; s.issue.alu_iq.entries[0].request=true; s.issue.alu_iq.count=1; }
    resolve_branch(s, br, true);
    CHECK(s.issue.alu_iq.count == 0, "IQ entry not killed"); PASS(); }

void t16_ldq_young_clear() { TEST("LDQ younger entries clear on mispredict");
    BoomCoreState s; MicroOp br;
    CHECK(rename_alloc(s, make_branch(), br), "branch rename failed");
    s.lsu.ldq[0].valid=true; s.lsu.ldq[0].branch_mask=bit(br.branch.br_tag); s.lsu.ldq_count=1; s.lsu.ldq_tail=1;
    resolve_branch(s, br, true);
    CHECK(valid_ldq_count(s) == 0 && s.lsu.ldq_count == 0, "LDQ not cleared"); PASS(); }

void t17_stq_young_clear() { TEST("STQ younger entries clear on mispredict");
    BoomCoreState s; MicroOp br;
    CHECK(rename_alloc(s, make_branch(), br), "branch rename failed");
    s.lsu.stq[0].valid=true; s.lsu.stq[0].branch_mask=bit(br.branch.br_tag); s.lsu.stq_count=1; s.lsu.stq_tail=1;
    resolve_branch(s, br, true);
    CHECK(valid_stq_count(s) == 0 && s.lsu.stq_count == 0, "STQ not cleared"); PASS(); }

void t18_wrong_path_store_no_request() { TEST("wrong-path store produces no committed dmem request");
    BoomCoreState s; PipeSignals p; MicroOp br,st;
    CHECK(rename_alloc(s, make_branch(), br), "branch rename failed");
    CHECK(rename_alloc(s, make_store(), st), "store rename failed");
    s.rob.entries[br.queue.rob_idx].busy=false;
    s.rob.entries[st.queue.rob_idx].busy=false;
    s.rob.entries[st.queue.rob_idx].is_store=true;
    s.rob.entries[st.queue.rob_idx].memory_valid=true;
    resolve_branch(s, br, true);
    boom::rob_commit_module(s, p);
    CHECK(p.dmem_req.empty(), "wrong-path store emitted dmem request"); PASS(); }

void t19_wrong_path_writeback_no_prf() { TEST("wrong-path writeback cannot pollute PRF");
    BoomCoreState s; MicroOp br,wrong;
    CHECK(rename_alloc(s, make_branch(), br), "branch rename failed");
    CHECK(rename_one(s, make_add(1), wrong), "wrong rename failed");
    s.brupdate.valid=true; s.brupdate.mispredict=true; s.brupdate.mispredict_mask=bit(br.branch.br_tag);
    s.issue.issued_valids[0]=true; s.issue.issued_uops[0]=wrong;
    boom::prf_seed(s,wrong.rename.pdst,0);
    boom::execute_module(s);
    CHECK(boom::prf_read(s,wrong.rename.pdst) == 0, "wrong-path PRF write occurred"); PASS(); }

void t20_wrong_path_cannot_commit() { TEST("wrong-path ROB entry cannot commit");
    BoomCoreState s; PipeSignals p; MicroOp br,wrong;
    CHECK(rename_alloc(s, make_branch(), br), "branch rename failed");
    CHECK(rename_alloc(s, make_add(1), wrong), "wrong rename failed");
    s.rob.entries[br.queue.rob_idx].busy=false;
    s.rob.entries[wrong.queue.rob_idx].busy=false;
    resolve_branch(s, br, true);
    boom::rob_commit_module(s, p);
    while(!p.commit_trace.empty()) { CommitEntry ce=p.commit_trace.read(); CHECK(ce.pc != wrong.debug_pc || ce.inst != wrong.inst, "wrong-path commit observed"); }
    CHECK(valid_rob_count(s) == 0, "branch should be only committed entry"); PASS(); }

void t21_stale_imem_response_rejected() { TEST("stale IMEM response rejected after redirect epoch");
    BoomCoreState s; PipeSignals p; MicroOp br;
    CHECK(rename_alloc(s, make_branch(), br), "branch rename failed");
    s.frontend.request_sent=true; s.frontend.pending_fetch_id=7;
    resolve_branch(s, br, true, RESET_VECTOR+64);
    ImemResponse stale; stale.fetch_id=7; stale.address=RESET_VECTOR; stale.instruction=0x00000073u; p.imem_resp.write(stale);
    boom::frontend_module(s, p);
    CHECK(!s.frontend.fetch_packet_valid, "stale response became fetch packet"); PASS(); }

void t22_mispredict_with_commit() { TEST("branch mispredict and commit same cycle keeps branch precise");
    BoomCoreState s; PipeSignals p; MicroOp br,wrong;
    CHECK(rename_alloc(s, make_branch(), br), "branch rename failed");
    CHECK(rename_alloc(s, make_add(1), wrong), "wrong rename failed");
    s.rob.entries[br.queue.rob_idx].busy=false;
    resolve_branch(s, br, true);
    boom::rob_commit_module(s, p);
    CHECK(s.rob.head == ((br.queue.rob_idx + 1) % ROB_DEPTH), "branch did not commit precisely"); PASS(); }

void t23_mispredict_with_writeback() { TEST("mispredict masks simultaneous younger writeback");
    BoomCoreState s; MicroOp br,wrong;
    CHECK(rename_alloc(s, make_branch(), br), "branch rename failed");
    CHECK(rename_one(s, make_add(1), wrong), "wrong rename failed");
    s.brupdate.valid=true; s.brupdate.mispredict=true; s.brupdate.mispredict_mask=bit(br.branch.br_tag);
    s.issue.issued_valids[0]=true; s.issue.issued_uops[0]=wrong;
    boom::prf_seed(s,wrong.rename.pdst,0);
    boom::execute_module(s);
    CHECK(boom::prf_read(s,wrong.rename.pdst) == 0, "younger writeback polluted PRF"); PASS(); }

void t24_mispredict_with_exception() { TEST("mispredict kills younger exception before trap");
    BoomCoreState s; MicroOp br,exc;
    CHECK(rename_alloc(s, make_branch(), br), "branch rename failed");
    exc = make_add(1); exc.exception=true; exc.exc_cause=2;
    CHECK(rename_alloc(s, exc, exc), "exception rename failed");
    resolve_branch(s, br, true);
    CHECK(!s.rob.entries[exc.queue.rob_idx].valid, "younger exception survived branch miss");
    CHECK(!s.io_trap, "younger exception set trap"); PASS(); }

void t25_consecutive_mispredicts() { TEST("multiple consecutive mispredicts do not leak tags");
    BoomCoreState s; MicroOp b0,b1,wrong;
    CHECK(rename_alloc(s, make_branch(), b0), "b0 rename failed");
    CHECK(rename_alloc(s, make_branch(), b1), "b1 rename failed");
    resolve_branch(s, b1, true);
    CHECK(s.branch_state.active_mask == bit(b0.branch.br_tag), "inner miss lost outer");
    CHECK(rename_alloc(s, make_add(2), wrong), "wrong rename failed");
    resolve_branch(s, b0, true);
    CHECK(s.branch_state.active_mask == 0, "outer miss leaked tag"); PASS(); }

void t26_free_list_near_exhaustion() { TEST("free-list near exhaustion rollback restores count");
    BoomCoreState s; MicroOp br,tmp;
    CHECK(rename_alloc(s, make_branch(), br), "branch rename failed");
    uint8_t before = s.rename.int_free_list.count;
    for (int i=0; i<40; i++) CHECK(rename_alloc(s, make_add((uint8_t)((i%31)+1)), tmp), "near-exhaust rename failed");
    CHECK(s.rename.int_free_list.count < before, "free-list did not drain");
    resolve_branch(s, br, true);
    CHECK(s.rename.int_free_list.count == before, "rollback did not restore near-exhaust count"); PASS(); }

void t27_rob_wrap_branch_recovery() { TEST("ROB wrap branch recovery resets tail after branch");
    BoomCoreState s; MicroOp br,wrong;
    s.rob.head = ROB_DEPTH-2; s.rob.tail = ROB_DEPTH-2; s.rob.maybe_full=false;
    CHECK(rename_alloc(s, make_branch(), br), "branch rename failed");
    CHECK(rename_alloc(s, make_add(1), wrong), "wrong rename failed");
    resolve_branch(s, br, true);
    CHECK(s.rob.tail == ((br.queue.rob_idx + 1) % ROB_DEPTH), "ROB tail not restored after wrap");
    CHECK(!s.rob.entries[wrong.queue.rob_idx].valid, "wrapped younger entry survived"); PASS(); }

void t28_iq_full_branch_recovery() { TEST("IQ full state clears all wrong-path entries");
    BoomCoreState s; MicroOp br;
    CHECK(rename_alloc(s, make_branch(), br), "branch rename failed");
    for (int i=0; i<ISSUE_QUEUE_ALU_DEPTH; i++) {
        s.issue.alu_iq.entries[i].valid=true; s.issue.alu_iq.entries[i].request=true;
        s.issue.alu_iq.entries[i].uop=make_add(1); s.issue.alu_iq.entries[i].uop.branch.br_mask=bit(br.branch.br_tag);
    }
    s.issue.alu_iq.count=ISSUE_QUEUE_ALU_DEPTH; s.issue.alu_iq.tail=0;
    resolve_branch(s, br, true);
    CHECK(s.issue.alu_iq.count == 0, "full IQ wrong-path entries survived"); PASS(); }

void t29_nested_branch_deterministic_stress() { TEST("nested branch deterministic stress preserves outer maps");
    BoomCoreState s; MicroOp b0,b1,a;
    CHECK(rename_alloc(s, make_add(1), a), "initial x1 rename failed");
    uint8_t x1 = s.rename.int_map_table.map_table[1];
    CHECK(rename_alloc(s, make_branch(), b0), "b0 rename failed");
    CHECK(rename_alloc(s, make_add(2), a), "x2 rename failed");
    CHECK(rename_alloc(s, make_branch(), b1), "b1 rename failed");
    CHECK(rename_alloc(s, make_add(1), a), "wrong x1 rename failed");
    resolve_branch(s, b1, true);
    CHECK(s.rename.int_map_table.map_table[1] == x1, "inner recovery lost outer x1 map");
    CHECK((s.branch_state.active_mask & bit(b0.branch.br_tag)) != 0, "outer tag lost"); PASS(); }

void t30_mask_tag_pressure_smoke() { TEST("branch mask/tag pressure smoke");
    BoomCoreState s; MicroOp brs[MAX_BRANCH_COUNT], tmp;
    uint8_t expected_mask = 0;
    for (int i=0; i<MAX_BRANCH_COUNT; i++) {
        CHECK(rename_alloc(s, make_branch(), brs[i]), "branch pressure alloc failed");
        CHECK(brs[i].branch.br_mask == expected_mask, "unexpected inherited mask");
        expected_mask |= bit(brs[i].branch.br_tag);
    }
    resolve_branch(s, brs[3], false);
    CHECK((s.branch_state.active_mask & bit(brs[3].branch.br_tag)) == 0, "resolved tag stayed active");
    CHECK(rename_alloc(s, make_branch(), tmp), "pressure reuse failed"); PASS(); }

int main() {
    printf("=== BOOM-HLS Gate 3.3 Branch Snapshot Tests ===\n\n");
    t1_single_branch_correct(); t2_single_branch_mispredict(); t3_two_nested_correct();
    t4_inner_mispredict(); t5_outer_mispredict(); t6_outer_mispredict_after_inner_resolved();
    t7_branch_tag_pool_exhaustion(); t8_allocate_release_same_cycle_model(); t9_branch_tag_wrap();
    t10_multi_rename_after_branch(); t11_map_table_restore(); t12_free_list_rollback();
    t13_busy_table_recovery(); t14_rob_young_clear(); t15_iq_young_clear();
    t16_ldq_young_clear(); t17_stq_young_clear(); t18_wrong_path_store_no_request();
    t19_wrong_path_writeback_no_prf(); t20_wrong_path_cannot_commit(); t21_stale_imem_response_rejected();
    t22_mispredict_with_commit(); t23_mispredict_with_writeback(); t24_mispredict_with_exception();
    t25_consecutive_mispredicts(); t26_free_list_near_exhaustion(); t27_rob_wrap_branch_recovery();
    t28_iq_full_branch_recovery(); t29_nested_branch_deterministic_stress(); t30_mask_tag_pressure_smoke();
    printf("\n=== %d passed, %d failed ===\n", tests_passed, tests_failed);
    return tests_failed ? 1 : 0;
}
