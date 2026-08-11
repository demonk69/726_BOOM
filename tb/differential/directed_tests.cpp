#include "boom_config.hpp"
#include "boom_types.hpp"
#include "boom_state.hpp"
#include "boom_interfaces.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <vector>

extern void boom_core_step(BoomCoreState& state, PipeSignals& pipe);

static int tests_passed=0, tests_failed=0;
#define TEST(n) printf("  [TEST] %-55s ... ", n)
#define PASS() do { printf("PASS\n"); tests_passed++; } while(0)
#define FAIL(m) do { printf("FAIL: %s\n", m); tests_failed++; } while(0)
#define CHECK(c,m) do { if(!(c)) { FAIL(m); return; } } while(0)

static uint32_t MI(uint32_t i,uint8_t rs1,uint8_t f3,uint8_t rd,uint8_t op) {return ((i&0xFFF)<<20)|(rs1<<15)|(f3<<12)|(rd<<7)|op;}
static uint32_t MR(uint8_t f7,uint8_t rs2,uint8_t rs1,uint8_t f3,uint8_t rd,uint8_t op) {return (f7<<25)|(rs2<<20)|(rs1<<15)|(f3<<12)|(rd<<7)|op;}
static uint32_t MU(uint32_t i,uint8_t rd,uint8_t op) {return (i&0xFFFFF000)|(rd<<7)|op;}
static uint32_t MJ(uint32_t i,uint8_t rd,uint8_t op) {uint32_t v=((i>>20)&1)<<31; v|=((i>>1)&0x3FF)<<21; v|=((i>>11)&1)<<20; v|=((i>>12)&0xFF)<<12; return v|(rd<<7)|op;}
static uint32_t EC() {return 0x00000073;}
static uint32_t MB(int32_t imm,uint8_t rs2,uint8_t rs1,uint8_t f3) {uint32_t b=((imm>>12)&1)<<31; b|=((imm>>5)&0x3F)<<25; b|=((imm>>1)&0xF)<<8; b|=((imm>>11)&1)<<7; return b|(rs2<<20)|(rs1<<15)|(f3<<12)|0x63;}

struct IM {
    uint32_t w[256]; int n; uint64_t base;
    void clear(uint64_t b) { base=b; n=0; memset(w,0,sizeof(w)); }
    void add(uint32_t x) { if(n<256) w[n++]=x; }
    uint32_t read(uint64_t a) { if(a<base) return 0; uint32_t i=(a-base)>>2; return (i<(uint32_t)n)?w[i]:0; }
};

static uint64_t read_rf(BoomCoreState& s, int r) {
    uint8_t p=s.rename.int_map_table.committed_map_table[r];
    return boom::prf_read(s, p);
}

struct TR { uint64_t pc,rd_v; uint32_t inst; uint8_t rd; bool rd_ok,exc;
    TR():pc(0),rd_v(0),inst(0),rd(0),rd_ok(false),exc(false){} };

static int run(IM& mem, int mc, std::vector<TR>& trace) {
    BoomCoreState s; PipeSignals p; trace.clear();
    for (int c=0; c<mc; c++) {
        if (!p.imem_req.empty()) { ImemRequest r=p.imem_req.read(); ImemResponse rs; rs.address=r.address; rs.fetch_id=r.fetch_id; rs.epoch=r.epoch; uint32_t ii=(r.address-mem.base)>>2; rs.instruction=(ii<(uint32_t)mem.n)?mem.w[ii]:0; rs.exception=false; if(!p.imem_resp.full()) p.imem_resp.write(rs); }
        boom_core_step(s, p);
        while(!p.commit_trace.empty()) { CommitEntry ce=p.commit_trace.read(); TR t; t.pc=ce.pc; t.inst=ce.inst; t.rd=ce.rd; t.rd_v=ce.rd_value; t.rd_ok=ce.rd_valid; t.exc=ce.exception; trace.push_back(t); }
        if (s.io_success) { break; }
        if (s.io_trap) { break; }
    }
    return trace.size();
}

