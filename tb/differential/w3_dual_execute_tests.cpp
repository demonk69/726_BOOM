#include "boom_config.hpp"
#include "boom_state.hpp"
#include "boom_interfaces.hpp"
#include <cstdio>

namespace boom { void issue_module(BoomCoreState&); void execute_module(BoomCoreState&); }
extern void boom_core_step(BoomCoreState&, PipeSignals&);

static int passed=0, failed=0;
#define TEST(n) std::printf("  [W3 dual] %-52s ... ", n)
#define CHECK(c,m) do { if (!(c)) { std::printf("FAIL: %s\n",m); failed++; return; } } while (0)
#define PASS() do { std::printf("PASS\n"); passed++; } while (0)

static MicroOp uop(IssuePortClass port, uint8_t rob, uint8_t pdst=0) {
    MicroOp u;
    u.queue.rob_idx=rob; u.rename.pdst=pdst; u.rename.dst_rtype=pdst ? DST_INT : DST_N;
    if (port==ISSUE_PORT_MEM) { u.uopc=39; u.iq_type=IQ_MEM; u.fu_code=FU_MEM; u.ctrl.is_load=true; u.mem.uses_ldq=true; }
    else { u.uopc=1; u.iq_type=IQ_ALU; u.fu_code=FU_ALU; }
    return u;
}
static void seed(BoomCoreState& s, int idx, IssuePortClass port, uint8_t rob, uint8_t pdst=0) {
    IssueSlotEntry& e=s.issue.alu_iq.entries[idx]; e=IssueSlotEntry(); e.valid=true; e.request=true; e.uop=uop(port,rob,pdst);
    s.issue.alu_iq.count++; s.issue.alu_iq.tail=s.issue.alu_iq.count%ISSUE_QUEUE_ALU_DEPTH;
}
static void dual(BoomCoreState& s) { seed(s,0,ISSUE_PORT_MEM,1); seed(s,1,ISSUE_PORT_INT,2,3); }

static void t01() { TEST("fixed MEM and INT lanes both accept"); BoomCoreState s; dual(s); boom::issue_module(s);
    CHECK(s.issue.grants[0].accepted && s.issue.grants[1].accepted,"dual grants not accepted"); PASS(); }
static void t02() { TEST("accepted dual entries both leave IQ"); BoomCoreState s; dual(s); boom::issue_module(s);
    CHECK(s.issue.alu_iq.count==0 && s.issue.grants_accepted==2,"accepted entries retained"); PASS(); }
static void t03() { TEST("issued uops retain fixed lane identity"); BoomCoreState s; dual(s); boom::issue_module(s);
    CHECK(s.issue.issued_valids[0] && s.issue.issued_uops[0].iq_type==IQ_MEM && s.issue.issued_valids[1] && s.issue.issued_uops[1].iq_type==IQ_ALU,"lane identity wrong"); PASS(); }
static void t04() { TEST("lane 2 remains invalid"); BoomCoreState s; dual(s); boom::issue_module(s);
    CHECK(!s.issue.grants[2].valid && !s.issue.issued_valids[2],"FP lane used"); PASS(); }
static void t05() { TEST("blocked MEM preserves entry while INT accepts"); BoomCoreState s; dual(s); s.issue.port_ready[0]=false; boom::issue_module(s);
    CHECK(!s.issue.grants[0].accepted && s.issue.grants[1].accepted && s.issue.alu_iq.count==1 && s.issue.alu_iq.entries[0].uop.queue.rob_idx==1,"lane independence lost"); PASS(); }
static void t06() { TEST("blocked INT preserves entry while MEM accepts"); BoomCoreState s; dual(s); s.issue.port_ready[1]=false; boom::issue_module(s);
    CHECK(s.issue.grants[0].accepted && !s.issue.grants[1].accepted && s.issue.alu_iq.count==1 && s.issue.alu_iq.entries[0].uop.queue.rob_idx==2,"lane independence lost"); PASS(); }
static void t07() { TEST("full LDQ blocks only load lane"); BoomCoreState s; dual(s); s.lsu.ldq_count=LDQ_DEPTH; boom::issue_module(s);
    CHECK(!s.issue.grants[0].accepted && s.issue.grants[1].accepted,"LDQ pressure crossed lanes"); PASS(); }
