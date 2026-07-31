#include "boom_config.hpp"
#include "boom_interfaces.hpp"
#include "boom_state.hpp"
#include "reset.hpp"

#include <cstdint>
#include <cstdio>
#include <map>
#include <set>
#include <vector>

namespace boom {
void rob_allocate(BoomCoreState&);
void issue_module(BoomCoreState&);
void execute_module(BoomCoreState&);
void rob_complete(BoomCoreState&);
void lsu_module(BoomCoreState&, PipeSignals&);
void rob_commit_module(BoomCoreState&, PipeSignals&);
}

namespace {

const int kSeeds = 100;
const int kCycles = 64;

struct Rng {
    uint32_t s;
    explicit Rng(uint32_t seed) : s(seed ? seed : 1) {}
    uint32_t next() { uint32_t x=s; x^=x<<13; x^=x>>17; x^=x<<5; return s=x; }
    uint32_t range(uint32_t n) { return next()%n; }
    bool one_in(uint32_t n) { return range(n)==0; }
    bool bit() { return (next()&1)!=0; }
};

struct Metrics {
    uint64_t seeds, random_cycles, generated, rob_allocations, rob_index_reuses;
    uint64_t rob_index_wraps, allocation_id_wraps, dispatch_retries, iq_entries;
    uint64_t accepted, dual_accepts, retained, held_completion_cycles;
    uint64_t completion_consumed, loads, stores, ldq_full_cycles, stq_full_cycles;
    uint64_t ldq_drains, stq_drains, load_requests, load_responses, response_delay_cycles;
    uint64_t stale_responses, stale_completion_ids, branch_resolves, branch_mispredicts;
    uint64_t branch_kills, resets, reset_kills, trace_backpressure, dmem_backpressure;
    uint64_t commits, consumed, killed, dropped, duplicates;
    uint64_t ready_masks[4], completion_masks[4];
    Metrics() : seeds(kSeeds), random_cycles(kSeeds*kCycles), generated(0), rob_allocations(0),
        rob_index_reuses(0), rob_index_wraps(0), allocation_id_wraps(0), dispatch_retries(0),
        iq_entries(0), accepted(0), dual_accepts(0), retained(0), held_completion_cycles(0),
        completion_consumed(0), loads(0), stores(0), ldq_full_cycles(0), stq_full_cycles(0),
        ldq_drains(0), stq_drains(0), load_requests(0), load_responses(0),
        response_delay_cycles(0), stale_responses(0), stale_completion_ids(0),
        branch_resolves(0), branch_mispredicts(0), branch_kills(0), resets(0),
        reset_kills(0), trace_backpressure(0), dmem_backpressure(0), commits(0),
        consumed(0), killed(0), dropped(0), duplicates(0) {
        for (int i=0;i<4;i++) ready_masks[i]=completion_masks[i]=0;
    }
};

enum Kind { K_ALU, K_LOAD, K_STORE, K_BRANCH };
enum EndState { LIVE, CONSUMED, KILLED };

struct TokenRecord {
    Kind kind;
    EndState end;
    uint8_t rob_idx;
    uint32_t allocation_id;
    unsigned accepts, completions;
    bool allocated;
    TokenRecord() : kind(K_ALU), end(LIVE), rob_idx(0), allocation_id(0),
        accepts(0), completions(0), allocated(false) {}
};

struct ResponsePlan {
    uint32_t tx;
    uint64_t data;
    int delay;
    ResponsePlan(uint32_t t, uint64_t d, int n) : tx(t), data(d), delay(n) {}
};

uint64_t token_of(const MicroOp& u) { return u.debug_pc; }

MicroOp make_uop(uint64_t token, Kind kind, uint8_t branch_mask, bool branch_taken) {
    MicroOp u;
    u.debug_pc=token; u.debug_inst=(uint32_t)token; u.inst=(uint32_t)token;
    u.rename.prs1=1; u.rename.prs2=2; u.branch.br_mask=branch_mask;
    u.imm_packed=(uint32_t)(token&31); u.mem.mem_size=(uint8_t)(token&3);
    if (kind==K_LOAD) {
        u.uopc=(uint8_t)(39+(token%7)); u.iq_type=IQ_MEM; u.fu_code=FU_MEM;
        u.ctrl.is_load=true; u.mem.uses_ldq=true; u.mem.mem_signed=(token&1)!=0;
        u.rename.dst_rtype=DST_INT; u.rename.pdst=(uint8_t)(3+(token%47));
    } else if (kind==K_STORE) {
        u.uopc=(uint8_t)(46+(token%4)); u.iq_type=IQ_MEM; u.fu_code=FU_MEM;
        u.ctrl.is_sta=true; u.mem.uses_stq=true; u.rename.dst_rtype=DST_N;
    } else if (kind==K_BRANCH) {
        u.uopc=branch_taken?31:32; u.iq_type=IQ_ALU; u.fu_code=FU_ALU;
        u.branch.is_br=true; u.branch.br_tag=0; u.rename.dst_rtype=DST_N;
    } else {
        u.uopc=(token&1)?1:2; u.iq_type=IQ_ALU; u.fu_code=FU_ALU;
        u.rename.dst_rtype=DST_INT; u.rename.pdst=(uint8_t)(3+(token%47));
    }
    return u;
}

bool preg_busy(const BoomCoreState& s, uint8_t p) {
    return p!=0 && p<INT_PHYS_REGS && s.rename.int_free_list.busy_table[p];
}

enum RefPort { RP_INT, RP_MEM, RP_NONE };
RefPort ref_port(const MicroOp& u) {
    if (u.exception) return RP_NONE;
    if (u.iq_type==IQ_MEM && u.fu_code==FU_MEM && u.uopc>=39 && u.uopc<=49) return RP_MEM;
    if (u.iq_type!=IQ_ALU) return RP_NONE;
    if (u.fu_code==FU_MUL) return u.uopc==16 ? RP_INT : RP_NONE;
    if (u.fu_code!=FU_ALU) return RP_NONE;
    if ((u.uopc>=1&&u.uopc<=13)||u.uopc==15||(u.uopc>=29&&u.uopc<=38)||
        (u.uopc>=50&&u.uopc<=60)||u.uopc==62) return RP_INT;
    return RP_NONE;
}

struct RefGrant {
    bool valid, accepted, dispatch;
    int index;
    MicroOp uop;
    RefGrant() : valid(false), accepted(false), dispatch(false), index(-1), uop() {}
};

struct RefIssue {
    RefGrant grant[2];
    std::vector<IssueSlotEntry> iq;
    bool dispatch_valid;
    int generated, accepted, retained, killed;
    RefIssue() : dispatch_valid(false), generated(0), accepted(0), retained(0), killed(0) {}
};

RefIssue ref_issue(const BoomCoreState& in) {
    RefIssue r;
    for (int i=0;i<ISSUE_QUEUE_ALU_DEPTH;i++) {
        IssueSlotEntry e=in.issue.alu_iq.entries[i];
        if (!e.valid) continue;
        if (in.brupdate.valid&&in.brupdate.mispredict&&
            (e.uop.branch.br_mask&in.brupdate.mispredict_mask)) { r.killed++; continue; }
        if (e.uop.exception) { r.killed++; continue; }
        if (in.brupdate.valid) e.uop.branch.br_mask&=(uint8_t)~in.brupdate.resolve_mask;
        if (e.uop.rename.prs1) e.prs1_busy=preg_busy(in,e.uop.rename.prs1);
        if (e.uop.rename.prs2) e.prs2_busy=preg_busy(in,e.uop.rename.prs2);
        if (!e.granted) r.iq.push_back(e);
    }
    for (size_t i=0;i<r.iq.size();i++) {
        const IssueSlotEntry& e=r.iq[i];
        if (!e.request||e.killed||e.prs1_busy||e.prs2_busy||e.pdst_busy) continue;
        RefPort p=ref_port(e.uop); int lane=p==RP_MEM?MEM_ISSUE_LANE:(p==RP_INT?INT_ISSUE_LANE:-1);
        if (lane>=0&&!r.grant[lane].valid) {
            r.grant[lane].valid=true; r.grant[lane].index=(int)i; r.grant[lane].uop=e.uop;
        }
    }
    const RenameDispatchPacket& d=in.rename.dispatch_packets[0];
    r.dispatch_valid=d.valid;
    bool dispatch_pending=d.valid&&d.rob_allocated&&!d.uop.exception;
    if (dispatch_pending) {
        RefPort p=ref_port(d.uop); int lane=p==RP_MEM?MEM_ISSUE_LANE:(p==RP_INT?INT_ISSUE_LANE:-1);
        bool ready=!preg_busy(in,d.uop.rename.prs1)&&!preg_busy(in,d.uop.rename.prs2);
        bool old_accept=false;
        for (int l=0;l<2;l++) old_accept|=r.grant[l].valid&&in.issue.port_ready[l];
        bool preservable=r.iq.size()<ISSUE_QUEUE_ALU_DEPTH||
            (lane>=0&&in.issue.port_ready[lane])||old_accept;
        if (lane>=0&&ready&&preservable&&!r.grant[lane].valid) {
            r.grant[lane].valid=true; r.grant[lane].dispatch=true; r.grant[lane].uop=d.uop;
        }
    }
    for (int lane=0;lane<2;lane++) {
        RefGrant& g=r.grant[lane];
        if (!g.valid) continue;
        r.generated++;
        bool downstream=in.issue.port_ready[lane];
        if (lane==MEM_ISSUE_LANE) {
            bool load=g.uop.ctrl.is_load||g.uop.mem.uses_ldq||(g.uop.uopc>=39&&g.uop.uopc<=45);
            bool store=g.uop.ctrl.is_sta||g.uop.mem.uses_stq||(g.uop.uopc>=46&&g.uop.uopc<=49);
            if (load) downstream&=in.lsu.ldq_count<LDQ_DEPTH;
            if (store) downstream&=in.lsu.stq_count<STQ_DEPTH;
        }
        if (!downstream) continue;
        g.accepted=true; r.accepted++;
        if (g.dispatch) { r.dispatch_valid=false; dispatch_pending=false; }
        else { r.iq[g.index].granted=true; r.iq[g.index].request=false; }
    }
    r.retained=r.generated-r.accepted;
    std::vector<IssueSlotEntry> compact;
    for (size_t i=0;i<r.iq.size();i++) if (r.iq[i].valid&&!r.iq[i].granted) compact.push_back(r.iq[i]);
    r.iq=compact;
    if (dispatch_pending&&r.iq.size()<ISSUE_QUEUE_ALU_DEPTH) {
        IssueSlotEntry e; e.valid=true; e.request=true; e.uop=d.uop;
        e.prs1_busy=preg_busy(in,e.uop.rename.prs1); e.prs2_busy=preg_busy(in,e.uop.rename.prs2);
        r.iq.push_back(e); r.dispatch_valid=false;
    }
    if (d.valid&&d.rob_allocated&&d.uop.exception) r.dispatch_valid=false;
    return r;
}

const char* compare_issue(const RefIssue& r, const BoomCoreState& a) {
    if (a.issue.grants_generated!=r.generated) return "generated grant count";
    if (a.issue.grants_accepted!=r.accepted) return "accepted grant count";
    if (a.issue.grants_retained!=r.retained||a.issue.grants_dropped) return "retained/dropped grant count";
    for (int lane=0;lane<2;lane++) {
        const IssueGrant& g=a.issue.grants[lane];
        if (g.valid!=r.grant[lane].valid||g.accepted!=r.grant[lane].accepted||
            g.from_dispatch!=r.grant[lane].dispatch) return "grant lane/handshake";
        if (g.valid&&(token_of(g.uop)!=token_of(r.grant[lane].uop)||
            g.uop.queue.rob_allocation_id!=r.grant[lane].uop.queue.rob_allocation_id)) return "grant identity";
        if (a.issue.issued_valids[lane]!=r.grant[lane].accepted) return "issued valid";
    }
    if (a.issue.issued_valids[FP_ISSUE_LANE]||a.issue.grants[FP_ISSUE_LANE].valid) return "reserved FP lane";
    if (a.issue.alu_iq.count!=r.iq.size()) return "IQ count";
    for (size_t i=0;i<r.iq.size();i++) {
        const IssueSlotEntry& x=a.issue.alu_iq.entries[i];
        if (!x.valid||token_of(x.uop)!=token_of(r.iq[i].uop)||
            x.uop.queue.rob_allocation_id!=r.iq[i].uop.queue.rob_allocation_id||
            x.uop.branch.br_mask!=r.iq[i].uop.branch.br_mask||
            x.prs1_busy!=r.iq[i].prs1_busy||x.prs2_busy!=r.iq[i].prs2_busy) return "IQ survivor/order";
    }
    if (a.rename.dispatch_packets[0].valid!=r.dispatch_valid) return "dispatch consumption";
    return 0;
}

ExecuteState::AluResult ref_execute_one(const BoomCoreState& before, const MicroOp& u) {
    ExecuteState::AluResult r; r.valid=true; r.uop=u;
    uint64_t rs1=u.rename.prs1?before.int_rf[u.rename.prs1]:0;
    uint64_t rs2=u.rename.prs2?before.int_rf[u.rename.prs2]:0;
    if (u.uopc>=39&&u.uopc<=45) {
        r.memory_valid=true; r.is_load=true; r.signed_load=u.mem.mem_signed;
        r.memory_size=u.mem.mem_size; r.memory_address=rs1+(int64_t)(int32_t)u.imm_packed;
        r.memory_mask=(uint8_t)(u.mem.mem_size>=3?0xff:((1u<<(1u<<u.mem.mem_size))-1u));
        r.result=r.memory_address;
    } else if (u.uopc>=46&&u.uopc<=49) {
        r.memory_valid=true; r.is_store=true; r.memory_size=u.mem.mem_size;
        r.memory_address=rs1+(int64_t)(int32_t)u.imm_packed; r.store_data=rs2;
        r.memory_mask=(uint8_t)(u.mem.mem_size>=3?0xff:((1u<<(1u<<u.mem.mem_size))-1u));
        r.result=r.memory_address;
    } else if (u.uopc==2) r.result=rs1-rs2;
    else if (u.uopc==31||u.uopc==32) {
        r.mispredict=u.uopc==31?(rs1==rs2):(rs1!=rs2);
        r.redirect_pc=u.debug_pc+(int64_t)(int32_t)u.imm_packed;
    } else r.result=rs1+rs2;
    return r;
}

const char* compare_result(const ExecuteState::AluResult& e, const ExecuteState::AluResult& a) {
    if (a.valid!=e.valid) return "result valid";
    if (!e.valid) return 0;
    if (token_of(a.uop)!=token_of(e.uop)||
        a.uop.queue.rob_allocation_id!=e.uop.queue.rob_allocation_id) return "result identity/overwrite";
    if (a.result!=e.result||a.memory_valid!=e.memory_valid||a.is_load!=e.is_load||
        a.is_store!=e.is_store||a.mispredict!=e.mispredict) return "result value/type";
    if (e.memory_valid&&(a.memory_address!=e.memory_address||a.store_data!=e.store_data||
        a.memory_mask!=e.memory_mask)) return "memory payload";
    return 0;
}

struct RefCompletion {
    ExecuteState::AluResult slots[2];
    bool rob_busy[ROB_DEPTH];
    uint64_t rf[INT_PHYS_REGS];
    bool preg_busy[INT_PHYS_REGS];
    uint8_t ldq_count, stq_count;
    int consumed;
    bool branch;
    RefCompletion() : ldq_count(0), stq_count(0), consumed(0), branch(false) {}
};

RefCompletion ref_complete(const BoomCoreState& in) {
    RefCompletion r;
    for (int l=0;l<2;l++) r.slots[l]=in.execute.alu_results[l];
    for (int i=0;i<ROB_DEPTH;i++) r.rob_busy[i]=in.rob.entries[i].busy;
    for (int i=0;i<INT_PHYS_REGS;i++) { r.rf[i]=in.int_rf[i]; r.preg_busy[i]=in.rename.int_free_list.busy_table[i]; }
    r.ldq_count=in.lsu.ldq_count; r.stq_count=in.lsu.stq_count;
    int selected=-1; unsigned age=ROB_DEPTH;
    for (int l=0;l<2;l++) {
        if (!r.slots[l].valid) continue;
        uint8_t idx=r.slots[l].uop.queue.rob_idx;
        if (idx>=ROB_DEPTH||!in.rob.entries[idx].valid||
            in.rob.entries[idx].uop.queue.rob_allocation_id!=r.slots[l].uop.queue.rob_allocation_id) {
            r.slots[l]=ExecuteState::AluResult(); continue;
        }
        unsigned a=(idx+ROB_DEPTH-in.rob.head)%ROB_DEPTH;
        if (selected<0||a<age) { selected=l; age=a; }
    }
    if (selected<0) return r;
    ExecuteState::AluResult& x=r.slots[selected]; uint8_t idx=x.uop.queue.rob_idx;
    if (x.uop.branch.is_br||x.uop.branch.is_jal||x.uop.branch.is_jalr) { r.branch=true; return r; }
    if (x.is_load&&r.ldq_count>=LDQ_DEPTH) return r;
    if (x.is_store&&r.stq_count>=STQ_DEPTH) return r;
    if (x.is_load) r.ldq_count++;
    if (x.is_store) r.stq_count++;
    if (!x.is_load) r.rob_busy[idx]=false;
    if (!x.is_load&&x.uop.rename.dst_rtype==DST_INT&&x.uop.rename.pdst) {
        r.rf[x.uop.rename.pdst]=x.result; r.preg_busy[x.uop.rename.pdst]=false;
    }
    x=ExecuteState::AluResult(); r.consumed=1;
    return r;
}

const char* compare_completion(const RefCompletion& r, const BoomCoreState& a) {
    if (r.branch) return 0;
    for (int l=0;l<2;l++) { const char* why=compare_result(r.slots[l],a.execute.alu_results[l]); if (why) return why; }
    for (int i=0;i<ROB_DEPTH;i++) if (a.rob.entries[i].busy!=r.rob_busy[i]) return "ROB busy";
    for (int i=1;i<INT_PHYS_REGS;i++) {
        if (a.int_rf[i]!=r.rf[i]) return "writeback value";
        if (a.rename.int_free_list.busy_table[i]!=r.preg_busy[i]) return "busy-table wakeup";
    }
    if (a.lsu.ldq_count!=r.ldq_count||a.lsu.stq_count!=r.stq_count) return "LSU admission";
    return 0;
}

void failure(uint32_t seed, int cycle, const char* phase, const char* why, const BoomCoreState& s) {
    std::printf("FIRST_MISMATCH seed=0x%08x cycle=%d phase=%s reason=%s\n",seed,cycle,phase,why);
    std::printf("  rob=%u/%u/%d iq=%u dispatch=%d ldq/stq=%u/%u pending=%d tx=%u br=%d/%d\n",
        s.rob.head,s.rob.tail,s.rob.maybe_full,s.issue.alu_iq.count,
        s.rename.dispatch_packets[0].valid,s.lsu.ldq_count,s.lsu.stq_count,
        s.lsu.load_response_pending,s.lsu.pending_load_transaction_id,
        s.brupdate.valid,s.brupdate.mispredict);
}

bool same_result(const ExecuteState::AluResult& a, const ExecuteState::AluResult& b) {
    return a.valid==b.valid&&(!a.valid||(token_of(a.uop)==token_of(b.uop)&&
        a.uop.queue.rob_allocation_id==b.uop.queue.rob_allocation_id&&a.result==b.result&&
        a.memory_address==b.memory_address&&a.store_data==b.store_data));
}

std::set<uint64_t> rob_tokens(const BoomCoreState& s) {
    std::set<uint64_t> out;
    for (int i=0;i<ROB_DEPTH;i++) if (s.rob.entries[i].valid) out.insert(token_of(s.rob.entries[i].uop));
    return out;
}

void drain_dmem(PipeSignals& p, std::vector<DmemRequest>& out) {
    while (!p.dmem_req.empty()) { DmemRequest r=p.dmem_req.read(); if (r.transaction_id) out.push_back(r); }
}

void drain_trace(PipeSignals& p, std::vector<CommitEntry>& out) {
    while (!p.commit_trace.empty()) { CommitEntry c=p.commit_trace.read(); if (c.valid) out.push_back(c); }
}

void fill_dmem(PipeSignals& p) { DmemRequest d; for (int i=0;i<1024;i++) p.dmem_req.write(d); }
void fill_trace(PipeSignals& p) { CommitEntry c; for (int i=0;i<1024;i++) p.commit_trace.write(c); }

uint64_t load_value(uint64_t data, uint64_t address, uint8_t size, bool sign) {
    uint8_t bytes=(uint8_t)(1u<<(size&3)); uint8_t bits=(uint8_t)(bytes*8);
    uint8_t shift=(uint8_t)((address&7)*8); uint64_t mask=bits==64?~0ULL:((1ULL<<bits)-1);
    uint64_t v=(data>>shift)&mask;
    if (sign&&bits<64) { uint64_t top=1ULL<<(bits-1); v=(v^top)-top; }
    return v;
}

struct Harness {
    uint32_t seed;
    Rng rng;
    BoomCoreState dut;
    PipeSignals pipe;
    Metrics& m;
    std::map<uint64_t,TokenRecord> tokens;
    std::vector<ResponsePlan> responses;
    uint64_t next_token;
    uint32_t last_allocation_id;
    uint32_t last_index_id[ROB_DEPTH];
    int reset_cycle, trace_block, dmem_block;
    bool failed;