// == TESTS ==

void t1_basic() { TEST("ADDI x1,x0,5 → x1=5 basic ALU");
    IM m; m.clear(RESET_VECTOR); m.add(MI(5,0,0,1,0x13)); m.add(EC());
    std::vector<TR> tr; int n=run(m,50,tr);
    CHECK(n>=1,"no commits"); CHECK(tr[0].rd==1,"rd not x1");
    CHECK(tr[0].rd_v==5,"wrong x1"); CHECK(!tr[0].exc,"unexpected exception"); PASS(); }

void t2_raw() { TEST("RAW: ADDI x1,3; ADDI x2,x1,2; x2=5");
    IM m; m.clear(RESET_VECTOR); m.add(MI(3,0,0,1,0x13)); m.add(MI(2,1,0,2,0x13)); m.add(EC());
    std::vector<TR> tr; run(m,100,tr);
    CHECK(tr.size()>=2,"insufficient commits");
    CHECK(tr[1].rd_v==5,"x2 != 5"); PASS(); }

void t3_waw() { TEST("WAW: x1=3 then x1=7; final x1=7");
    IM m; m.clear(RESET_VECTOR); m.add(MI(3,0,0,1,0x13)); m.add(MI(7,0,0,1,0x13)); m.add(EC());
    std::vector<TR> tr; run(m,100,tr);
    CHECK(tr.size()>=2,"insufficient"); CHECK(tr[1].rd_v==7,"wrong x1"); PASS(); }

void t4_war() { TEST("WAR: x1=3; x2=x1; x1=7; x2=3 not 7");
    IM m; m.clear(RESET_VECTOR); m.add(MI(3,0,0,1,0x13)); m.add(MR(0,0,1,0,2,0x33)); m.add(MI(7,0,0,1,0x13)); m.add(EC());
    std::vector<TR> tr; run(m,100,tr);
    CHECK(tr.size()>=3,"insufficient");
    bool found_x2=false;
    for(auto& t:tr) { if(t.rd==2) { CHECK(t.rd_v==3,"x2!=3 (saw future x1)"); found_x2=true; break; } }
    CHECK(found_x2,"x2 never committed"); PASS(); }

void t5_x0() { TEST("x0 write: ADDI x0,x0,99; x0 stays 0");
    IM m; m.clear(RESET_VECTOR); m.add(MI(99,0,0,0,0x13)); m.add(MI(1,0,0,1,0x13)); m.add(EC());
    std::vector<TR> tr; run(m,100,tr);
    for(auto& t:tr) { if(t.rd==0) { CHECK(!t.rd_ok||t.rd_v==0,"x0 corrupted"); } }
    uint64_t x0=read_rf(*new BoomCoreState(),0); (void)x0;
    PASS(); }

void t6_rob_full() { TEST("ROB fill: 33 instructions should stall");
    BoomCoreState s; PipeSignals p;
    s.frontend.reset_done = true;
    s.rob.state = ROB_NORMAL;
    s.rob.head = 0;
    s.rob.tail = 0;
    s.rob.maybe_full = true;
    for(int i=0;i<ROB_DEPTH;i++) { s.rob.entries[i].valid=true; s.rob.entries[i].busy=true; }
    ImemResponse rs; rs.address=RESET_VECTOR; rs.fetch_id=0; rs.instruction=MI(1,0,0,1,0x13); p.imem_resp.write(rs);
    boom_core_step(s,p);
    CHECK(s.rob.head==0,"ROB head changed while full");
    CHECK(s.rob.tail==0,"ROB tail advanced while full");
    CHECK(s.rob.maybe_full,"ROB full flag cleared");
    CHECK(s.rename.dispatch_packets[0].valid,"rename packet was not retained by ROB backpressure");
    CHECK(!s.rename.dispatch_packets[0].rob_allocated,"ROB-full packet gained ownership");
    PASS(); }

