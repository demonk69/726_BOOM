#include "boom_config.hpp"
#include "boom_types.hpp"
#include "boom_state.hpp"
#include "boom_interfaces.hpp"
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <vector>

extern void boom_core_step(BoomCoreState& state, PipeSignals& pipe);
namespace boom { void issue_module(BoomCoreState& state); }

static int tests_passed=0, tests_failed=0;
#define TEST(n) printf("  [GATE1] %-58s ... ", n)
#define PASS() do { printf("PASS\n"); tests_passed++; } while(0)
#define FAIL(m) do { printf("FAIL: %s\n", m); tests_failed++; } while(0)
#define CHECK(c,m) do { if(!(c)) { FAIL(m); return; } } while(0)

static uint32_t MI(uint32_t i,uint8_t rs1,uint8_t f3,uint8_t rd,uint8_t op) {return ((i&0xFFF)<<20)|(rs1<<15)|(f3<<12)|(rd<<7)|op;}
static uint32_t MR(uint8_t f7,uint8_t rs2,uint8_t rs1,uint8_t f3,uint8_t rd,uint8_t op) {return (f7<<25)|(rs2<<20)|(rs1<<15)|(f3<<12)|(rd<<7)|op;}
static uint32_t MB(int32_t imm,uint8_t rs2,uint8_t rs1,uint8_t f3) {uint32_t b=((imm>>12)&1)<<31; b|=((imm>>5)&0x3F)<<25; b|=((imm>>1)&0xF)<<8; b|=((imm>>11)&1)<<7; return b|(rs2<<20)|(rs1<<15)|(f3<<12)|0x63;}
static uint32_t EC() {return 0x00000073;}

struct IM {
    uint32_t w[512]; int n; uint64_t base;
    void clear(uint64_t b) { base=b; n=0; memset(w,0,sizeof(w)); }
    void add(uint32_t x) { if(n<512) w[n++]=x; }
    uint32_t read(uint64_t a) const { if(a<base) return 0; uint32_t i=(a-base)>>2; return (i<(uint32_t)n)?w[i]:0; }
};

struct TR { uint64_t pc,rd_v; uint32_t inst; uint8_t rd; bool rd_ok,exc;
    TR():pc(0),rd_v(0),inst(0),rd(0),rd_ok(false),exc(false){} };

static BoomCoreState run_program(const IM& mem, int mc, std::vector<TR>& trace) {
    BoomCoreState s; PipeSignals p; trace.clear();
    for (int c=0; c<mc; c++) {
        if (!p.imem_req.empty()) {
            ImemRequest r=p.imem_req.read(); ImemResponse rs;
            rs.address=r.address; rs.fetch_id=r.fetch_id; rs.epoch=r.epoch; rs.instruction=mem.read(r.address); rs.exception=false;
            if(!p.imem_resp.full()) p.imem_resp.write(rs);
        }
        boom_core_step(s,p);
        while(!p.commit_trace.empty()) { CommitEntry ce=p.commit_trace.read(); TR t; t.pc=ce.pc; t.inst=ce.inst; t.rd=ce.rd; t.rd_v=ce.rd_value; t.rd_ok=ce.rd_valid; t.exc=ce.exception; trace.push_back(t); }
        if (s.io_success || s.io_trap) break;
    }
    return s;
}

static bool saw_rd_value(const std::vector<TR>& tr, uint8_t rd, uint64_t value) {
    for (size_t i=0; i<tr.size(); i++) if (tr[i].rd==rd && tr[i].rd_v==value) return true;
    return false;
}

static uint64_t last_rd_value(const std::vector<TR>& tr, uint8_t rd) {
    uint64_t v=0;
    for (size_t i=0; i<tr.size(); i++) if (tr[i].rd==rd) v=tr[i].rd_v;
    return v;
}

static MicroOp make_iq_uop(uint64_t pc, uint8_t rob_idx) {
    MicroOp u;
    u.uopc = 1;
    u.iq_type = IQ_ALU;
    u.fu_code = FU_ALU;
    u.debug_pc = pc;
    u.queue.rob_idx = rob_idx;
    u.rename.dst_rtype = DST_INT;
    return u;
}

void t_war_chain() { TEST("WAR chain preserves old source mapping");
    IM m; m.clear(RESET_VECTOR);
    m.add(MI(3,0,0,1,0x13)); m.add(MR(0,0,1,0,2,0x33)); m.add(MR(0,0,1,0,3,0x33)); m.add(MI(7,0,0,1,0x13)); m.add(EC());
    std::vector<TR> tr; run_program(m,200,tr);
    CHECK(saw_rd_value(tr,2,3),"x2 did not read old x1=3");
    CHECK(saw_rd_value(tr,3,3),"x3 did not read old x1=3");
    CHECK(last_rd_value(tr,1)==7,"final x1 WAW value not 7"); PASS(); }