    Harness(uint32_t sd, uint64_t first, Metrics& metrics) : seed(sd), rng(sd), m(metrics),
        next_token(first), last_allocation_id(0), reset_cycle(28+(int)rng.range(21)),
        trace_block(0), dmem_block(0), failed(false) {
        for (int i=0;i<ROB_DEPTH;i++) last_index_id[i]=0;
        dut.rob.state=ROB_NORMAL;
        dut.rob.next_allocation_id=(seed&1)?0xfffffff8u:(1u+rng.range(100000));
        dut.int_rf[1]=0x1000+(seed&0xff); dut.int_rf[2]=0x80+(seed&0x3f);
    }

    bool check(bool condition, int cycle, const char* phase, const char* why) {
        if (condition) return true;
        failure(seed,cycle,phase,why,dut); failed=true; return false;
    }

    void mark_disappeared(const std::set<uint64_t>& before, bool reset, int cycle) {
        std::set<uint64_t> after=rob_tokens(dut);
        for (std::set<uint64_t>::const_iterator it=before.begin();it!=before.end();++it) {
            if (after.count(*it)) continue;
            std::map<uint64_t,TokenRecord>::iterator tr=tokens.find(*it);
            if (tr==tokens.end()||tr->second.end!=LIVE) continue;
            tr->second.end=KILLED; m.killed++;
            if (reset) { m.reset_kills++; } else { m.branch_kills++; }
        }
        (void)cycle;
    }