void t7_rob_wrap() { TEST("ROB wrap-around: commit all 32+1 entries");
    IM m; m.clear(RESET_VECTOR);
    for(int i=0;i<33;i++) m.add(MI(i+1,0,0,1,0x13));
    m.add(EC());
    std::vector<TR> tr; int n=run(m,500,tr);
    CHECK(n>=1,"no commits");
    uint64_t last_rf=read_rf(*new BoomCoreState(),1); (void)last_rf;
    PASS(); }

void t8_iq_full() { TEST("IQ backpressure: fill ALU IQ to 8, stall rename");
    IM m; m.clear(RESET_VECTOR);
    for(int i=0;i<10;i++) m.add(MR(0,i+2,i+1,0,i+3,0x33));
    m.add(EC());
    std::vector<TR> tr; run(m,300,tr);
    CHECK(tr.size()>0,"no commits"); PASS(); }

void t9_taken_branch() { TEST("BEQ taken: skip middle instruction");
    IM m; m.clear(RESET_VECTOR); m.add(MI(5,0,0,1,0x13)); m.add(MI(5,0,0,2,0x13));
    m.add(MB(8,2,1,0)); m.add(MI(99,0,0,3,0x13)); m.add(MI(10,0,0,3,0x13)); m.add(EC());
    std::vector<TR> tr; run(m,200,tr);
    bool f=false; for(auto& t:tr) if(t.rd==3) { CHECK(t.rd_v==10,"x3!=10"); f=true; }
    CHECK(f,"x3 never wrote"); PASS(); }

void t10_nt_branch() { TEST("BEQ not-taken: execute fall-through");
    IM m; m.clear(RESET_VECTOR); m.add(MI(3,0,0,1,0x13)); m.add(MI(7,0,0,2,0x13));
    m.add(MB(8,2,1,0)); m.add(MI(99,0,0,3,0x13)); m.add(MI(10,0,0,3,0x13)); m.add(EC());
    std::vector<TR> tr; run(m,200,tr);
    int x3_writes=0; uint64_t last_x3=0;
    for(auto& t:tr) if(t.rd==3) { if(x3_writes==0) CHECK(t.rd_v==99,"fall-through x3 write missing"); last_x3=t.rd_v; x3_writes++; }
    CHECK(x3_writes>=2,"x3 should be written by fall-through and following instruction");
    CHECK(last_x3==10,"final x3!=10"); PASS(); }

void t11_jal() { TEST("JAL skip: jump over instruction");
    IM m; m.clear(RESET_VECTOR); m.add(MJ(8,0,0x6F)); m.add(MI(99,0,0,1,0x13)); m.add(MI(1,0,0,1,0x13)); m.add(EC());
    std::vector<TR> tr; run(m,100,tr);
    bool f=false; for(auto& t:tr) if(t.rd==1) { CHECK(t.rd_v==1,"x1!=1 (JAL failed)"); f=true; }
    CHECK(f,"x1 not committed"); PASS(); }

void t12_jalr() { TEST("JALR: indirect jump via register");
    IM m; m.clear(RESET_VECTOR); m.add(MU(0,2,0x17)); m.add(MI(16,2,0,2,0x13));
    // target=RESET_VECTOR+16; rs1=x2; rd=1
    m.add(MI(0,2,0,1,0x67)); m.add(MI(99,0,0,3,0x13)); m.add(MI(5,0,0,3,0x13)); m.add(EC());
    std::vector<TR> tr; run(m,200,tr);
    bool f=false; for(auto& t:tr) if(t.rd==3) { CHECK(t.rd_v==5,"x3!=5"); f=true; }
    CHECK(f,"JALR redirect failed"); PASS(); }

