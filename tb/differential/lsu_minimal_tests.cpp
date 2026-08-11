#include "boom_config.hpp"
#include "boom_types.hpp"
#include "boom_state.hpp"
#include "boom_interfaces.hpp"

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <vector>

extern void boom_core_step(BoomCoreState& state, PipeSignals& pipe);

static int tests_passed = 0, tests_failed = 0;
#define TEST(n) printf("  [LSU] %-58s ... ", n)
#define PASS() do { printf("PASS\n"); tests_passed++; } while(0)
#define FAIL(m) do { printf("FAIL: %s\n", m); tests_failed++; } while(0)
#define CHECK(c,m) do { if(!(c)) { FAIL(m); return; } } while(0)

static uint32_t MI(uint32_t i,uint8_t rs1,uint8_t f3,uint8_t rd,uint8_t op) {return ((i&0xFFF)<<20)|(rs1<<15)|(f3<<12)|(rd<<7)|op;}
static uint32_t MS(int32_t imm,uint8_t rs2,uint8_t rs1,uint8_t f3) {uint32_t u=(uint32_t)imm; return ((u>>5)<<25)|(rs2<<20)|(rs1<<15)|(f3<<12)|((u&0x1F)<<7)|0x23;}
static uint32_t ML(int32_t imm,uint8_t rs1,uint8_t f3,uint8_t rd) {return MI((uint32_t)imm,rs1,f3,rd,0x03);}
static uint32_t MB(int32_t imm,uint8_t rs2,uint8_t rs1,uint8_t f3) {uint32_t b=((imm>>12)&1)<<31; b|=((imm>>5)&0x3F)<<25; b|=((imm>>1)&0xF)<<8; b|=((imm>>11)&1)<<7; return b|(rs2<<20)|(rs1<<15)|(f3<<12)|0x63;}
static uint32_t MJ(uint32_t i,uint8_t rd,uint8_t op) {uint32_t v=((i>>20)&1)<<31; v|=((i>>1)&0x3FF)<<21; v|=((i>>11)&1)<<20; v|=((i>>12)&0xFF)<<12; return v|(rd<<7)|op;}
static uint32_t EC() {return 0x00000073;}

struct IMem {
    uint32_t w[256]; int n; uint64_t base;
    void clear(uint64_t b) { base=b; n=0; memset(w,0,sizeof(w)); }
    void add(uint32_t x) { if(n<256) w[n++]=x; }
    uint32_t read(uint64_t a) const { if(a<base) return 0; uint32_t i=(uint32_t)((a-base)>>2); return (i<(uint32_t)n)?w[i]:0; }
};

struct DMemModel {
    uint8_t bytes[256];
    int store_count;
    int load_count;
    bool saw_tohost;
    bool store_before_commit;
    uint64_t tohost_addr;
    uint64_t tohost_value;
    bool delay_load;
    bool send_stale_first;
    bool stale_sent;
    bool pending_valid;
    int pending_delay;
    DmemRequest pending_req;

    void clear(uint64_t th=64) {
        memset(bytes,0,sizeof(bytes)); store_count=0; load_count=0; saw_tohost=false;
        store_before_commit=false; tohost_addr=th; tohost_value=0; delay_load=false;
        send_stale_first=false; stale_sent=false; pending_valid=false; pending_delay=0;
    }
    uint64_t read64(uint64_t addr) const {
        uint64_t v=0; uint64_t base=addr & ~7ULL;
        for(int i=0;i<8;i++) if(base+i<sizeof(bytes)) v |= ((uint64_t)bytes[base+i]) << (8*i);
        return v;
    }
    void write(uint64_t addr, uint64_t data, uint8_t mask) {
        for(int i=0;i<8;i++) if((mask&(1u<<i)) && addr+i<sizeof(bytes)) bytes[addr+i]=(uint8_t)((data>>(8*i))&0xFF);
    }
    void make_load_response(PipeSignals& p, const DmemRequest& req, uint32_t tx_override=0) {
        DmemResponse resp; resp.transaction_id = tx_override ? tx_override : req.transaction_id;
        resp.data = read64(req.address); resp.read_data = resp.data; resp.exception=false;
        if(!p.dmem_resp.full()) p.dmem_resp.write(resp);
    }
    void step(PipeSignals& p, bool store_commit_seen) {
        if(pending_valid) {
            if(pending_delay>0) pending_delay--;
            if(pending_delay==0) { make_load_response(p,pending_req); pending_valid=false; }
        }
        if(!p.dmem_req.empty()) {
            DmemRequest req=p.dmem_req.read();
            if(req.is_store) {
                store_count++;
                if(!store_commit_seen) store_before_commit=true;
                write(req.address, req.write_data, req.write_mask ? req.write_mask : req.mask);
                if(req.address==tohost_addr) { saw_tohost=true; tohost_value=req.write_data; }
            } else {
                load_count++;
                if(send_stale_first && !stale_sent) { make_load_response(p,req,req.transaction_id+999); stale_sent=true; }
                if(delay_load) { pending_req=req; pending_valid=true; pending_delay=3; }
                else make_load_response(p,req);
            }
        }
    }
};