    void account_commit(const CommitEntry& ce, int cycle) {
        uint64_t token=ce.pc;
        std::map<uint64_t,TokenRecord>::iterator it=tokens.find(token);
        if (!check(it!=tokens.end(),cycle,"accounting","commit of unknown token")) return;
        if (!check(it->second.end==LIVE,cycle,"accounting","duplicate terminal token")) { m.duplicates++; return; }
        it->second.end=CONSUMED; m.commits++; m.consumed++;
    }

    bool reset(int cycle) {
        std::set<uint64_t> live=rob_tokens(dut);
        if (dut.rename.dispatch_packets[0].valid&&!dut.rename.dispatch_packets[0].rob_allocated) {
            uint64_t t=token_of(dut.rename.dispatch_packets[0].uop);
            if (tokens[t].end==LIVE) { tokens[t].end=KILLED; m.killed++; m.reset_kills++; }
        }
        ResetControllerState rc;
        for (int n=0;!rc.completed&&n<256;n++) boom_core_reset_step(dut,rc);
        m.resets++; mark_disappeared(live,true,cycle);
        std::vector<DmemRequest> rq; std::vector<CommitEntry> ce;
        drain_dmem(pipe,rq); drain_trace(pipe,ce);
        while (!pipe.dmem_resp.empty()) pipe.dmem_resp.read();
        return check(rc.completed&&!dut.rename.dispatch_packets[0].valid&&dut.issue.alu_iq.count==0&&
            !dut.execute.alu_results[0].valid&&!dut.execute.alu_results[1].valid&&
            dut.lsu.ldq_count==0&&dut.lsu.stq_count==0,cycle,"reset","in-flight state survived reset");
    }

