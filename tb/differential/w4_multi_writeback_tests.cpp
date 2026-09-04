#include "completion.hpp"
#include "reset.hpp"
#include <cstdio>

namespace boom { void rob_commit_module(BoomCoreState&, PipeSignals&); }

static int passed, failed;
static unsigned peak_accepts, peak_rob, peak_writes, peak_wakeups, max_wait;
static uint64_t measured_completion_drops, measured_drops, measured_duplicates, measured_conflicts;
static uint64_t measured_faults, measured_fault_events, measured_deduplications;
static uint64_t injected_completion_drop_probe;
#define TEST(n) std::printf("  [W4D writeback] %-42s ... ", n)
#define CHECK(c,m) do { if (!(c)) { std::printf("FAIL: %s\n",m); failed++; return; } } while (0)
#define PASS() do { std::printf("PASS\n"); passed++; } while (0)

static void owner(BoomCoreState& s, uint8_t i, uint32_t a, uint8_t p) {
    RobEntry& e=s.rob.entries[i]; e=RobEntry(); e.valid=e.busy=true;
    e.uop.uopc=1; e.uop.queue.rob_idx=i; e.uop.queue.rob_allocation_id=a;
    e.uop.rename.pdst=p; e.uop.rename.dst_rtype=p ? DST_INT : DST_X0;
    if (p) s.rename.int_free_list.busy_table[p]=true;
}
static ExecuteState::AluResult result(const BoomCoreState& s,uint8_t i,uint64_t v) {
    ExecuteState::AluResult r; r.valid=true; r.uop=s.rob.entries[i].uop; r.result=v; return r;
}
static void observe(const BoomCoreState& s) {
    if(s.completion.peak_completion_accepts>peak_accepts) peak_accepts=s.completion.peak_completion_accepts;
    if(s.completion.peak_rob_completes>peak_rob) peak_rob=s.completion.peak_rob_completes;
    if(s.completion.peak_prf_writes>peak_writes) peak_writes=s.completion.peak_prf_writes;
    if(s.completion.peak_wakeups>peak_wakeups) peak_wakeups=s.completion.peak_wakeups;
    if(s.completion.dropped_completions>measured_completion_drops) measured_completion_drops=s.completion.dropped_completions;
    if(s.completion.dropped_writebacks>measured_drops) measured_drops=s.completion.dropped_writebacks;
    if(s.completion.duplicate_writebacks>measured_duplicates) measured_duplicates=s.completion.duplicate_writebacks;
    if(s.completion.writeback_conflicts>measured_conflicts) measured_conflicts=s.completion.writeback_conflicts;
    if(s.completion.writeback_validation_faults>measured_faults) measured_faults=s.completion.writeback_validation_faults;
    if(s.completion.writeback_fault_events>measured_fault_events) measured_fault_events=s.completion.writeback_fault_events;
    if(s.completion.writeback_deduplications>measured_deduplications) measured_deduplications=s.completion.writeback_deduplications;
}
static void two(BoomCoreState& s,uint8_t a=1,uint8_t b=2,uint8_t pa=10,uint8_t pb=11) {
    s.rob.head=a; owner(s,a,101,pa); owner(s,b,102,pb);
    s.execute.alu_results[MEM_ISSUE_LANE]=result(s,a,0xaaaa);
    s.execute.alu_results[INT_ISSUE_LANE]=result(s,b,0xbbbb);
}
static void seed_load(BoomCoreState& s,PipeSignals& p,uint8_t i,uint32_t a,uint8_t pdst,uint32_t tx,uint64_t value) {
    owner(s,i,a,pdst); RobEntry& e=s.rob.entries[i]; e.is_load=e.memory_valid=e.memory_request_sent=true;
    e.memory_transaction_id=tx; e.memory_size=3; s.lsu.load_response_pending=true;
    s.lsu.pending_load_transaction_id=tx; s.lsu.pending_load_rob_idx=i; s.lsu.pending_load_allocation_id=a;
    s.lsu.ldq_count=1; s.lsu.ldq[0].valid=true; s.lsu.ldq[0].rob_idx=i; s.lsu.ldq[0].rob_allocation_id=a;
    DmemResponse d; d.transaction_id=tx; d.data=d.read_data=value; p.dmem_resp.write(d);
}