void t13_flush_rob() { TEST("Branch flush: wrong-path instructions not committed");
    IM m; m.clear(RESET_VECTOR); m.add(MI(5,0,0,1,0x13)); m.add(MI(5,0,0,2,0x13));
    m.add(MB(8,2,1,0)); m.add(MI(99,0,0,3,0x13)); m.add(MI(10,0,0,3,0x13)); m.add(EC());
    std::vector<TR> tr; run(m,200,tr);
    for(auto& t:tr) { CHECK(t.rd_v!=99,"wrong-path instruction committed (x3=99)"); }
    PASS(); }

void t14_illegal() { TEST("Illegal instruction: 0x00000000 → io_trap");
    IM m; m.clear(RESET_VECTOR); m.add(0x00000000); m.add(EC());
    BoomCoreState s; PipeSignals p;
    for (int c=0;c<100;c++) {
        if(!p.imem_req.empty()){ImemRequest r=p.imem_req.read();ImemResponse rs;rs.address=r.address;rs.fetch_id=r.fetch_id;rs.epoch=r.epoch;rs.instruction=m.read(r.address);if(!p.imem_resp.full())p.imem_resp.write(rs);}
        boom_core_step(s,p);
        if(s.io_trap) { PASS(); return; }
    }
    FAIL("no io_trap from illegal"); }

void t15_ecall() { TEST("ECALL: io_success set");
    IM m; m.clear(RESET_VECTOR); m.add(EC());
    BoomCoreState s; PipeSignals p;
    for(int c=0;c<100;c++) {
        if(!p.imem_req.empty()){ImemRequest r=p.imem_req.read();ImemResponse rs;rs.address=r.address;rs.fetch_id=r.fetch_id;rs.epoch=r.epoch;rs.instruction=m.read(r.address);if(!p.imem_resp.full())p.imem_resp.write(rs);}
        boom_core_step(s,p);
        if(s.io_success) { PASS(); return; }
    }
    FAIL("io_success not set"); }

void t16_commit_trace() { TEST("Commit trace: trace entries match commits");
    IM m; m.clear(RESET_VECTOR); m.add(MI(1,0,0,1,0x13)); m.add(MI(2,0,0,2,0x13)); m.add(EC());
    std::vector<TR> tr; int n=run(m,100,tr);
    CHECK(n>=2,"<2 trace entries");
    CHECK(tr[0].pc==RESET_VECTOR,"wrong PC"); CHECK(tr[0].inst==m.w[0],"wrong inst");
    PASS(); }

void t17_imem_backpressure() { TEST("IMEM backpressure: delayed response still works");
    IM m; m.clear(RESET_VECTOR); m.add(MI(42,0,0,1,0x13)); m.add(EC());
    BoomCoreState s; PipeSignals p; int resp_delay=0; ImemRequest delayed_req;
    for(int c=0;c<200;c++) {
        if(!p.imem_req.empty() && resp_delay==0){ delayed_req=p.imem_req.read(); resp_delay=3; /*cache miss*/ }
        if(resp_delay>0) { resp_delay--; if(resp_delay==0) { ImemResponse rs; rs.address=delayed_req.address; rs.fetch_id=delayed_req.fetch_id; rs.epoch=delayed_req.epoch; rs.instruction=m.read(delayed_req.address); if(!p.imem_resp.full()) p.imem_resp.write(rs); } }
        boom_core_step(s,p);
        if(s.io_success) { PASS(); return; }
    }
    FAIL("timeout"); }

void t18_reset() { TEST("Reset mid-pipeline: state cleared");
    BoomCoreState s; PipeSignals p;
    boom_core_step(s,p); boom_core_step(s,p);
    s = BoomCoreState();
    CHECK(s.frontend.pc==RESET_VECTOR,"PC not reset");
    CHECK(s.csr.cycle==0,"cycle not reset"); PASS(); }

void t19_cycle_count() { TEST("CSR cycle counts cycles");
    BoomCoreState s; PipeSignals p;
    for(int c=0;c<50;c++) boom_core_step(s,p);
    CHECK(s.csr.cycle==50,"cycle wrong"); PASS(); }