    bool stale_completion_probe(int cycle) {
        if (dut.rename.dispatch_packets[0].valid) return true;
        uint64_t token=next_token++;
        dut.rename.dispatch_packets[0].valid=true;
        dut.rename.dispatch_packets[0].uop=make_uop(token,K_ALU,0,false);
        tokens[token].kind=K_ALU; m.generated++;
        boom::rob_allocate(dut);
        RenameDispatchPacket& p=dut.rename.dispatch_packets[0];
        TokenRecord& tr=tokens[token]; tr.allocated=true; tr.rob_idx=p.uop.queue.rob_idx;
        tr.allocation_id=p.uop.queue.rob_allocation_id;
        m.rob_allocations++; last_index_id[tr.rob_idx]=tr.allocation_id;
        ExecuteState::AluResult stale=ref_execute_one(dut,p.uop);
        stale.uop.queue.rob_allocation_id=tr.allocation_id+1;
        stale.is_store=true; stale.memory_valid=true; stale.uop.branch.is_br=true;
        stale.uop.rename.pdst=7; stale.result=0xdeadbeef;
        dut.rename.int_free_list.busy_table[7]=true;
        bool busy=dut.rob.entries[tr.rob_idx].busy; uint8_t stq=dut.lsu.stq_count;
        uint64_t rf=dut.int_rf[7]; bool br=dut.brupdate.valid;
        dut.execute.alu_results[MEM_ISSUE_LANE]=stale;
        boom::rob_complete(dut); m.stale_completion_ids++;
        return check(!dut.execute.alu_results[MEM_ISSUE_LANE].valid&&dut.rob.entries[tr.rob_idx].busy==busy&&
            dut.lsu.stq_count==stq&&dut.int_rf[7]==rf&&dut.brupdate.valid==br&&
            dut.rename.int_free_list.busy_table[7],cycle,"stale-completion","mismatched allocation ID had side effects");
    }