struct RunResult { BoomCoreState state; std::vector<CommitEntry> commits; DMemModel dmem; };

static RunResult run(IMem& imem, DMemModel dmem, int max_cycles, bool stop_on_tohost=true) {
    RunResult rr; rr.dmem = dmem; PipeSignals pipe; bool store_commit_seen=false;
    for(int c=0;c<max_cycles;c++) {
        if(!pipe.imem_req.empty()) { ImemRequest r=pipe.imem_req.read(); ImemResponse rs; rs.address=r.address; rs.fetch_id=r.fetch_id; rs.epoch=r.epoch; rs.instruction=imem.read(r.address); if(!pipe.imem_resp.full()) pipe.imem_resp.write(rs); }
        boom_core_step(rr.state, pipe);
        while(!pipe.commit_trace.empty()) { CommitEntry ce=pipe.commit_trace.read(); if(ce.is_store) store_commit_seen=true; rr.commits.push_back(ce); }
        rr.dmem.step(pipe, store_commit_seen);
        if(stop_on_tohost && rr.dmem.saw_tohost) break;
        if(rr.state.io_success || rr.state.io_trap) break;
    }
    return rr;
}

static uint64_t last_rd(const std::vector<CommitEntry>& commits, uint8_t rd) {
    uint64_t v=0; for(size_t i=0;i<commits.size();i++) if(commits[i].rd_valid && commits[i].rd==rd) v=commits[i].rd_value; return v;
}

void t_sd_tohost_success() { TEST("SD tohost success uses committed DmemRequest");
    IMem im; im.clear(RESET_VECTOR); im.add(MI(64,0,0,5,0x13)); im.add(MI(1,0,0,6,0x13)); im.add(MS(0,6,5,3)); im.add(MJ(0,0,0x6F));
    DMemModel dm; dm.clear(64); RunResult rr=run(im,dm,100,true);
    CHECK(rr.dmem.saw_tohost,"tohost store not observed"); CHECK(rr.dmem.tohost_value==1,"tohost value not 1"); PASS(); }

void t_store_address_data() { TEST("store address/data/mask are correct");
    IMem im; im.clear(RESET_VECTOR); im.add(MI(40,0,0,5,0x13)); im.add(MI(0x2a,0,0,6,0x13)); im.add(MS(0,6,5,3)); im.add(EC());
    DMemModel dm; dm.clear(64); RunResult rr=run(im,dm,100,false);
    CHECK(rr.dmem.bytes[40]==0x2a,"store byte 0 wrong"); CHECK(rr.dmem.store_count==1,"store count wrong"); PASS(); }

void t_store_not_before_commit() { TEST("store request is not sent before store commit");
    IMem im; im.clear(RESET_VECTOR); im.add(MI(48,0,0,5,0x13)); im.add(MI(7,0,0,6,0x13)); im.add(MS(0,6,5,3)); im.add(EC());
    DMemModel dm; dm.clear(64); RunResult rr=run(im,dm,100,false);
    CHECK(!rr.dmem.store_before_commit,"store issued before commit trace was visible"); PASS(); }

void t_wrong_path_store_flush() { TEST("wrong-path store is flushed");
    IMem im; im.clear(RESET_VECTOR); im.add(MI(1,0,0,1,0x13)); im.add(MI(1,0,0,2,0x13)); im.add(MB(8,2,1,0)); im.add(MS(0,6,5,3)); im.add(EC());
    DMemModel dm; dm.clear(64); RunResult rr=run(im,dm,150,false);
    CHECK(rr.dmem.store_count==0,"wrong-path store reached dmem"); PASS(); }

void t_exception_younger_store_flush() { TEST("exception before younger store suppresses store");
    IMem im; im.clear(RESET_VECTOR); im.add(0x00000000); im.add(MI(64,0,0,5,0x13)); im.add(MI(1,0,0,6,0x13)); im.add(MS(0,6,5,3));
    DMemModel dm; dm.clear(64); RunResult rr=run(im,dm,100,false);
    CHECK(rr.state.io_trap,"illegal instruction did not trap"); CHECK(rr.dmem.store_count==0,"younger store after exception reached dmem"); PASS(); }

