#include "completion.hpp"
#include "reset.hpp"
#include <cstdio>

namespace boom { void issue_module(BoomCoreState&); }

static int passed, failed;
static uint8_t observed_peak_wakeups, observed_peak_prf, observed_bounded_wait;
#define TEST(n) std::printf("  [W4C wakeup] %-42s ... ", n)
#define CHECK(c,m) do { if (!(c)) { std::printf("FAIL: %s\n",m); failed++; return; } } while (0)
#define PASS() do { std::printf("PASS\n"); passed++; } while (0)

static void owner(BoomCoreState& s, uint8_t idx, uint32_t alloc,
                  uint8_t pdst, uint8_t mask=0) {
    RobEntry& e=s.rob.entries[idx]; e=RobEntry(); e.valid=e.busy=true;
    e.uop.uopc=1; e.uop.iq_type=IQ_ALU; e.uop.fu_code=FU_ALU;
    e.uop.queue.rob_idx=idx; e.uop.queue.rob_allocation_id=alloc;
    e.uop.rename.pdst=pdst; e.uop.rename.dst_rtype=pdst ? DST_INT : DST_N;
    e.uop.branch.br_mask=mask; s.rename.int_free_list.busy_table[pdst]=pdst!=0;
}

static ExecuteState::AluResult result(const BoomCoreState& s, uint8_t idx,
                                      uint64_t value) {
    ExecuteState::AluResult r; r.valid=true; r.uop=s.rob.entries[idx].uop;
    r.result=value; return r;
}

static void seed_two(BoomCoreState& s) {
    s.rob.head=1; owner(s,1,101,10); owner(s,2,102,11);
    s.execute.alu_results[MEM_ISSUE_LANE]=result(s,1,0xaaaa);
    s.execute.alu_results[INT_ISSUE_LANE]=result(s,2,0xbbbb);
}

static IssueSlotEntry consumer(uint8_t port, uint8_t prs1, uint8_t prs2=0,
                               uint8_t prs3=0) {
    IssueSlotEntry e; e.valid=e.request=true; e.uop.uopc=port==ISSUE_PORT_MEM?46:1;
    e.uop.iq_type=port==ISSUE_PORT_MEM?IQ_MEM:IQ_ALU;
    e.uop.fu_code=port==ISSUE_PORT_MEM?FU_MEM:FU_ALU;
    e.uop.rename.prs1=prs1; e.uop.rename.prs2=prs2; e.uop.rename.prs3=prs3;
    return e;
}