    bool stale_response_probe(int cycle, uint32_t tx) {
        bool pending=dut.lsu.load_response_pending; uint32_t pending_tx=dut.lsu.pending_load_transaction_id;
        uint8_t ldq=dut.lsu.ldq_count; uint64_t rf[INT_PHYS_REGS]; bool busy[ROB_DEPTH];
        for (int i=0;i<INT_PHYS_REGS;i++) rf[i]=dut.int_rf[i];
        for (int i=0;i<ROB_DEPTH;i++) busy[i]=dut.rob.entries[i].busy;
        fill_dmem(pipe);
        DmemResponse d; d.transaction_id=tx; d.data=d.read_data=0xf00dbaad;
        pipe.dmem_resp.write(d); boom::lsu_module(dut,pipe);
        std::vector<DmemRequest> ignored; drain_dmem(pipe,ignored);
        bool same=pending==dut.lsu.load_response_pending&&pending_tx==dut.lsu.pending_load_transaction_id&&ldq==dut.lsu.ldq_count;
        for (int i=0;i<INT_PHYS_REGS;i++) same&=rf[i]==dut.int_rf[i];
        for (int i=0;i<ROB_DEPTH;i++) same&=busy[i]==dut.rob.entries[i].busy;
        m.stale_responses++;
        return check(same,cycle,"stale-response","mismatched transaction ID had side effects");
    }

    void generate(int cycle, bool draining) {
        if (draining||dut.rename.dispatch_packets[0].valid) return;
        Kind kind;
        if (!dut.branch_state.active_mask&&((cycle%13)==5||rng.one_in(13))) kind=K_BRANCH;
        else { uint32_t k=rng.range(10); kind=k<3?K_LOAD:(k<6?K_STORE:K_ALU); }
        bool taken=rng.bit(); uint64_t token=next_token++;
        uint8_t mask=dut.branch_state.active_mask;
        dut.rename.dispatch_packets[0].valid=true;
        dut.rename.dispatch_packets[0].uop=make_uop(token,kind,mask,taken);
        TokenRecord& tr=tokens[token]; tr.kind=kind; m.generated++;
        if (kind==K_LOAD) m.loads++; else if (kind==K_STORE) m.stores++;
    }

    void allocate(int cycle, bool draining) {
        RenameDispatchPacket& p=dut.rename.dispatch_packets[0];
        if (!p.valid) return;
        uint64_t token=token_of(p.uop); bool was_alloc=p.rob_allocated;
        uint8_t old_tail=dut.rob.tail; uint32_t old_next=dut.rob.next_allocation_id;
        if (draining||rng.range(4)!=0) boom::rob_allocate(dut);
        if (!was_alloc&&p.rob_allocated) {
            TokenRecord& tr=tokens[token]; tr.allocated=true; tr.rob_idx=p.uop.queue.rob_idx;
            tr.allocation_id=p.uop.queue.rob_allocation_id; m.rob_allocations++;
            if (last_index_id[tr.rob_idx]&&last_index_id[tr.rob_idx]!=tr.allocation_id) m.rob_index_reuses++;
            last_index_id[tr.rob_idx]=tr.allocation_id;
            if (dut.rob.tail<old_tail) m.rob_index_wraps++;
            if (last_allocation_id&&tr.allocation_id<last_allocation_id) m.allocation_id_wraps++;
            last_allocation_id=tr.allocation_id;
            if (tr.kind==K_BRANCH) {
                dut.branch_state.active_mask|=1; dut.branch_state.tag_valid[0]=true;
                dut.branch_state.snapshot_valid[0]=true;
            }
        } else if (!p.rob_allocated) {
            check(dut.rob.next_allocation_id==old_next,cycle,"allocation","retry changed allocation ID source");
        }
    }

