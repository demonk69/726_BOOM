#include "completion.hpp"
#include "boom_interfaces.hpp"
#include <cstdio>

extern void boom_core_step(BoomCoreState&, PipeSignals&);

static int passed,failed;
#define TEST(n) std::printf("  [W4D completion] %-41s ... ",n)
#define CHECK(c,m) do{if(!(c)){std::printf("FAIL: %s\n",m);failed++;return;}}while(0)
#define PASS() do{std::printf("PASS\n");passed++;}while(0)
static void owner(BoomCoreState&s,uint8_t i,uint32_t a,uint8_t p=0){RobEntry&e=s.rob.entries[i];e=RobEntry();e.valid=e.busy=true;e.uop.uopc=1;e.uop.queue.rob_idx=i;e.uop.queue.rob_allocation_id=a;e.uop.rename.pdst=p;e.uop.rename.dst_rtype=p?DST_INT:DST_N;if(p)s.rename.int_free_list.busy_table[p]=true;}
static ExecuteState::AluResult result(const BoomCoreState&s,uint8_t i,uint64_t v=0){ExecuteState::AluResult r;r.valid=true;r.uop=s.rob.entries[i].uop;r.result=v;return r;}
static void t01(){TEST("ALU and store multi-complete");BoomCoreState s;s.rob.head=1;owner(s,1,1,24);owner(s,2,2);s.execute.alu_results[1]=result(s,1,24);s.execute.alu_results[0]=result(s,2);s.execute.alu_results[0].is_store=s.execute.alu_results[0].memory_valid=true;boom::completion_service_execute(s);CHECK(s.completion.rob_completes_this_cycle==2&&s.completion.completion_accepts_this_cycle==2&&s.lsu.stq_count==1&&boom::prf_read(s,24)==24,"ALU/store failed");PASS();}
static void t02(){TEST("branch fences load AGU for one cycle");BoomCoreState s;s.rob.head=1;owner(s,1,1);owner(s,2,2,25);s.branch_state.active_mask=1;s.branch_state.tag_valid[0]=true;s.execute.alu_results[1]=result(s,1);s.execute.alu_results[1].uop.branch.is_br=true;s.execute.alu_results[1].uop.branch.br_tag=0;s.execute.alu_results[0]=result(s,2);s.execute.alu_results[0].is_load=s.execute.alu_results[0].memory_valid=true;boom::completion_service_execute(s);CHECK(s.brupdate.valid&&!s.rob.entries[1].busy&&s.rob.entries[2].busy&&!s.rob.entries[2].memory_valid&&s.completion.completion_accepts_this_cycle==1&&s.completion.mem_execute.valid,"load AGU crossed branch cycle");boom::completion_service_execute(s);CHECK(s.rob.entries[2].memory_valid&&!s.completion.mem_execute.valid,"deferred load AGU failed");PASS();}
static void t03(){TEST("backpressure does not repeat completion");BoomCoreState s;s.rob.head=1;s.rob.tail=3;owner(s,1,1,26);owner(s,2,2,27);s.execute.alu_results[0]=result(s,1,26);s.execute.alu_results[1]=result(s,2,27);boom::completion_service_execute(s);uint64_t total=s.completion.total_rob_completes;boom::completion_service_execute(s);CHECK(total==2&&s.completion.total_rob_completes==2&&s.completion.rob_completes_this_cycle==0,"completion repeated");PASS();}
static void t04(){TEST("oldest mispredict kills younger writer");BoomCoreState s;s.rob.head=1;s.rob.tail=3;owner(s,1,1);owner(s,2,2,28);s.branch_state.active_mask=1;s.branch_state.tag_valid[0]=s.branch_state.snapshot_valid[0]=true;s.execute.alu_results[1]=result(s,1);s.execute.alu_results[1].uop.branch.is_br=true;s.execute.alu_results[1].uop.branch.br_tag=0;s.execute.alu_results[1].mispredict=true;s.execute.alu_results[0]=result(s,2,28);s.execute.alu_results[0].uop.branch.br_mask=1;boom::completion_service_execute(s);CHECK(s.brupdate.mispredict&&!s.rob.entries[2].valid&&boom::prf_read(s,28)==0&&s.completion.rob_completes_this_cycle==1,"younger crossed branch fence");PASS();}
static void t05(){TEST("oldest exception records before younger");BoomCoreState s;s.rob.head=1;owner(s,1,1,29);owner(s,2,2,30);s.execute.alu_results[1]=result(s,1,29);s.execute.alu_results[1].exception=true;s.execute.alu_results[1].exc_cause=13;s.execute.alu_results[0]=result(s,2,30);boom::completion_service_execute(s);CHECK(s.rob.entries[1].exception&&s.rob.entries[1].uop.exc_cause==13&&s.completion.writebacks[0].rob_idx==1,"exception age order failed");PASS();}
static void t06(){TEST("equal-age source priority stable");BoomCoreState s;s.rob.head=1;owner(s,1,1,31);s.completion.mem_execute.valid=true;s.completion.mem_execute.kind=COMPLETION_EXECUTE;s.completion.mem_execute.source=COMPLETION_SOURCE_MEM_EXECUTE;s.completion.mem_execute.uop=s.rob.entries[1].uop;s.completion.mem_execute.writes_prf=true;s.completion.mem_execute.value=31;s.completion.int_execute=s.completion.mem_execute;s.completion.int_execute.source=COMPLETION_SOURCE_INT_EXECUTE;boom::completion_service_execute(s);CHECK(s.completion.writebacks[0].source==COMPLETION_SOURCE_MEM_EXECUTE&&s.completion.rob_completes_this_cycle==1,"source priority changed");PASS();}
static void t07(){
    TEST("core step retains queued load response");
    BoomCoreState s; PipeSignals p; s.rob.head=0; s.rob.tail=5;
    owner(s,1,701,14); owner(s,2,702,15); owner(s,3,703,16); owner(s,4,704,17);
    s.completion.load_response.valid=true;
    s.completion.load_response.kind=COMPLETION_EXECUTE;
    s.completion.load_response.source=COMPLETION_SOURCE_LSU_LOAD;
    s.completion.load_response.uop=s.rob.entries[1].uop;
    s.completion.load_response.writes_prf=true;
    s.completion.load_response.value=0x1414;
    s.execute.alu_results[MEM_ISSUE_LANE]=result(s,2,0x1515);
    s.execute.alu_results[INT_ISSUE_LANE]=result(s,3,0x1616);
    RobEntry& load=s.rob.entries[4];
    load.is_load=load.memory_valid=load.memory_request_sent=true;
    load.memory_size=3; load.memory_mask=0xff; load.memory_transaction_id=77;
    s.lsu.load_response_pending=true;
    s.lsu.pending_load_transaction_id=77;
    s.lsu.pending_load_rob_idx=4;
    s.lsu.pending_load_allocation_id=704;
    s.lsu.ldq_count=1; s.lsu.ldq_tail=1; s.lsu.ldq[0].valid=true;
    s.lsu.ldq[0].rob_idx=4; s.lsu.ldq[0].rob_allocation_id=704;
    DmemResponse response; response.transaction_id=77;
    response.data=response.read_data=0x1717; p.dmem_resp.write(response);
    s.completion.total_completion_accepts=40;
    s.completion.total_prf_writes=40;
    s.completion.total_wakeups=40;
    boom_core_step(s,p);
    CHECK(!p.dmem_resp.empty()&&s.completion.prf_writes_this_cycle==2&&
          s.completion.wakeups_this_cycle==3&&s.completion.total_completion_accepts==42&&
          s.completion.total_prf_writes==42&&s.completion.total_wakeups==43&&
          !s.completion.load_response.valid&&s.completion.int_execute.valid,
          "first step serviced completion twice or lost queued response");
    boom_core_step(s,p);
    CHECK(p.dmem_resp.empty()&&s.completion.prf_writes_this_cycle==2&&
          s.completion.completion_accepts_this_cycle==2&&!s.completion.int_execute.valid&&
          !s.completion.load_response.valid&&!s.lsu.load_response_pending&&
          boom::prf_read(s,14)==0x1414&&boom::prf_read(s,15)==0x1515&&
          boom::prf_read(s,16)==0x1616&&boom::prf_read(s,17)==0x1717,
          "retained response was not consumed on the next step");
    PASS();
}
int main(){std::printf("=== Gate 4.0 W4D Multi Completion Tests ===\n");t01();t02();t03();t04();t05();t06();t07();std::printf("W4D multi completion: %d passed, %d failed\n",passed,failed);return failed?1:0;}