static void t01() { TEST("fixed real topology"); CHECK(NUM_INT_WAKEUP_PORTS==3 && NUM_INT_BYPASS_PORTS==3 && COMMIT_WIDTH==1,"topology changed"); PASS(); }
static void t02() { TEST("two producers wake and dual-write PRF"); BoomCoreState s; seed_two(s); boom::completion_service_execute(s); observed_peak_wakeups=s.completion.peak_wakeups; observed_peak_prf=s.completion.peak_prf_writes; CHECK(s.completion.wakeups_this_cycle==2 && s.completion.peak_wakeups==2 && s.completion.prf_writes_this_cycle==2 && boom::prf_read(s,10)==0xaaaa && boom::prf_read(s,11)==0xbbbb && !s.rename.int_free_list.busy_table[11],"multi-wakeup/dual-write contract failed"); PASS(); }
static void t03() { TEST("identical pdst/value de-duplicates"); BoomCoreState s; owner(s,1,1,12); owner(s,2,2,12); s.execute.alu_results[0]=result(s,1,7); s.execute.alu_results[1]=result(s,2,7); boom::completion_service_execute(s); CHECK(s.completion.wakeups_this_cycle==1 && s.completion.wakeup_conflicts==0,"identical events not de-duplicated"); PASS(); }
static void t04() { TEST("conflicting same pdst faults without wakeup"); BoomCoreState s; owner(s,1,1,13); owner(s,2,2,13); s.execute.alu_results[0]=result(s,1,7); s.execute.alu_results[1]=result(s,2,8); boom::completion_service_execute(s); CHECK(s.completion.wakeups_this_cycle==0 && s.completion.bypass_this_cycle==0 && s.completion.wakeup_conflicts==1 && s.completion.writeback_conflict && s.completion.writeback_fault_valid && s.completion.prf_writes_this_cycle==0 && boom::prf_read(s,13)==0 && !s.completion.mem_execute.valid && !s.completion.int_execute.valid && s.rob.entries[1].exception,"conflict was forwarded, written, or left deadlocked"); PASS(); }
static void t05() { TEST("two producers to two consumers"); BoomCoreState s; seed_two(s); boom::completion_service_execute(s); s.issue.alu_iq.entries[0]=consumer(ISSUE_PORT_MEM,10); s.issue.alu_iq.entries[1]=consumer(ISSUE_PORT_INT,11); s.issue.alu_iq.count=2; s.issue.alu_iq.tail=2; boom::issue_module(s); CHECK(s.issue.grants_accepted==2 && s.issue.issued_prs1_data[0]==0xaaaa && s.issue.issued_prs1_data[1]==0xbbbb,"dual consumer data mismatch"); PASS(); }
static void t06() { TEST("one producer wakes many consumers"); BoomCoreState s; owner(s,1,1,14); s.execute.alu_results[1]=result(s,1,0x1414); boom::completion_service_execute(s); for(int i=0;i<3;i++) s.issue.alu_iq.entries[i]=consumer(ISSUE_PORT_INT,14); s.issue.alu_iq.count=3; s.issue.alu_iq.tail=3; s.issue.port_ready[INT_ISSUE_LANE]=false; boom::issue_module(s); CHECK(s.issue.alu_iq.count==3 && !s.issue.alu_iq.entries[0].prs1_busy && !s.issue.alu_iq.entries[1].prs1_busy && !s.issue.alu_iq.entries[2].prs1_busy && s.issue.alu_iq.entries[2].prs1_data==0x1414,"fanout did not latch exact data"); PASS(); }
static void t07() { TEST("prs1 prs2 prs3 and x0"); BoomCoreState s; seed_two(s); boom::completion_service_execute(s); s.issue.alu_iq.entries[0]=consumer(ISSUE_PORT_INT,0,10,11); s.issue.alu_iq.count=1; s.issue.alu_iq.tail=1; s.issue.port_ready[INT_ISSUE_LANE]=false; boom::issue_module(s); const IssueSlotEntry& e=s.issue.alu_iq.entries[0]; CHECK(!e.prs1_busy && !e.prs2_busy && !e.prs3_busy && e.prs1_data==0 && e.prs2_data==0xaaaa && e.prs3_data==0xbbbb,"operand wakeup mismatch"); PASS(); }
static void t08() { TEST("late dispatch sees completed second write"); BoomCoreState s; seed_two(s); boom::completion_service_execute(s); CHECK(boom::prf_read(s,11)==0xbbbb && !s.rename.int_free_list.busy_table[11] && !s.completion.int_execute.valid,"second writer did not complete"); RenameDispatchPacket& p=s.rename.dispatch_packets[0]; p.valid=p.rob_allocated=true; p.uop.uopc=1; p.uop.iq_type=IQ_ALU; p.uop.fu_code=FU_ALU; p.uop.rename.prs1=11; boom::issue_module(s); CHECK(s.issue.issued_valids[INT_ISSUE_LANE] && s.issue.issued_prs1_data[INT_ISSUE_LANE]==0xbbbb,"late dispatch read stale PRF"); PASS(); }
static void t09() { TEST("stale allocation produces no wakeup"); BoomCoreState s; owner(s,1,9,15); ExecuteState::AluResult r=result(s,1,9); r.uop.queue.rob_allocation_id=8; s.execute.alu_results[1]=r; boom::completion_service_execute(s); CHECK(s.completion.wakeups_this_cycle==0 && s.rob.entries[1].busy,"stale result forwarded"); PASS(); }
static void t10() { TEST("mispredict kills younger transient ports"); BoomCoreState s; s.rob.head=1; owner(s,1,1,0); owner(s,2,2,16,1); s.branch_state.active_mask=1; s.branch_state.tag_valid[0]=s.branch_state.snapshot_valid[0]=true; s.execute.alu_results[1]=result(s,1,0); s.execute.alu_results[1].uop.branch.is_br=true; s.execute.alu_results[1].uop.branch.br_tag=0; s.execute.alu_results[1].mispredict=true; s.execute.alu_results[0]=result(s,2,0x16); boom::completion_service_execute(s); CHECK(!s.rob.entries[2].valid && !s.completion.wakeups[0].valid && !s.completion.bypass[0].valid,"killed result remained visible"); PASS(); }
static void t11() { TEST("three writers drain without starvation"); BoomCoreState s; s.rob.head=1; owner(s,1,1,17); owner(s,2,2,18); owner(s,3,3,19); s.execute.alu_results[0]=result(s,2,18); s.execute.alu_results[1]=result(s,3,19); s.completion.load_response.valid=true; s.completion.load_response.kind=COMPLETION_EXECUTE; s.completion.load_response.source=COMPLETION_SOURCE_LSU_LOAD; s.completion.load_response.uop=s.rob.entries[1].uop; s.completion.load_response.writes_prf=true; s.completion.load_response.value=17; uint8_t cycles=0; while(cycles<4 && (s.completion.load_response.valid || s.completion.mem_execute.valid || s.completion.int_execute.valid || s.execute.alu_results[0].valid || s.execute.alu_results[1].valid)){ boom::completion_service_execute(s); cycles++; } observed_bounded_wait=cycles; if(s.completion.peak_wakeups>observed_peak_wakeups) observed_peak_wakeups=s.completion.peak_wakeups; if(s.completion.peak_prf_writes>observed_peak_prf) observed_peak_prf=s.completion.peak_prf_writes; CHECK(boom::prf_read(s,17)==17 && boom::prf_read(s,18)==18 && boom::prf_read(s,19)==19 && !s.completion.load_response.valid && !s.completion.mem_execute.valid && !s.completion.int_execute.valid && s.completion.peak_prf_writes==2 && cycles==2,"writer dropped, duplicated, or starved"); PASS(); }
static void t12() { TEST("reset clears transient and sent state"); BoomCoreState s; seed_two(s); boom::completion_service_execute(s); ResetControllerState r; boom_core_reset_step(s,r); CHECK(s.completion.wakeups_this_cycle==0 && s.completion.bypass_this_cycle==0 && !s.completion.wakeups[0].valid && !s.completion.wakeup_sent[0],"reset retained W4C state"); PASS(); }
static void t13() { TEST("precise fence blocks younger wakeup"); BoomCoreState s; s.rob.head=1; owner(s,1,1,21); owner(s,2,2,22); owner(s,3,3,23); s.completion.load_response.valid=true; s.completion.load_response.kind=COMPLETION_EXECUTE; s.completion.load_response.source=COMPLETION_SOURCE_LSU_LOAD; s.completion.load_response.uop=s.rob.entries[1].uop; s.completion.load_response.writes_prf=true; s.completion.load_response.value=21; s.execute.alu_results[INT_ISSUE_LANE]=result(s,2,22); s.execute.alu_results[INT_ISSUE_LANE].exception=true; s.execute.alu_results[MEM_ISSUE_LANE]=result(s,3,23); boom::completion_service_execute(s); CHECK(s.completion.wakeups_this_cycle==1 && s.completion.wakeups[0].pdst==21 && !s.completion.int_execute.valid && s.rob.entries[2].exception && s.completion.mem_execute.valid && boom::prf_read(s,23)==0,"younger result crossed precise fence"); PASS(); }
static void t14() { TEST("resolved branch defers younger wake/write"); BoomCoreState s; s.rob.head=1; owner(s,1,1,0); owner(s,2,2,24); s.branch_state.active_mask=1; s.branch_state.tag_valid[0]=true; s.execute.alu_results[INT_ISSUE_LANE]=result(s,1,0); s.execute.alu_results[INT_ISSUE_LANE].uop.branch.is_br=true; s.execute.alu_results[INT_ISSUE_LANE].uop.branch.br_tag=0; s.execute.alu_results[MEM_ISSUE_LANE]=result(s,2,24); boom::completion_service_execute(s); CHECK(s.brupdate.valid&&!s.brupdate.mispredict&&s.completion.mem_execute.valid&&s.completion.wakeups_this_cycle==0&&boom::prf_read(s,24)==0&&s.rob.entries[2].busy,"younger writer crossed branch cycle"); boom::completion_service_execute(s); CHECK(!s.completion.mem_execute.valid&&s.completion.wakeups_this_cycle==1&&s.completion.wakeups[0].pdst==24&&boom::prf_read(s,24)==24&&!s.rob.entries[2].busy,"deferred writer missed wakeup before write"); PASS(); }

int main(){ std::printf("=== Gate 4.0 W4C Multi Wakeup Tests ===\n"); t01();t02();t03();t04();t05();t06();t07();t08();t09();t10();t11();t12();t13();t14(); std::printf("W4C multi wakeup: %d passed, %d failed\n",passed,failed); std::printf("W4C_WAKEUP_METRICS peak_wakeups=%u peak_prf_writes=%u bounded_wait=%u\n",observed_peak_wakeups,observed_peak_prf,observed_bounded_wait); return failed?1:0; }