    bool check_live_locations(int cycle) {
        std::set<uint64_t> pipe_tokens;
        std::set<uint64_t> lsu_tokens;
        if (dut.rename.dispatch_packets[0].valid) pipe_tokens.insert(token_of(dut.rename.dispatch_packets[0].uop));
        for (int i=0;i<dut.issue.alu_iq.count;i++) {
            uint64_t t=token_of(dut.issue.alu_iq.entries[i].uop);
            if (!pipe_tokens.insert(t).second) { m.duplicates++; return check(false,cycle,"accounting","duplicate dispatch/IQ token"); }
        }
        for (int l=0;l<2;l++) if (dut.execute.alu_results[l].valid) {
            uint64_t t=token_of(dut.execute.alu_results[l].uop);
            if (!pipe_tokens.insert(t).second) { m.duplicates++; return check(false,cycle,"accounting","duplicate IQ/completion token"); }
        }
        for (std::map<uint64_t,TokenRecord>::const_iterator it=tokens.begin();it!=tokens.end();++it) {
            if (it->second.end!=LIVE||!it->second.allocated) continue;
            uint8_t idx=it->second.rob_idx;
            if (!check(idx<ROB_DEPTH&&dut.rob.entries[idx].valid&&token_of(dut.rob.entries[idx].uop)==it->first&&
                dut.rob.entries[idx].uop.queue.rob_allocation_id==it->second.allocation_id,
                cycle,"accounting","live token lost ROB ownership")) return false;
        }
        for (int i=0;i<LDQ_DEPTH;i++) if (dut.lsu.ldq[i].valid) {
            const LoadQueueEntry& q=dut.lsu.ldq[i];
            if (!check(q.rob_idx<ROB_DEPTH&&dut.rob.entries[q.rob_idx].valid&&
                dut.rob.entries[q.rob_idx].uop.queue.rob_allocation_id==q.rob_allocation_id,
                cycle,"accounting","LDQ token lost ROB identity")) return false;
            uint64_t t=token_of(dut.rob.entries[q.rob_idx].uop);
            if (!check(lsu_tokens.insert(t).second&&tokens[t].accepts==1&&tokens[t].completions==1,
                cycle,"accounting","invalid or duplicate LDQ token transition")) return false;
        }
        for (int i=0;i<STQ_DEPTH;i++) if (dut.lsu.stq[i].valid) {
            const StoreQueueEntry& q=dut.lsu.stq[i];
            if (!check(q.rob_idx<ROB_DEPTH&&dut.rob.entries[q.rob_idx].valid&&
                dut.rob.entries[q.rob_idx].uop.queue.rob_allocation_id==q.rob_allocation_id,
                cycle,"accounting","STQ token lost ROB identity")) return false;
            uint64_t t=token_of(dut.rob.entries[q.rob_idx].uop);
            if (!check(lsu_tokens.insert(t).second&&tokens[t].accepts==1&&tokens[t].completions==1,
                cycle,"accounting","invalid or duplicate STQ token transition")) return false;
        }
        return true;
    }