static void t01(){TEST("fixed two-port topology");CHECK(NUM_INT_WRITEBACK_PORTS==2&&INT_PHYS_REGS==52&&COMMIT_WIDTH==1,"topology changed");PASS();}
static void t02(){TEST("same-parity destinations use independent replica ports");BoomCoreState s;two(s,1,2,10,12);boom::completion_service_execute(s);observe(s);CHECK(boom::prf_read(s,10)==0xaaaa&&boom::prf_read(s,12)==0xbbbb&&s.int_rf_bank0[10]==0xaaaa&&s.int_rf_bank1[12]==0xbbbb&&((s.int_rf_latest_bank>>10)&1ULL)==0&&((s.int_rf_latest_bank>>12)&1ULL)==1&&s.completion.prf_writes_this_cycle==2&&s.completion.writebacks[0].valid&&s.completion.writebacks[1].valid&&!s.rob.entries[1].busy&&!s.rob.entries[2].busy,"dual-bank LVT write failed");PASS();}
static void t03(){TEST("INT plus load dual writeback");BoomCoreState s;PipeSignals p;s.rob.head=1;seed_load(s,p,1,201,12,7,0x1212);owner(s,2,202,13);s.execute.alu_results[INT_ISSUE_LANE]=result(s,2,0x1313);boom::completion_service_cycle(s,p);observe(s);CHECK(boom::prf_read(s,12)==0x1212&&boom::prf_read(s,13)==0x1313&&s.completion.prf_writes_this_cycle==2&&!s.lsu.load_response_pending,"load/INT did not dual write");PASS();}
static void t04(){TEST("same pdst same value de-duplicates");BoomCoreState s;two(s,1,2,14,14);s.execute.alu_results[0].result=9;s.execute.alu_results[1].result=9;boom::completion_service_execute(s);observe(s);CHECK(boom::prf_read(s,14)==9&&s.completion.prf_writes_this_cycle==1&&s.completion.rob_completes_this_cycle==2&&!s.rob.entries[1].busy&&!s.rob.entries[2].busy&&!s.completion.writeback_conflict,"safe dedup failed");PASS();}
static void t05(){TEST("same pdst conflict becomes precise fault");BoomCoreState s;two(s,1,2,15,15);s.rob.tail=3;boom::completion_service_execute(s);observe(s);CHECK(boom::prf_read(s,15)==0&&s.completion.prf_writes_this_cycle==0&&s.completion.wakeups_this_cycle==0&&s.completion.writeback_conflict&&s.completion.writeback_fault_valid&&s.completion.writeback_fault_rob_idx==1&&s.completion.writeback_fault_cause==WRITEBACK_VALIDATION_FAULT_CAUSE&&s.completion.writeback_conflicts==1&&s.completion.writeback_validation_faults==1&&s.completion.writeback_fault_events==2&&!s.rob.entries[1].busy&&!s.rob.entries[2].busy&&s.rob.entries[1].exception&&s.rob.entries[2].exception&&!s.completion.mem_execute.valid&&!s.completion.int_execute.valid,"conflict fault was not precise and recoverable");PipeSignals p;boom::rob_commit_module(s,p);CHECK(!s.io_trap&&s.rob.state==ROB_NORMAL&&s.exception_commit.valid&&s.frontend_redirect.valid&&!p.commit_trace.empty(),"fault did not enter recoverable architectural trap");CommitEntry ce=p.commit_trace.read();CHECK(ce.exception&&ce.exc_cause==WRITEBACK_VALIDATION_FAULT_CAUSE,"fault trace missing cause");boom::rob_commit_module(s,p);CHECK(p.commit_trace.empty(),"fault trace repeated");ResetControllerState r;for(int i=0;i<200&&!r.completed;i++)boom_core_reset_step(s,r);CHECK(r.completed&&!s.completion.writeback_fault_valid&&!s.completion.writeback_conflict&&!s.io_trap,"reset did not recover validation fault");PASS();}
static void t06(){TEST("fault persistently suppresses younger publication");BoomCoreState s;s.rob.head=1;owner(s,1,1,16);owner(s,2,2,16);owner(s,3,3,17);s.completion.load_response.valid=true;s.completion.load_response.kind=COMPLETION_EXECUTE;s.completion.load_response.source=COMPLETION_SOURCE_LSU_LOAD;s.completion.load_response.uop=s.rob.entries[1].uop;s.completion.load_response.writes_prf=true;s.completion.load_response.value=1;s.execute.alu_results[0]=result(s,2,2);s.execute.alu_results[1]=result(s,3,3);boom::completion_service_execute(s);observe(s);CHECK(boom::prf_read(s,16)==0&&boom::prf_read(s,17)==0&&s.completion.prf_writes_this_cycle==0&&s.completion.wakeups_this_cycle==0&&s.completion.bypass_this_cycle==0&&s.completion.int_execute.valid&&s.rob.entries[3].busy,"fault cycle published unrelated younger writer");for(int cycle=0;cycle<2;cycle++){boom::completion_service_execute(s);CHECK(boom::prf_read(s,17)==0&&s.completion.int_execute.valid&&s.rob.entries[3].busy&&s.completion.prf_writes_this_cycle==0&&s.completion.wakeups_this_cycle==0&&s.completion.bypass_this_cycle==0,"younger writer crossed persistent precise fault fence");}PASS();}
static void t07(){TEST("x0 never writes");BoomCoreState s;boom::prf_force_x0(s);owner(s,1,1,0);s.execute.alu_results[1]=result(s,1,~0ULL);boom::completion_service_execute(s);CHECK(boom::prf_read(s,0)==0&&s.completion.prf_writes_this_cycle==0&&!s.rob.entries[1].busy,"x0 write occurred");PASS();}
static void t08(){TEST("ROB wrap deterministic age");BoomCoreState s;two(s,31,0,18,19);boom::completion_service_execute(s);CHECK(s.completion.writebacks[0].rob_idx==31&&s.completion.writebacks[1].rob_idx==0&&boom::prf_read(s,18)==0xaaaa&&boom::prf_read(s,19)==0xbbbb,"wrap ordering failed");PASS();}
static void t09(){TEST("three writers retain bounded oldest");BoomCoreState s;s.rob.head=1;owner(s,1,1,20);owner(s,2,2,21);owner(s,3,3,22);s.completion.load_response.valid=true;s.completion.load_response.kind=COMPLETION_EXECUTE;s.completion.load_response.source=COMPLETION_SOURCE_LSU_LOAD;s.completion.load_response.uop=s.rob.entries[1].uop;s.completion.load_response.writes_prf=true;s.completion.load_response.value=20;s.execute.alu_results[0]=result(s,2,21);s.execute.alu_results[1]=result(s,3,22);unsigned cycles=0;while(cycles<3&&(s.rob.entries[1].busy||s.rob.entries[2].busy||s.rob.entries[3].busy)){boom::completion_service_execute(s);cycles++;}max_wait=cycles;observe(s);CHECK(cycles==2&&boom::prf_read(s,20)==20&&boom::prf_read(s,21)==21&&boom::prf_read(s,22)==22&&s.completion.total_prf_writes==3&&s.completion.total_rob_completes==3,"retention/drop/duplicate failure");PASS();}
static void t10(){TEST("lane2 invalid and reset clears evidence");BoomCoreState s;owner(s,1,1,23);s.execute.alu_results[FP_ISSUE_LANE]=result(s,1,23);boom::completion_service_execute(s);CHECK(s.rob.entries[1].busy&&!s.execute.alu_results[FP_ISSUE_LANE].valid,"lane2 completed");s.completion.writeback_conflict=true;ResetControllerState r;boom_core_reset_step(s,r);CHECK(!s.completion.writeback_conflict&&!s.completion.writebacks[0].valid,"reset retained W4D state");PASS();}
static void seed_cross_branch(BoomCoreState& s, bool mispredict) { s.rob.head=1;owner(s,1,301,24);owner(s,2,302,0);owner(s,3,303,24);s.rob.tail=4;s.branch_state.active_mask=1;s.branch_state.tag_valid[0]=s.branch_state.snapshot_valid[0]=true;s.completion.load_response.valid=true;s.completion.load_response.kind=COMPLETION_EXECUTE;s.completion.load_response.source=COMPLETION_SOURCE_LSU_LOAD;s.completion.load_response.uop=s.rob.entries[1].uop;s.completion.load_response.writes_prf=true;s.completion.load_response.value=1;s.execute.alu_results[INT_ISSUE_LANE]=result(s,2,0);s.execute.alu_results[INT_ISSUE_LANE].uop.branch.is_br=true;s.execute.alu_results[INT_ISSUE_LANE].uop.branch.br_tag=0;s.execute.alu_results[INT_ISSUE_LANE].mispredict=mispredict;s.execute.alu_results[MEM_ISSUE_LANE]=result(s,3,2);s.execute.alu_results[MEM_ISSUE_LANE].uop.branch.br_mask=1;}
static void t11(){TEST("correct branch preserves cross-boundary conflict");BoomCoreState s;seed_cross_branch(s,false);boom::completion_service_execute(s);observe(s);CHECK(s.brupdate.valid&&!s.brupdate.mispredict&&s.completion.writeback_fault_valid&&s.completion.writeback_fault_rob_idx==1&&boom::prf_read(s,24)==0&&s.completion.prf_writes_this_cycle==0&&s.completion.wakeups_this_cycle==0&&!s.completion.load_response.valid&&!s.completion.mem_execute.valid&&s.completion.int_execute.valid,"correct branch lost or published cross-boundary conflict");PASS();}
static void t12(){TEST("mispredict kills younger false conflict");BoomCoreState s;seed_cross_branch(s,true);boom::completion_service_execute(s);observe(s);CHECK(s.brupdate.valid&&s.brupdate.mispredict&&!s.completion.writeback_fault_valid&&s.completion.writeback_conflicts==0&&boom::prf_read(s,24)==1&&s.completion.prf_writes_this_cycle==1&&!s.rob.entries[3].valid&&!s.completion.mem_execute.valid,"killed younger writer caused conflict");PASS();}
static void t13(){TEST("duplicate held source increments drop invariant");BoomCoreState s;s.rob.head=1;owner(s,1,401,25);s.completion.mem_execute.valid=true;s.completion.mem_execute.kind=COMPLETION_EXECUTE;s.completion.mem_execute.source=COMPLETION_SOURCE_MEM_EXECUTE;s.completion.mem_execute.uop=s.rob.entries[1].uop;s.completion.mem_execute.writes_prf=true;s.completion.mem_execute.value=25;s.execute.alu_results[MEM_ISSUE_LANE]=result(s,1,25);boom::completion_service_execute(s);injected_completion_drop_probe=s.completion.dropped_completions;CHECK(s.completion.dropped_completions==1&&!s.execute.alu_results[MEM_ISSUE_LANE].valid&&boom::prf_read(s,25)==25,"hold violation was not counted and contained");PASS();}

int main(){std::printf("=== Gate 4.0 W4D Multi Writeback Tests ===\n");t01();t02();t03();t04();t05();t06();t07();t08();t09();t10();t11();t12();t13();std::printf("W4D multi writeback: %d passed, %d failed\n",passed,failed);std::printf("W4D_WRITEBACK_METRICS peak_completion_accepts=%u peak_rob_completes=%u peak_prf_writes=%u peak_wakeups=%u bounded_wait=%u completion_drops=%llu writeback_drops=%llu duplicate_writes=%llu conflicts=%llu validation_faults=%llu fault_events=%llu deduplications=%llu hold_violation_probe=%llu\n",peak_accepts,peak_rob,peak_writes,peak_wakeups,max_wait,(unsigned long long)measured_completion_drops,(unsigned long long)measured_drops,(unsigned long long)measured_duplicates,(unsigned long long)measured_conflicts,(unsigned long long)measured_faults,(unsigned long long)measured_fault_events,(unsigned long long)measured_deduplications,(unsigned long long)injected_completion_drop_probe);return failed?1:0;}