void t_multi_waw() { TEST("multiple WAW commits newest architectural value");
    IM m; m.clear(RESET_VECTOR);
    m.add(MI(1,0,0,1,0x13)); m.add(MI(2,0,0,1,0x13)); m.add(MI(3,0,0,1,0x13)); m.add(MR(0,0,1,0,2,0x33)); m.add(EC());
    std::vector<TR> tr; run_program(m,200,tr);
    CHECK(last_rd_value(tr,1)==3,"final x1 not newest WAW value");
    CHECK(last_rd_value(tr,2)==3,"consumer did not see newest x1"); PASS(); }

void t_raw_chain() { TEST("RAW chain forwards through renamed physical regs");
    IM m; m.clear(RESET_VECTOR);
    m.add(MI(5,0,0,1,0x13)); m.add(MI(2,1,0,2,0x13)); m.add(MI(4,2,0,3,0x13)); m.add(EC());
    std::vector<TR> tr; run_program(m,200,tr);
    CHECK(last_rd_value(tr,3)==11,"RAW chain result x3!=11"); PASS(); }

void t_stale_pdst_recycle() { TEST("stale physical registers recycle under WAW pressure");
    IM m; m.clear(RESET_VECTOR);
    for (int i=1; i<=80; i++) m.add(MI((uint32_t)i,0,0,1,0x13));
    m.add(EC());
    std::vector<TR> tr; BoomCoreState s=run_program(m,500,tr);
    CHECK(last_rd_value(tr,1)==80,"final WAW value after recycle wrong");
    CHECK(s.rename.int_free_list.count>=49,"free list did not recover stale pdsts"); PASS(); }

void t_branch_after_allocs() { TEST("taken branch after WAW allocations skips wrong path");
    IM m; m.clear(RESET_VECTOR);
    m.add(MI(1,0,0,1,0x13)); m.add(MI(2,0,0,1,0x13)); m.add(MI(2,0,0,2,0x13));
    m.add(MB(8,2,1,0)); m.add(MI(99,0,0,1,0x13)); m.add(MI(5,0,0,3,0x13)); m.add(EC());
    std::vector<TR> tr; run_program(m,250,tr);
    CHECK(!saw_rd_value(tr,1,99),"wrong-path x1=99 committed");
    CHECK(last_rd_value(tr,1)==2,"x1 did not recover committed mapping");
    CHECK(last_rd_value(tr,3)==5,"branch target did not commit"); PASS(); }

void t_nested_taken_branches() { TEST("back-to-back taken branches clear wrong paths");
    IM m; m.clear(RESET_VECTOR);
    m.add(MI(1,0,0,1,0x13)); m.add(MI(1,0,0,2,0x13));
    m.add(MB(8,2,1,0)); m.add(MI(11,0,0,3,0x13));
    m.add(MB(8,2,1,0)); m.add(MI(22,0,0,3,0x13));
    m.add(MI(33,0,0,3,0x13)); m.add(EC());
    std::vector<TR> tr; run_program(m,300,tr);
    CHECK(!saw_rd_value(tr,3,11),"first wrong-path write committed");
    CHECK(!saw_rd_value(tr,3,22),"second wrong-path write committed");
    CHECK(last_rd_value(tr,3)==33,"nested branch final target missing"); PASS(); }

void t_branch_commit_trace() { TEST("branch commit and redirect preserve event order");
    IM m; m.clear(RESET_VECTOR);
    uint32_t br=MB(8,2,1,0);
    m.add(MI(5,0,0,1,0x13)); m.add(MI(5,0,0,2,0x13)); m.add(br); m.add(MI(99,0,0,3,0x13)); m.add(MI(10,0,0,3,0x13)); m.add(EC());
    std::vector<TR> tr; run_program(m,250,tr);
    bool branch_committed=false;
    for (size_t i=0; i<tr.size(); i++) if (tr[i].inst==br && tr[i].pc==RESET_VECTOR+8) branch_committed=true;
    CHECK(branch_committed,"taken branch did not appear in commit trace");
    CHECK(!saw_rd_value(tr,3,99),"wrong-path commit after branch"); PASS(); }

void t_iq_port_conflict() { TEST("IQ grants only implemented ALU execute port");
    BoomCoreState s;
    for (int i=0; i<3; i++) { IssueSlotEntry& e=s.issue.alu_iq.entries[i]; e.valid=true; e.request=true; e.uop=make_iq_uop(RESET_VECTOR+4*i,i); }
    s.issue.alu_iq.count=3; s.issue.alu_iq.tail=3;
    boom::issue_module(s);
    CHECK(s.issue.issued_valids[INT_ISSUE_LANE],"no IQ grant");
    CHECK(!s.issue.issued_valids[MEM_ISSUE_LANE] && !s.issue.issued_valids[FP_ISSUE_LANE],"more than one uop granted to one execute lane");
    CHECK(s.issue.alu_iq.count==2,"unissued IQ entries were dropped");
    CHECK(s.issue.alu_iq.entries[0].uop.debug_pc==RESET_VECTOR+4,"oldest remaining entry wrong"); PASS(); }