    bool step(int cycle, bool draining) {
        dut.brupdate.valid=false; dut.brupdate.mispredict=false;
        if (!draining&&cycle==reset_cycle) if (!reset(cycle)) return false;

        if (!draining&&rng.one_in(11)) trace_block=1+(int)rng.range(6);
        if (!draining&&rng.one_in(9)) dmem_block=1+(int)rng.range(6);
        bool trace_full=trace_block>0, dmem_full=dmem_block>0;
        if (trace_full) { fill_trace(pipe); m.trace_backpressure++; trace_block--; }
        if (dmem_full) { fill_dmem(pipe); m.dmem_backpressure++; dmem_block--; }

        bool sent_response=false;
        for (size_t i=0;i<responses.size();i++) if (responses[i].delay>0) {
            responses[i].delay--; m.response_delay_cycles++;
        }
        for (size_t i=0;i<responses.size();i++) if (responses[i].delay<=0) {
            DmemResponse d; d.transaction_id=responses[i].tx; d.data=d.read_data=responses[i].data;
            pipe.dmem_resp.write(d); responses.erase(responses.begin()+i); sent_response=true; break;
        }

        if (!draining&&!sent_response&&pipe.dmem_resp.empty()&&dut.lsu.load_response_pending&&rng.one_in(4)) {
            uint32_t stale=dut.lsu.pending_load_transaction_id^0x80000000u;
            if (stale==dut.lsu.pending_load_transaction_id) stale++;
            if (!stale_response_probe(cycle,stale)) return false;
        }

        std::set<uint64_t> before_complete=rob_tokens(dut);
        uint64_t unallocated_dispatch=0;
        if (dut.rename.dispatch_packets[0].valid&&!dut.rename.dispatch_packets[0].rob_allocated)
            unallocated_dispatch=token_of(dut.rename.dispatch_packets[0].uop);
        RefCompletion rc;
        bool did_complete=false;
        if (!sent_response&&pipe.dmem_resp.empty()) {
            BoomCoreState before=dut; rc=ref_complete(before); boom::rob_complete(dut); did_complete=true;
            const char* why=compare_completion(rc,dut);
            if (why&&!check(false,cycle,"completion",why)) return false;
            if (rc.branch) {
                if (!check(dut.brupdate.valid,cycle,"branch","branch completion did not resolve")) return false;
                m.branch_resolves++; if (dut.brupdate.mispredict) m.branch_mispredicts++;
                mark_disappeared(before_complete,false,cycle);
                if (unallocated_dispatch&&!dut.rename.dispatch_packets[0].valid&&tokens[unallocated_dispatch].end==LIVE) {
                    tokens[unallocated_dispatch].end=KILLED; m.killed++; m.branch_kills++;
                }
            } else {
                m.completion_consumed+=rc.consumed;
                if (!rc.consumed&&(before.execute.alu_results[0].valid||before.execute.alu_results[1].valid)) m.held_completion_cycles++;
            }
            for (int l=0;l<2;l++) if (before.execute.alu_results[l].valid&&!dut.execute.alu_results[l].valid) {
                uint64_t t=token_of(before.execute.alu_results[l].uop);
                if (tokens.count(t)&&tokens[t].end==LIVE&&before_complete.count(t)) tokens[t].completions++;
            }
        }

        uint8_t old_ldq=dut.lsu.ldq_count, old_stq=dut.lsu.stq_count;
        bool had_pending=dut.lsu.load_response_pending; uint32_t pending_tx=dut.lsu.pending_load_transaction_id;
        uint8_t pending_idx=dut.lsu.pending_load_rob_idx; uint32_t pending_id=dut.lsu.pending_load_allocation_id;
        uint64_t expected_value=0; uint8_t expected_pdst=0; bool expect_match=false;
        if (sent_response&&had_pending&&pending_idx<ROB_DEPTH&&!pipe.dmem_resp.empty()) {
            const RobEntry& e=dut.rob.entries[pending_idx];
            DmemResponse d=pipe.dmem_resp.read(); pipe.dmem_resp.write(d);
            if (d.transaction_id==pending_tx&&e.valid&&e.uop.queue.rob_allocation_id==pending_id) {
                expected_value=load_value(d.read_data?d.read_data:d.data,e.memory_address,e.memory_size,e.signed_load);
                expected_pdst=e.uop.rename.pdst; expect_match=true;
            }
        }
        boom::lsu_module(dut,pipe);
        if (expect_match) {
            if (!check((!dut.lsu.load_response_pending||dut.lsu.pending_load_transaction_id!=pending_tx)&&
                !dut.rob.entries[pending_idx].busy&&
                (!expected_pdst||dut.int_rf[expected_pdst]==expected_value),cycle,"load-response","matching response not applied")) return false;
            m.load_responses++;
        } else if (sent_response) m.stale_responses++;
        if (dut.lsu.ldq_count<old_ldq) m.ldq_drains+=old_ldq-dut.lsu.ldq_count;

        std::vector<DmemRequest> reqs; drain_dmem(pipe,reqs);
        for (size_t i=0;i<reqs.size();i++) if (!reqs[i].is_store) {
            uint64_t data=0x9e3779b97f4a7c15ULL^(uint64_t)reqs[i].transaction_id*0x100010001ULL;
            int delay=draining?0:(int)rng.range(18);
            responses.push_back(ResponsePlan(reqs[i].transaction_id,data,delay)); m.load_requests++;
        }

        // LSU and commit share the DMEM stream; restore the randomized full condition
        // after observing any LSU request so store commit sees the same backpressure.
        if (dmem_full) fill_dmem(pipe);

        bool commit_candidate=dut.rob.head<ROB_DEPTH&&dut.rob.entries[dut.rob.head].valid&&!dut.rob.entries[dut.rob.head].busy;
        uint8_t commit_head=dut.rob.head; uint64_t instret=dut.csr.instret;
        boom::rob_commit_module(dut,pipe);
        if ((trace_full||(dmem_full&&commit_candidate&&dut.rob.entries[commit_head].is_store))&&commit_candidate) {
            if (!check(dut.rob.entries[commit_head].valid,cycle,"commit-backpressure","blocked commit removed ROB entry")) return false;
            if (!check(dut.csr.instret==instret,cycle,"commit-backpressure","blocked commit incremented instret")) return false;
        }
        std::vector<CommitEntry> commits; drain_trace(pipe,commits);
        drain_dmem(pipe,reqs);
        for (size_t i=0;i<commits.size();i++) account_commit(commits[i],cycle);
        if (dut.lsu.stq_count<old_stq) m.stq_drains+=old_stq-dut.lsu.stq_count;

        generate(cycle,draining); allocate(cycle,draining);
        RenameDispatchPacket packet_before=dut.rename.dispatch_packets[0];
        for (int lane=0;lane<2;lane++) dut.issue.port_ready[lane]=!dut.execute.alu_results[lane].valid&&(draining||rng.bit());
        dut.issue.port_ready[FP_ISSUE_LANE]=false;
        unsigned rm=(dut.issue.port_ready[0]?1:0)|(dut.issue.port_ready[1]?2:0); m.ready_masks[rm]++;
        unsigned cm=(!dut.execute.alu_results[0].valid?1:0)|(!dut.execute.alu_results[1].valid?2:0); m.completion_masks[cm]++;
        BoomCoreState before_issue=dut; RefIssue ri=ref_issue(before_issue); boom::issue_module(dut);
        const char* why=compare_issue(ri,dut); if (why&&!check(false,cycle,"issue",why)) return false;
        m.accepted+=ri.accepted; m.retained+=ri.retained; m.iq_entries+=dut.issue.alu_iq.count;
        if (ri.accepted==2) m.dual_accepts++;
        for (int lane=0;lane<2;lane++) if (ri.grant[lane].accepted) {
            uint64_t t=token_of(ri.grant[lane].uop); TokenRecord& tr=tokens[t]; tr.accepts++;
            if (!check(tr.accepts==1,cycle,"accounting","token accepted more than once")) { m.duplicates++; return false; }
        }
        if (packet_before.valid&&dut.rename.dispatch_packets[0].valid) {
            if (!check(token_of(packet_before.uop)==token_of(dut.rename.dispatch_packets[0].uop)&&
                packet_before.rob_allocated==dut.rename.dispatch_packets[0].rob_allocated&&
                packet_before.uop.queue.rob_allocation_id==dut.rename.dispatch_packets[0].uop.queue.rob_allocation_id,
                cycle,"dispatch-retry","held packet identity changed")) return false;
            m.dispatch_retries++;
        }

        ExecuteState::AluResult expected[2]={before_issue.execute.alu_results[0],before_issue.execute.alu_results[1]};
        for (int lane=0;lane<2;lane++) if (ri.grant[lane].accepted&&!expected[lane].valid) {
            const MicroOp& u=ri.grant[lane].uop;
            if (!(before_issue.brupdate.valid&&before_issue.brupdate.mispredict&&
                (u.branch.br_mask&before_issue.brupdate.mispredict_mask))) expected[lane]=ref_execute_one(before_issue,u);
        }
        boom::execute_module(dut);
        bool held_this_cycle=false;
        for (int lane=0;lane<2;lane++) {
            why=compare_result(expected[lane],dut.execute.alu_results[lane]);
            if (why&&!check(false,cycle,"execute",why)) return false;
            if (before_issue.execute.alu_results[lane].valid) {
                if (!same_result(before_issue.execute.alu_results[lane],dut.execute.alu_results[lane]))
                    if (!check(false,cycle,"execute","held completion was overwritten")) return false;
                held_this_cycle=true;
            }
        }
        if (held_this_cycle) m.held_completion_cycles++;

        if (dut.lsu.ldq_count==LDQ_DEPTH) m.ldq_full_cycles++;
        if (dut.lsu.stq_count==STQ_DEPTH) m.stq_full_cycles++;
        m.dropped+=dut.issue.grants_dropped;
        (void)did_complete;
        return !failed&&check_live_locations(cycle);
    }