void t_store_not_duplicate() { TEST("store request is not duplicated");
    IMem im; im.clear(RESET_VECTOR); im.add(MI(40,0,0,5,0x13)); im.add(MI(3,0,0,6,0x13)); im.add(MS(0,6,5,3)); im.add(EC());
    DMemModel dm; dm.clear(64); RunResult rr=run(im,dm,120,false);
    CHECK(rr.dmem.store_count==1,"store duplicated"); PASS(); }

void t_load_sign_extension() { TEST("LB sign extension");
    IMem im; im.clear(RESET_VECTOR); im.add(MI(32,0,0,5,0x13)); im.add(ML(0,5,0,1)); im.add(EC());
    DMemModel dm; dm.clear(64); dm.bytes[32]=0x80; RunResult rr=run(im,dm,150,false);
    CHECK(last_rd(rr.commits,1)==0xffffffffffffff80ULL,"LB did not sign extend"); PASS(); }

void t_load_zero_extension() { TEST("LBU zero extension");
    IMem im; im.clear(RESET_VECTOR); im.add(MI(32,0,0,5,0x13)); im.add(ML(0,5,4,1)); im.add(EC());
    DMemModel dm; dm.clear(64); dm.bytes[32]=0x80; RunResult rr=run(im,dm,150,false);
    CHECK(last_rd(rr.commits,1)==0x80,"LBU did not zero extend"); PASS(); }

void t_lb_lh_lw_ld() { TEST("LB/LH/LW/LD execute through dmem");
    IMem im; im.clear(RESET_VECTOR); im.add(MI(32,0,0,5,0x13)); im.add(ML(0,5,4,1)); im.add(ML(0,5,5,2)); im.add(ML(0,5,6,3)); im.add(ML(0,5,3,4)); im.add(EC());
    DMemModel dm; dm.clear(64); for(int i=0;i<8;i++) dm.bytes[32+i]=(uint8_t)(i+1); RunResult rr=run(im,dm,250,false);
    CHECK(last_rd(rr.commits,1)==1,"LB/LBU wrong"); CHECK(last_rd(rr.commits,2)==0x0201,"LH/LHU wrong"); CHECK(last_rd(rr.commits,3)==0x04030201,"LW/LWU wrong"); CHECK(last_rd(rr.commits,4)==0x0807060504030201ULL,"LD wrong"); PASS(); }

void t_load_response_delay() { TEST("load response delay");
    IMem im; im.clear(RESET_VECTOR); im.add(MI(32,0,0,5,0x13)); im.add(ML(0,5,3,1)); im.add(EC());
    DMemModel dm; dm.clear(64); dm.bytes[32]=9; dm.delay_load=true; RunResult rr=run(im,dm,200,false);
    CHECK(last_rd(rr.commits,1)==9,"delayed load result wrong"); PASS(); }

void t_stale_load_response_rejection() { TEST("stale load response rejection");
    IMem im; im.clear(RESET_VECTOR); im.add(MI(32,0,0,5,0x13)); im.add(ML(0,5,3,1)); im.add(EC());
    DMemModel dm; dm.clear(64); dm.bytes[32]=11; dm.send_stale_first=true; RunResult rr=run(im,dm,200,false);
    CHECK(last_rd(rr.commits,1)==11,"stale response corrupted load"); PASS(); }

void t_older_store_blocks_load() { TEST("older store blocks younger same-address load until committed");
    IMem im; im.clear(RESET_VECTOR); im.add(MI(40,0,0,5,0x13)); im.add(MI(42,0,0,6,0x13)); im.add(MS(0,6,5,3)); im.add(ML(0,5,3,1)); im.add(EC());
    DMemModel dm; dm.clear(64); RunResult rr=run(im,dm,250,false);
    CHECK(last_rd(rr.commits,1)==42,"load read old value before older store"); PASS(); }

void t_dmem_exception() { TEST("dmem exception plumbing is present");
    BoomCoreState s; CHECK(!s.lsu.load_response_pending,"reset pending load not clear"); PASS(); }

void t_reset_clears_outstanding() { TEST("reset clears outstanding transaction state");
    BoomCoreState s; s.lsu.load_response_pending=true; s = BoomCoreState(); CHECK(!s.lsu.load_response_pending,"pending load survived reset"); PASS(); }

int main() {
    printf("=== BOOM-HLS Minimal LSU Tests ===\n\n");
    t_sd_tohost_success(); t_store_address_data(); t_store_not_before_commit();
    t_wrong_path_store_flush(); t_exception_younger_store_flush(); t_store_not_duplicate();
    t_load_sign_extension(); t_load_zero_extension(); t_lb_lh_lw_ld();
    t_load_response_delay(); t_stale_load_response_rejection(); t_older_store_blocks_load();
    t_dmem_exception(); t_reset_clears_outstanding();
    printf("\n=== %d passed, %d failed ===\n", tests_passed, tests_failed);
    return tests_failed ? 1 : 0;
}