void t_iq_oldest_ready() { TEST("IQ selects oldest ready entry and keeps older blocked");
    BoomCoreState s;
    for (int i=0; i<3; i++) { IssueSlotEntry& e=s.issue.alu_iq.entries[i]; e.valid=true; e.request=true; e.uop=make_iq_uop(RESET_VECTOR+4*i,i); }
    s.issue.alu_iq.entries[0].uop.rename.prs1=7;
    s.rename.int_free_list.busy_table[7]=true;
    s.issue.alu_iq.count=3; s.issue.alu_iq.tail=3;
    boom::issue_module(s);
    CHECK(s.issue.issued_valids[INT_ISSUE_LANE],"no ready grant");
    CHECK(s.issue.issued_uops[INT_ISSUE_LANE].debug_pc==RESET_VECTOR+4,"did not select oldest ready entry");
    CHECK(s.issue.alu_iq.count==2,"wrong IQ count after grant");
    CHECK(s.issue.alu_iq.entries[0].uop.debug_pc==RESET_VECTOR,"older blocked entry not retained"); PASS(); }

void t_iq_dispatch_issue_same_cycle() { TEST("IQ dispatch can issue in same core cycle");
    BoomCoreState s;
    s.rename.dispatch_packets[0].valid=true;
    s.rename.dispatch_packets[0].rob_allocated=true;
    s.rename.dispatch_packets[0].uop=make_iq_uop(RESET_VECTOR,0);
    boom::issue_module(s);
    CHECK(s.issue.issued_valids[INT_ISSUE_LANE],"dispatched uop did not issue from empty IQ");
    CHECK(s.issue.alu_iq.count==0,"same-cycle issued uop left in IQ"); PASS(); }

void t_iq_flush_clear() { TEST("flush clears all IQ entries");
    BoomCoreState s;
    for (int i=0; i<4; i++) { s.issue.alu_iq.entries[i].valid=true; s.issue.alu_iq.entries[i].request=true; }
    s.issue.alu_iq.count=4; s.issue.alu_iq.tail=4; s.global_flush=true;
    boom::issue_module(s);
    CHECK(s.issue.alu_iq.count==0,"IQ count not cleared");
    for (int i=0; i<ISSUE_QUEUE_ALU_DEPTH; i++) CHECK(!s.issue.alu_iq.entries[i].valid,"IQ valid not cleared");
    PASS(); }

void t_stale_imem_response() { TEST("frontend drops stale imem response fetch_id");
    BoomCoreState s; PipeSignals p;
    boom_core_step(s,p);
    CHECK(!p.imem_req.empty(),"no initial imem request");
    ImemRequest r0=p.imem_req.read();
    ImemResponse stale; stale.address=r0.address; stale.fetch_id=r0.fetch_id+99; stale.instruction=EC(); p.imem_resp.write(stale);
    boom_core_step(s,p);
    CHECK(!s.io_success && !s.io_trap,"stale response was executed");
    CHECK(s.frontend.request_sent,"request was cleared by stale response");
    ImemResponse good0; good0.address=r0.address; good0.fetch_id=r0.fetch_id; good0.epoch=r0.epoch; good0.instruction=MI(42,0,0,1,0x13); p.imem_resp.write(good0);
    boom_core_step(s,p);
    boom_core_step(s,p);
    CHECK(!p.imem_req.empty(),"no second imem request");
    ImemRequest r1=p.imem_req.read();
    ImemResponse good1; good1.address=r1.address; good1.fetch_id=r1.fetch_id; good1.epoch=r1.epoch; good1.instruction=EC(); p.imem_resp.write(good1);
    for (int i=0; i<4 && !s.io_success; i++) boom_core_step(s,p);
    CHECK(s.io_success,"correct response stream did not reach ECALL"); PASS(); }

void t_single_lane_config() { TEST("SmallBoom HLS build is single-lane rename/dispatch");
    CHECK(DISPATCH_WIDTH==1,"this regression expects SmallBoom single dispatch lane");
    CHECK(DECODE_WIDTH==1,"this regression expects SmallBoom single decode lane"); PASS(); }

int main() {
    printf("=== BOOM-HLS Gate 1 Regression Tests ===\n\n");
    t_war_chain(); t_multi_waw(); t_raw_chain(); t_stale_pdst_recycle();
    t_branch_after_allocs(); t_nested_taken_branches(); t_branch_commit_trace();
    t_iq_port_conflict(); t_iq_oldest_ready(); t_iq_dispatch_issue_same_cycle(); t_iq_flush_clear();
    t_stale_imem_response(); t_single_lane_config();
    printf("\n=== %d passed, %d failed ===\n", tests_passed, tests_failed);
    return (tests_failed>0)?1:0;
}