    bool finish(int cycle) {
        for (int n=0;n<512;n++) {
            bool any=dut.rename.dispatch_packets[0].valid||dut.issue.alu_iq.count||
                dut.execute.alu_results[0].valid||dut.execute.alu_results[1].valid||
                !rob_tokens(dut).empty()||dut.lsu.ldq_count||dut.lsu.stq_count||
                dut.lsu.load_response_pending||!responses.empty();
            if (!any) break;
            if (!step(cycle+n,true)) return false;
        }
        if (!check(!dut.rename.dispatch_packets[0].valid&&dut.issue.alu_iq.count==0&&
            !dut.execute.alu_results[0].valid&&!dut.execute.alu_results[1].valid&&
            rob_tokens(dut).empty()&&dut.lsu.ldq_count==0&&dut.lsu.stq_count==0,
            cycle,"drain","pipeline did not drain")) return false;
        for (std::map<uint64_t,TokenRecord>::const_iterator it=tokens.begin();it!=tokens.end();++it) {
            const TokenRecord& tr=it->second;
            if (!check(tr.end!=LIVE,cycle,"accounting","token neither consumed nor killed")) { m.dropped++; return false; }
            if (tr.accepts>1||tr.completions>1) { m.duplicates++; return check(false,cycle,"accounting","duplicate token transition"); }
            if (tr.accepts==1&&tr.end==CONSUMED&&tr.completions!=1)
                return check(false,cycle,"accounting","consumed accepted token missing completion");
        }
        return true;
    }
};

void metric(const char* name, uint64_t value) {
    std::printf("METRIC,%s,%llu\n",name,(unsigned long long)value);
}

} // namespace

int main() {
    Metrics m; uint64_t next_token=1;
    for (int si=0;si<kSeeds;si++) {
        uint32_t seed=0x243f6a88u^(0x9e3779b9u*(uint32_t)(si+1));
        Harness h(seed,next_token,m);
        if (!h.stale_completion_probe(-2)||!h.stale_response_probe(-1,0x60000000u^seed)) return 1;
        for (int cycle=0;cycle<kCycles;cycle++) if (!h.step(cycle,false)) return 1;
        if (!h.finish(kCycles)) return 1;
        next_token=h.next_token+1;
    }
    std::printf("W3 persistent dual issue/execute/ROB/LSU random differential: PASS\n");
    metric("random_seeds",m.seeds); metric("cycles_per_seed",kCycles); metric("total_random_cycles",m.random_cycles);
    metric("generated_tokens",m.generated); metric("rob_allocations",m.rob_allocations);
    metric("rob_index_reuses",m.rob_index_reuses); metric("rob_index_wraps",m.rob_index_wraps);
    metric("allocation_id_wraps",m.allocation_id_wraps); metric("dispatch_retries",m.dispatch_retries);
    metric("iq_occupancy_samples",m.iq_entries); metric("accepted_uops",m.accepted);
    metric("dual_accept_cycles",m.dual_accepts); metric("retained_grants",m.retained);
    metric("completion_consumed",m.completion_consumed); metric("held_completion_cycles",m.held_completion_cycles);
    metric("random_loads",m.loads); metric("random_stores",m.stores);
    metric("ldq_full_cycles",m.ldq_full_cycles); metric("stq_full_cycles",m.stq_full_cycles);
    metric("ldq_drains",m.ldq_drains); metric("stq_drains",m.stq_drains);
    metric("load_requests",m.load_requests); metric("load_responses",m.load_responses);
    metric("load_response_delay_cycles",m.response_delay_cycles); metric("stale_responses",m.stale_responses);
    metric("stale_completion_allocation_ids",m.stale_completion_ids);
    metric("branch_resolves",m.branch_resolves); metric("branch_mispredicts",m.branch_mispredicts);
    metric("branch_killed_tokens",m.branch_kills); metric("resets",m.resets); metric("reset_killed_tokens",m.reset_kills);
    metric("trace_backpressure_cycles",m.trace_backpressure); metric("dmem_backpressure_cycles",m.dmem_backpressure);
    metric("committed_tokens",m.commits); metric("consumed_tokens",m.consumed); metric("killed_tokens",m.killed);
    metric("lane_ready_mask_00",m.ready_masks[0]); metric("lane_ready_mask_01",m.ready_masks[1]);
    metric("lane_ready_mask_10",m.ready_masks[2]); metric("lane_ready_mask_11",m.ready_masks[3]);
    metric("completion_free_mask_00",m.completion_masks[0]); metric("completion_free_mask_01",m.completion_masks[1]);
    metric("completion_free_mask_10",m.completion_masks[2]); metric("completion_free_mask_11",m.completion_masks[3]);
    metric("dropped_tokens",m.dropped); metric("duplicate_tokens",m.duplicates);
    return (m.dropped||m.duplicates)?1:0;
}