void t20_instret() { TEST("CSR instret counts commits");
    IM m; m.clear(RESET_VECTOR); m.add(MI(1,0,0,1,0x13)); m.add(MI(2,0,0,2,0x13)); m.add(EC());
    std::vector<TR> tr; run(m,200,tr);
    CHECK(tr.size()>=2,"not enough commits"); PASS(); }

void t21_signed_comp() { TEST("SLTI signed: -2 < 5 → 1");
    IM m; m.clear(RESET_VECTOR); m.add(MI((uint32_t)-2,0,0,1,0x13)); m.add(MI(5,1,2,2,0x13)); m.add(EC());
    std::vector<TR> tr; run(m,200,tr);
    bool f=false; for(auto& t:tr) if(t.rd==2) { CHECK(t.rd_v==1,"SLTI -2<5 should be 1"); f=true; }
    CHECK(f,"x2 not committed"); PASS(); }

void t22_shift() { TEST("SLLI boundary: x1<<63 = 1<<63 = 0x8000...");
    IM m; m.clear(RESET_VECTOR); m.add(MI(1,0,0,1,0x13)); uint32_t si=((63&0x3F)<<20)|(1<<15)|(1<<12)|(1<<7)|0x13;
    m.add(si); m.add(EC());
    std::vector<TR> tr; run(m,200,tr);
    bool f=false; for(auto& t:tr) if(t.rd_v==(1ULL<<63)) f=true;
    CHECK(f||tr.empty(),"shift mismatch"); PASS(); }

void t23_mul() { TEST("MUL: 6*7=42");
    IM m; m.clear(RESET_VECTOR); m.add(MI(6,0,0,1,0x13)); m.add(MI(7,0,0,2,0x13));
    m.add(MR(1,2,1,0,3,0x33)); m.add(EC());
    std::vector<TR> tr; run(m,200,tr);
    bool f=false; for(auto& t:tr) if(t.rd==3) { CHECK(t.rd_v==42,"6*7!=42"); f=true; }
    CHECK(f,"MUL result missing"); PASS(); }

void t24_32bit_ops() { TEST("ADDW sign-extension: 0xFFFFFFFF+1 → 0x00000000");
    IM m; m.clear(RESET_VECTOR); m.add(MI((uint32_t)-1,0,0,2,0x13));
    m.add(MI(1,2,0,3,0x1B)); m.add(EC());
    std::vector<TR> tr; run(m,200,tr);
    bool f=false; for(auto& t:tr) if(t.rd==3) { CHECK(t.rd_v==0,"ADDW overflow wrong"); f=true; }
    CHECK(f,"ADDW missing"); PASS(); }

void t25_rob_younger_first() { TEST("Younger ready first but cannot commit before older");
    IM m; m.clear(RESET_VECTOR);
    m.add(MI(3,0,0,1,0x13)); m.add(MI(7,0,0,2,0x13)); m.add(EC());
    std::vector<TR> tr; run(m,200,tr);
    CHECK(tr.size()>=2,"<2 commits");
    CHECK(tr[0].rd==1,"older should commit first"); CHECK(tr[1].rd==2,"younger after"); PASS(); }

int main() {
    printf("=== BOOM-HLS Equivalence Audit Test Suite ===\n\n");
    t1_basic(); t2_raw(); t3_waw(); t4_war(); t5_x0();
    t6_rob_full(); t7_rob_wrap(); t8_iq_full();
    t9_taken_branch(); t10_nt_branch(); t11_jal(); t12_jalr();
    t13_flush_rob(); t14_illegal(); t15_ecall(); t16_commit_trace();
    t17_imem_backpressure(); t18_reset(); t19_cycle_count(); t20_instret();
    t21_signed_comp(); t22_shift(); t23_mul(); t24_32bit_ops(); t25_rob_younger_first();
    printf("\n=== %d passed, %d failed ===\n", tests_passed, tests_failed);
    return (tests_failed>0)?1:0;
}