static void t08() { TEST("full STQ does not block load lane"); BoomCoreState s; dual(s); s.lsu.stq_count=STQ_DEPTH; boom::issue_module(s);
    CHECK(s.issue.grants[0].accepted && s.issue.grants[1].accepted,"unrelated STQ blocked load"); PASS(); }
static void t09() { TEST("execute fills fixed persistent slots"); BoomCoreState s; dual(s); s.rob.entries[1].valid=s.rob.entries[2].valid=true; boom::issue_module(s); boom::execute_module(s);
    CHECK(s.execute.alu_results[0].valid && s.execute.alu_results[0].is_load && s.execute.alu_results[1].valid && !s.execute.alu_results[1].memory_valid,"execute slot mapping wrong"); PASS(); }
static void t10() { TEST("execute does not write integer RF immediately"); BoomCoreState s; seed(s,0,ISSUE_PORT_INT,2,3); boom::prf_seed(s,3,99); boom::issue_module(s); boom::execute_module(s);
    CHECK(boom::prf_read(s,3)==99 && s.rename.int_free_list.busy_table[3]==false,"execute wrote RF"); PASS(); }
static void t11() { TEST("execute never overwrites valid INT slot"); BoomCoreState s; s.execute.alu_results[1].valid=true; s.execute.alu_results[1].result=0x55; s.issue.issued_valids[1]=true; s.issue.issued_uops[1]=uop(ISSUE_PORT_INT,2,3); boom::execute_module(s);
    CHECK(s.execute.alu_results[1].result==0x55,"held completion overwritten"); PASS(); }
static void t12() { TEST("execute never overwrites valid MEM slot"); BoomCoreState s; s.execute.alu_results[0].valid=true; s.execute.alu_results[0].result=0x66; s.issue.issued_valids[0]=true; s.issue.issued_uops[0]=uop(ISSUE_PORT_MEM,1); boom::execute_module(s);
    CHECK(s.execute.alu_results[0].result==0x66,"held MEM completion overwritten"); PASS(); }
static void t13() { TEST("dispatch readiness consults current busy table"); BoomCoreState s; s.rename.dispatch_packets[0].valid=true; s.rename.dispatch_packets[0].rob_allocated=true; s.rename.dispatch_packets[0].uop=uop(ISSUE_PORT_INT,4); s.rename.dispatch_packets[0].uop.rename.prs1=7; s.rename.dispatch_packets[0].uop.rename.prs1_busy=false; s.rename.int_free_list.busy_table[7]=true; boom::issue_module(s);
    CHECK(!s.issue.grants[1].valid && s.issue.alu_iq.count==1 && s.issue.alu_iq.entries[0].prs1_busy,"stale dispatch readiness used"); PASS(); }
static void t14() { TEST("empty completion slot enables corresponding lane"); BoomCoreState s; PipeSignals p; s.rob.state=ROB_NORMAL; s.frontend.reset_done=true; s.execute.alu_results[1].valid=true; s.execute.alu_results[1].uop.queue.rob_idx=3; s.rob.entries[3].valid=false; boom_core_step(s,p);
    CHECK(s.issue.port_ready[1],"stale completion did not release lane"); PASS(); }
static void t15() { TEST("valid held slot backpressures only its lane"); BoomCoreState s; s.execute.alu_results[0].valid=true; s.execute.alu_results[0].uop.queue.rob_idx=1; s.rob.entries[1].valid=true; s.lsu.ldq_count=LDQ_DEPTH; seed(s,0,ISSUE_PORT_MEM,2); seed(s,1,ISSUE_PORT_INT,3); PipeSignals p; boom_core_step(s,p);
    CHECK(!s.issue.grants[0].accepted && s.issue.grants[1].accepted,"slot backpressure crossed lanes"); PASS(); }

int main() { std::printf("=== W3 Core Dual Execute Tests ===\n");
    t01();t02();t03();t04();t05();t06();t07();t08();t09();t10();t11();t12();t13();t14();t15();
    std::printf("W3 dual execute: %d passed, %d failed\n",passed,failed); return failed?1:0; }
