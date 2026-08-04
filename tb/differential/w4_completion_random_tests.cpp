#include "completion.hpp"
#include "reset.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <map>
#include <vector>

extern void boom_core_step(BoomCoreState&, PipeSignals&);

namespace boom {
void issue_module(BoomCoreState&);
void lsu_module(BoomCoreState&, PipeSignals&);
void rob_commit_module(BoomCoreState&, PipeSignals&);
}

namespace {

const unsigned kSeeds = 128;
const unsigned kRandomCycles = 128;
const unsigned kMaxDrainCycles = 256;
const unsigned kEligibleWriteArbitrationWaitBound =
    (COMPLETION_PENDING_SLOTS + NUM_INT_WRITEBACK_PORTS - 1) /
    NUM_INT_WRITEBACK_PORTS - 1;

struct Rng {
    uint32_t state;
    explicit Rng(uint32_t seed) : state(seed ? seed : 1) {}
    uint32_t next() { uint32_t x=state; x^=x<<13; x^=x>>17; x^=x<<5; return state=x; }
    uint32_t range(uint32_t n) { return next()%n; }
    bool one_in(uint32_t n) { return range(n)==0; }
    bool bit() { return (next()&1)!=0; }
};

enum TokenTerminal { TOKEN_LIVE, TOKEN_COMMITTED, TOKEN_KILLED, TOKEN_FAULTED };
enum EventTerminal { EVENT_PENDING, EVENT_ACCEPTED, EVENT_REJECTED, EVENT_KILLED, EVENT_FAULTED };
enum TokenKind { TK_INT, TK_MEM, TK_LOAD, TK_STORE, TK_BRANCH, TK_EXCEPTION };

struct Token {
    uint64_t id;
    TokenKind kind;
    TokenTerminal terminal;
    uint8_t rob, pdst;
    uint32_t allocation;
    bool ever_accepted;
    unsigned source_events, prf_writes, rob_completes, commits;
    int first_offer, first_accept, writeback_cycle, rob_complete_cycle, commit_cycle;
    Token() : id(0), kind(TK_INT), terminal(TOKEN_LIVE), rob(0), pdst(0),
        allocation(0), ever_accepted(false), source_events(0), prf_writes(0),
        rob_completes(0), commits(0), first_offer(-1), first_accept(-1),
        writeback_cycle(-1), rob_complete_cycle(-1), commit_cycle(-1) {}
};

struct SourceEvent {
    uint64_t id, token, value;
    EventTerminal terminal;
    CompletionKind kind;
    CompletionSourceId source;
    uint8_t rob, pdst, branch_mask;
    uint32_t allocation, transaction;
    bool writes, exception, branch, mispredict, control_resolved;
    bool is_load, is_store, signed_load;
    uint64_t cause, address, store_data, redirect_pc;
    uint8_t memory_mask, memory_size;
    unsigned offer_cycle, eligible_wait, fence_blocked_cycles;
    bool wakeup_sent;
    SourceEvent() : id(0), token(0), value(0), terminal(EVENT_PENDING),
        kind(COMPLETION_NONE), source(COMPLETION_SOURCE_LSU_LOAD), rob(0), pdst(0),
        branch_mask(0), allocation(0), transaction(0), writes(false), exception(false),
        branch(false), mispredict(false), control_resolved(false), is_load(false),
        is_store(false), signed_load(false), cause(0), address(0), store_data(0),
        redirect_pc(0), memory_mask(0), memory_size(0), offer_cycle(0), eligible_wait(0),
        fence_blocked_cycles(0), wakeup_sent(false) {}
};

struct Owner {
    bool valid, busy, exception, exception_reported;
    uint64_t token;
    uint32_t allocation;
    uint8_t pdst, branch_mask;
    bool writes, is_load, is_store, signed_load;
    bool memory_valid, memory_request_sent, memory_completed;
    uint64_t address, memory_data, cause;
    uint8_t memory_mask, memory_size;
    uint32_t transaction;
    Owner() : valid(false), busy(false), exception(false), exception_reported(false),
        token(0), allocation(0), pdst(0), branch_mask(0), writes(false), is_load(false),
        is_store(false), signed_load(false), memory_valid(false),
        memory_request_sent(false), memory_completed(false), address(0), memory_data(0),
        cause(0), memory_mask(0), memory_size(0), transaction(0) {}
};

struct LoadPlan {
    uint64_t token, data;
    uint8_t rob;
    uint32_t allocation, transaction;
    int delay;
    bool reset_seen;
    LoadPlan() : token(0), data(0), rob(0), allocation(0), transaction(0), delay(0),
        reset_seen(false) {}
};

struct QueueOwner {
    uint8_t rob, branch_mask;
    uint32_t allocation;
    QueueOwner(uint8_t r,uint32_t a,uint8_t m) : rob(r), branch_mask(m), allocation(a) {}
};

struct Publication {
    uint64_t event, token, value;
    uint8_t pdst, rob, branch_mask;
    uint32_t allocation;
    CompletionSourceId source;
};

struct ExpectedCycle {
    std::vector<Publication> writebacks, wakeups;
    std::vector<uint64_t> rob_completes, accepted_events, killed_events, faulted_events;
    bool branch_valid, branch_mispredict, validation_fault;
    uint8_t branch_rob, branch_tag, resolve_mask, mispredict_mask, branch_uop_mask;
    uint32_t branch_allocation;
    uint64_t branch_token, branch_target;
    bool branch_taken;
    bool commit_valid, commit_exception, store_request;
    uint8_t store_rob;
    uint32_t commit_inst;
    uint8_t commit_rd, commit_priv;
    bool commit_rd_valid, commit_memory_valid, commit_is_store;
    uint64_t commit_token, commit_value, commit_cause, commit_memory_address;
    uint64_t commit_memory_data, commit_store_address, commit_store_data;
    uint8_t commit_memory_mask, commit_store_mask;
    uint32_t store_transaction;
    uint64_t store_address, store_data;
    uint8_t store_size, store_mask, store_branch_mask;
    ExpectedCycle() : branch_valid(false), branch_mispredict(false),
        validation_fault(false), branch_rob(0), branch_tag(0), resolve_mask(0),
        mispredict_mask(0), branch_uop_mask(0), branch_allocation(0), branch_token(0),
        branch_target(0), branch_taken(false), commit_valid(false), commit_exception(false),
        store_request(false), store_rob(0), commit_inst(0), commit_rd(0), commit_priv(0),
        commit_rd_valid(false), commit_memory_valid(false), commit_is_store(false),
        commit_token(0), commit_value(0), commit_cause(0), commit_memory_address(0),
        commit_memory_data(0), commit_store_address(0), commit_store_data(0),
        commit_memory_mask(0), commit_store_mask(0), store_transaction(0), store_address(0),
        store_data(0), store_size(0), store_mask(0), store_branch_mask(0) {}
};

struct Metrics {
    uint64_t seeds, random_cycles, drain_cycles, seed_passes;
    uint64_t tokens_offered, tokens_committed, tokens_killed, tokens_faulted;
    uint64_t accepted_tokens, accepted_terminal_committed, accepted_then_killed;
    uint64_t accepted_terminal_faulted, accepted_conservation_lhs, accepted_conservation_rhs;
    uint64_t events_offered, events_accepted, events_rejected, events_killed, events_faulted;
    uint64_t int_events, mem_events, mem_agus, stores, load_responses;
    uint64_t stale_completions, stale_responses, delayed_responses, post_reset_stale_responses;
    uint64_t branch_resolves, branch_mispredicts, exception_events, validation_faults;
    uint64_t rob_index_wraps, rob_index_reuses, allocation_id_wraps;
    uint64_t resets, reset_with_inflight, reset_kills, branch_kills, fence_blocked_cycles;
    uint64_t prf_writes, rob_completes, commits, wakeup_publications, bypass_publications;
    uint64_t repeated_wakeup_publications, same_pdst_same, same_pdst_different;
    uint64_t source_combination_mask[8], source_age_permutations, sustained_arrival_cycles;
    uint64_t pending_pressure_cycles, trace_backpressure_cycles, dmem_backpressure_cycles;
    uint64_t ldq_capacity_blocked_cycles, stq_capacity_blocked_cycles;
    uint64_t iq_prs1_checks, iq_prs2_checks, iq_prs3_checks, prf_checks, busy_checks;
    uint64_t exact_writeback_checks, exact_wakeup_checks, exact_rob_complete_checks;
    uint64_t full_branch_checks, fault_metadata_checks, commit_payload_checks;
    uint64_t lsu_ownership_checks, csr_counter_checks;
    uint64_t full_load_request_checks;
    uint64_t dropped_tokens, duplicate_tokens, stale_side_effects, unexplained_tokens;
    uint64_t stale_snapshot_checks, terminal_duplicate_classifications, terminal_missing_records;
    uint64_t scanned_event_pending, scanned_token_pending, scanned_accepted_pending;
    uint64_t scanned_tokens_offered, scanned_tokens_committed, scanned_tokens_killed;
    uint64_t scanned_tokens_faulted, scanned_accepted_tokens, scanned_accepted_committed;
    uint64_t scanned_accepted_killed, scanned_accepted_faulted;
    uint64_t scanned_events_offered, scanned_events_accepted, scanned_events_rejected;
    uint64_t scanned_events_killed, scanned_events_faulted;
    uint64_t source_conservation_lhs, source_conservation_rhs;
    uint64_t token_conservation_lhs, token_conservation_rhs;
    uint64_t offer_to_accept_samples, offer_to_accept_sum, rob_to_commit_samples, rob_to_commit_sum;
    unsigned max_offer_to_accept, max_rob_to_commit, max_eligible_write_arbitration_wait;
    unsigned peak_sources, peak_writes, peak_wakeups, peak_bypass;
};

struct Model {
    Owner rob[ROB_DEPTH];
    SourceEvent slots[COMPLETION_PENDING_SLOTS];
    bool slot_valid[COMPLETION_PENDING_SLOTS];
    uint64_t prf[INT_PHYS_REGS];
    bool busy[INT_PHYS_REGS];
    uint8_t head, tail;
    bool maybe_full, branch_active, fault_active, conflict_active;
    uint8_t fault_pdst, fault_rob;
    uint32_t fault_allocation;
    uint64_t fault_cause, csr_instret;
    RobState rob_state;
    uint32_t next_allocation, next_transaction;
    bool load_pending;
    uint8_t pending_load_rob;
    uint32_t pending_load_allocation, pending_load_transaction;
    std::map<uint64_t,Token> tokens;
    std::map<uint64_t,SourceEvent> event_history;
    std::map<uint64_t,Token> token_origins;
    std::map<uint64_t,SourceEvent> event_origins;
    std::map<uint64_t,std::vector<TokenTerminal> > token_terminal_evidence;
    std::map<uint64_t,std::vector<EventTerminal> > event_terminal_evidence;
    std::vector<QueueOwner> ldq, stq;
    uint32_t last_allocation[ROB_DEPTH];

    Model() : head(0), tail(0), maybe_full(false), branch_active(false),
        fault_active(false), conflict_active(false), fault_pdst(0), fault_rob(0), fault_allocation(0),
        fault_cause(0), csr_instret(0), rob_state(ROB_NORMAL),
        next_allocation(1), next_transaction(1), load_pending(false),
        pending_load_rob(0), pending_load_allocation(0), pending_load_transaction(0) {
        for (int i=0;i<ROB_DEPTH;i++) rob[i]=Owner();
        for (int i=0;i<ROB_DEPTH;i++) last_allocation[i]=0;
        for (int i=0;i<COMPLETION_PENDING_SLOTS;i++) slot_valid[i]=false;
        for (int p=0;p<INT_PHYS_REGS;p++) { prf[p]=0; busy[p]=false; }
    }

    unsigned age(uint8_t idx) const { return (idx+ROB_DEPTH-head)%ROB_DEPTH; }
    bool empty() const { return head==tail&&!maybe_full; }
    bool full() const { return head==tail&&maybe_full; }
    unsigned owner_count() const {
        unsigned n=0; for (int i=0;i<ROB_DEPTH;i++) if (rob[i].valid) n++; return n;
    }
    unsigned source_mask() const {
        unsigned m=0; for (int i=0;i<COMPLETION_PENDING_SLOTS;i++) if (slot_valid[i]) m|=1u<<i;
        return m;
    }
    bool owns(const SourceEvent& e) const {
        return e.rob<ROB_DEPTH&&rob[e.rob].valid&&rob[e.rob].allocation==e.allocation;
    }
    bool event_valid(const SourceEvent& e) const {
        if (!owns(e)||!rob[e.rob].busy) return false;
        const Owner& o=rob[e.rob];
        if (e.kind==COMPLETION_MEMORY_ADDRESS) return !o.memory_valid&&!o.memory_completed;
        if (e.kind==COMPLETION_STORE) return !o.memory_completed;
        if (e.kind==COMPLETION_LOAD_RESPONSE)
            return o.is_load&&o.memory_request_sent&&!o.memory_completed&&o.transaction==e.transaction;
        return true;
    }
    bool writes_integer(const SourceEvent& e) const {
        bool complete=e.kind!=COMPLETION_MEMORY_ADDRESS||!e.is_load;
        return complete&&e.writes&&!(e.kind==COMPLETION_LOAD_RESPONSE&&e.exception);
    }
    bool forwardable(const SourceEvent& e) const {
        return event_valid(e)&&e.writes&&!e.exception&&e.pdst!=0&&e.pdst<INT_PHYS_REGS;
    }
    void set_event_terminal(SourceEvent& e, EventTerminal terminal) {
        e.terminal=terminal; event_history[e.id]=e;
        event_terminal_evidence[e.id].push_back(terminal);
    }
    bool set_token_terminal(uint64_t id,TokenTerminal terminal) {
        Token& token=tokens[id];
        if (token.terminal!=TOKEN_LIVE) return false;
        token.terminal=terminal; token_terminal_evidence[id].push_back(terminal); return true;
    }
    void kill_owner(uint8_t idx, Metrics& m, bool reset_kill) {
        if (!rob[idx].valid) return;
        Token& t=tokens[rob[idx].token];
        if (t.terminal==TOKEN_LIVE) {
            set_token_terminal(t.id,TOKEN_KILLED); m.tokens_killed++;
            if (t.ever_accepted) m.accepted_then_killed++;
            if (reset_kill) m.reset_kills++; else m.branch_kills++;
        }
        rob[idx]=Owner();
    }
    void rebuild_busy() {
        for (int p=0;p<INT_PHYS_REGS;p++) busy[p]=false;
        for (int i=0;i<ROB_DEPTH;i++) if (rob[i].valid&&rob[i].busy&&rob[i].pdst)
            busy[rob[i].pdst]=true;
    }
};

struct Harness {
    uint32_t seed;
    unsigned seed_index, cycle;
    Rng rng;
    BoomCoreState dut;
    PipeSignals pipe;
    Model model;
    Metrics& metrics;
    std::deque<LoadPlan> responses;
    uint64_t next_token, next_event;
    unsigned seed_resets;
    bool trace_block, dmem_block;
    bool stale_probe_active;
    uint64_t stale_before_signature;
    SourceEvent debug_slots[3];
    bool debug_valid[3];

    Harness(unsigned index, uint32_t value, uint64_t token_base, uint64_t event_base,
            Metrics& global) : seed(value), seed_index(index), cycle(0), rng(value),
        metrics(global), next_token(token_base), next_event(event_base), seed_resets(0),
        trace_block(false), dmem_block(false), stale_probe_active(false),
        stale_before_signature(0) {
        model.next_allocation=(index&1)?0xfffffff0u:(1u+rng.range(0x100000));
        model.next_transaction=1u+rng.range(0x10000);
        dut.rob.state=ROB_NORMAL;
        dut.rob.next_allocation_id=model.next_allocation;
        dut.lsu.next_transaction_id=model.next_transaction;
    }

    bool fail(const char* phase, const char* reason) {
        std::printf("FIRST_MISMATCH seed_index=%u seed=0x%08x cycle=%u phase=%s reason=%s\n",
                    seed_index,seed,cycle,phase,reason);
        std::printf("STATE model_rob=%u/%u/%d dut_rob=%u/%u/%d sources=%u dut_sources=%u%u%u "
                    "fault=%d/%d load=%d:%u/%d:%u\n",model.head,model.tail,model.maybe_full,
                    dut.rob.head,dut.rob.tail,dut.rob.maybe_full,model.source_mask(),
                    dut.completion.load_response.valid,dut.completion.mem_execute.valid,
                    dut.completion.int_execute.valid,model.fault_active,dut.completion.writeback_fault_valid,
                    model.load_pending,model.pending_load_transaction,dut.lsu.load_response_pending,
                    dut.lsu.pending_load_transaction_id);
        for (int i=0;i<3;i++) if (debug_valid[i])
            std::printf("  PRE_SOURCE index=%d id=%llu token=%llu rob=%u alloc=%u kind=%u "
                        "write=%d branch=%d exception=%d resolved=%d pdst=%u value=%llu\n",i,
                        (unsigned long long)debug_slots[i].id,
                        (unsigned long long)debug_slots[i].token,debug_slots[i].rob,
                        debug_slots[i].allocation,(unsigned)debug_slots[i].kind,
                        debug_slots[i].writes,debug_slots[i].branch,debug_slots[i].exception,
                        debug_slots[i].control_resolved,debug_slots[i].pdst,
                        (unsigned long long)debug_slots[i].value);
        return false;
    }
    bool check(bool condition,const char* phase,const char* reason) {
        return condition?true:fail(phase,reason);
    }

    static void hash_value(uint64_t& hash,uint64_t value) {
        hash^=value+0x9e3779b97f4a7c15ULL+(hash<<6)+(hash>>2);
    }
    uint64_t recovery_signature() const {
        uint64_t hash=0xcbf29ce484222325ULL;
        hash_value(hash,dut.rob.head); hash_value(hash,dut.rob.tail);
        hash_value(hash,dut.rob.maybe_full); hash_value(hash,dut.rob.state);
        hash_value(hash,dut.csr.instret); hash_value(hash,dut.branch_state.active_mask);
        hash_value(hash,dut.completion.writeback_conflict);
        hash_value(hash,dut.completion.writeback_fault_valid);
        hash_value(hash,dut.completion.writeback_fault_pdst);
        hash_value(hash,dut.completion.writeback_fault_rob_idx);
        hash_value(hash,dut.completion.writeback_fault_allocation_id);
        hash_value(hash,dut.completion.writeback_fault_cause);
        hash_value(hash,dut.lsu.load_response_pending);
        hash_value(hash,dut.lsu.pending_load_transaction_id);
        hash_value(hash,dut.lsu.pending_load_rob_idx);
        hash_value(hash,dut.lsu.pending_load_allocation_id);
        hash_value(hash,dut.lsu.ldq_count); hash_value(hash,dut.lsu.stq_count);
        for (int i=0;i<ROB_DEPTH;i++) {
            const RobEntry& e=dut.rob.entries[i]; hash_value(hash,e.valid); hash_value(hash,e.busy);
            hash_value(hash,e.exception); hash_value(hash,e.uop.queue.rob_idx);
            hash_value(hash,e.uop.queue.rob_allocation_id); hash_value(hash,e.uop.exc_cause);
            hash_value(hash,e.memory_valid); hash_value(hash,e.memory_request_sent);
            hash_value(hash,e.memory_completed); hash_value(hash,e.memory_transaction_id);
        }
        for (int p=0;p<INT_PHYS_REGS;p++) {
            hash_value(hash,boom::prf_read(dut,p));
            hash_value(hash,dut.rename.int_free_list.busy_table[p]);
        }
        hash_value(hash,dut.completion.load_response.valid);
        hash_value(hash,dut.completion.mem_execute.valid);
        hash_value(hash,dut.completion.int_execute.valid);
        return hash;
    }
    void begin_stale_snapshot() {
        stale_probe_active=true; stale_before_signature=recovery_signature();
    }
    bool check_stale_snapshot() {
        if (!stale_probe_active) return true;
        metrics.stale_snapshot_checks++;
        bool same=stale_before_signature==recovery_signature();
        stale_probe_active=false;
        if (!same) { metrics.stale_side_effects++; return fail("stale-snapshot","architectural/recovery side effect"); }
        return true;
    }

    MicroOp uop_for(const Owner& o,uint8_t idx) const {
        MicroOp u; u.uopc=o.is_load?39:(o.is_store?46:1); u.debug_pc=o.token; u.inst=(uint32_t)o.token;
        u.queue.rob_idx=idx; u.queue.rob_allocation_id=o.allocation;
        u.rename.pdst=o.pdst; u.rename.ldst=(uint8_t)(o.pdst%LOGICAL_REG_COUNT);
        u.rename.dst_rtype=o.writes?DST_INT:DST_N; u.branch.br_mask=o.branch_mask;
        u.iq_type=(o.is_load||o.is_store)?IQ_MEM:IQ_ALU;
        u.fu_code=(o.is_load||o.is_store)?FU_MEM:FU_ALU;
        u.ctrl.is_load=o.is_load; u.ctrl.is_sta=o.is_store;
        return u;
    }

    void install_owner_dut(uint8_t idx,const Owner& o) {
        RobEntry& d=dut.rob.entries[idx]; d=RobEntry(); d.valid=o.valid; d.busy=o.busy;
        d.exception=o.exception; d.exception_reported=o.exception_reported; d.uop=uop_for(o,idx);
        d.memory_valid=o.memory_valid; d.is_load=o.is_load; d.is_store=o.is_store;
        d.signed_load=o.signed_load; d.memory_request_sent=o.memory_request_sent;
        d.memory_completed=o.memory_completed; d.memory_address=o.address;
        d.memory_data=o.memory_data; d.memory_mask=o.memory_mask; d.memory_size=o.memory_size;
        d.memory_transaction_id=o.transaction;
    }

    bool allocate(TokenKind kind,uint8_t source,uint8_t pdst,uint64_t value,
                  bool same_value,bool different_value) {
        if (model.full()||model.slot_valid[source]||model.rob[model.tail].valid) return false;
        uint8_t idx=model.tail;
        Owner o; o.valid=o.busy=true; o.token=next_token++; o.allocation=model.next_allocation++;
        if (o.allocation==0) { o.allocation=model.next_allocation++; metrics.allocation_id_wraps++; }
        if (model.last_allocation[idx]) metrics.rob_index_reuses++;
        model.last_allocation[idx]=o.allocation;
        o.pdst=pdst; o.branch_mask=model.branch_active?1:0;
        o.writes=kind==TK_INT||kind==TK_MEM||kind==TK_LOAD;
        o.is_load=kind==TK_LOAD; o.is_store=kind==TK_STORE;
        if (kind==TK_BRANCH) o.writes=false;
        if (kind==TK_EXCEPTION) o.writes=false;
        model.rob[idx]=o; model.tail=(uint8_t)((model.tail+1)%ROB_DEPTH);
        if (model.tail==0) metrics.rob_index_wraps++;
        if (model.tail==model.head) model.maybe_full=true;
        Token t; t.id=o.token; t.kind=kind; t.rob=idx; t.pdst=pdst; t.allocation=o.allocation;
        t.source_events=1; t.first_offer=(int)cycle; model.tokens[t.id]=t;
        model.token_origins[t.id]=t;
        SourceEvent e; e.id=next_event++; e.token=t.id; e.source=(CompletionSourceId)source;
        e.rob=idx; e.pdst=pdst; e.allocation=o.allocation; e.value=value;
        e.branch_mask=o.branch_mask; e.offer_cycle=cycle; e.writes=o.writes;
        e.kind=COMPLETION_EXECUTE;
        if (kind==TK_LOAD) { e.kind=COMPLETION_MEMORY_ADDRESS; e.is_load=true; e.writes=false;
            e.address=0x1000+(rng.range(64)*8); e.memory_mask=0xff; e.memory_size=3;
            e.signed_load=rng.bit(); }
        if (kind==TK_STORE) { e.kind=COMPLETION_STORE; e.is_store=true; e.writes=false;
            e.address=0x2000+(rng.range(64)*8); e.store_data=value; e.memory_mask=0xff; e.memory_size=3; }
        if (kind==TK_BRANCH) { e.kind=COMPLETION_BRANCH; e.branch=true; e.mispredict=rng.bit();
            e.redirect_pc=(e.token<<2)^0x80000000ULL;
            model.branch_active=true; }
        if (kind==TK_EXCEPTION) { e.exception=true; e.cause=13+rng.range(3); }
        if (same_value||different_value) (void)0;
        model.slots[source]=e; model.slot_valid[source]=true;
        model.event_history[e.id]=e;
        model.event_origins[e.id]=e;
        install_owner_dut(idx,o);
        dut.rob.head=model.head; dut.rob.tail=model.tail; dut.rob.maybe_full=model.maybe_full;
        if (o.pdst) { model.busy[o.pdst]=true; dut.rename.int_free_list.busy_table[o.pdst]=true; }
        install_slot_dut(source,e);
        metrics.tokens_offered++; metrics.events_offered++;
        if (source==COMPLETION_SOURCE_INT_EXECUTE) metrics.int_events++; else metrics.mem_events++;
        if (kind==TK_LOAD) metrics.mem_agus++;
        if (kind==TK_STORE) metrics.stores++;
        if (kind==TK_EXCEPTION) metrics.exception_events++;
        return true;
    }

    void install_slot_dut(int source,const SourceEvent& e) {
        CompletionEvent d; d.valid=true; d.kind=e.kind; d.source=e.source;
        d.uop=uop_for(model.rob[e.rob],e.rob); d.writes_prf=e.writes; d.value=e.value;
        d.exception=e.exception; d.exc_cause=e.cause; d.mispredict=e.mispredict;
        d.redirect_pc=e.redirect_pc;
        d.control_resolved=e.control_resolved; d.memory_valid=e.is_load||e.is_store;
        d.is_load=e.is_load; d.is_store=e.is_store; d.signed_load=e.signed_load;
        d.memory_address=e.address; d.store_data=e.store_data; d.memory_mask=e.memory_mask;
        d.memory_size=e.memory_size; d.transaction_id=e.transaction;
        if (e.branch) { d.uop.branch.is_br=true; d.uop.branch.br_tag=0; }
        if (source==COMPLETION_SOURCE_LSU_LOAD) dut.completion.load_response=d;
        else if (source==COMPLETION_SOURCE_MEM_EXECUTE) dut.completion.mem_execute=d;
        else dut.completion.int_execute=d;
    }

    void offer_stale_completion() {
        if (model.slot_valid[COMPLETION_SOURCE_INT_EXECUTE]) return;
        begin_stale_snapshot();
        SourceEvent e; e.id=next_event++; e.source=COMPLETION_SOURCE_INT_EXECUTE;
        e.rob=(uint8_t)rng.range(ROB_DEPTH); e.allocation=rng.next()|1; e.pdst=(uint8_t)(3+rng.range(48));
        e.value=rng.next(); e.kind=COMPLETION_EXECUTE; e.writes=true; e.offer_cycle=cycle;
        if (model.rob[e.rob].valid&&model.rob[e.rob].allocation==e.allocation) e.allocation^=0x80000000u;
        model.slots[2]=e; model.slot_valid[2]=true; model.event_history[e.id]=e;
        model.event_origins[e.id]=e;
        CompletionEvent d; d.valid=true; d.kind=e.kind; d.source=e.source; d.writes_prf=true;
        d.value=e.value; d.uop.queue.rob_idx=e.rob; d.uop.queue.rob_allocation_id=e.allocation;
        d.uop.rename.pdst=e.pdst; d.uop.rename.dst_rtype=DST_INT;
        dut.completion.int_execute=d;
        metrics.events_offered++; metrics.stale_completions++;
    }

    void generate_arrivals(bool draining) {
        if (draining) return;
        if (model.source_mask()==0&&rng.one_in(19)) { offer_stale_completion(); return; }
        bool mem_free=!model.slot_valid[COMPLETION_SOURCE_MEM_EXECUTE];
        bool int_free=!model.slot_valid[COMPLETION_SOURCE_INT_EXECUTE];
        bool had_held=model.source_mask()!=0;
        bool both=mem_free&&int_free&&!model.full()&&(cycle%7==seed_index%7||rng.one_in(3));
        bool conflict=both&&(cycle%13==seed_index%13);
        bool same=conflict&&((cycle/13+seed_index)&1)==0;
        uint8_t shared=(uint8_t)(3+rng.range(48));
        uint64_t shared_value=((uint64_t)rng.next()<<32)|rng.next();
        unsigned int_selector=(cycle*3+seed_index)%17;
        bool precise_first=both&&((!model.branch_active&&int_selector==4)||int_selector==9);
        if (precise_first) {
            TokenKind int_kind=int_selector==4?TK_BRANCH:TK_EXCEPTION;
            allocate(int_kind,COMPLETION_SOURCE_INT_EXECUTE,
                     (uint8_t)(3+rng.range(48)),shared_value,false,false);
            allocate(TK_MEM,COMPLETION_SOURCE_MEM_EXECUTE,
                     (uint8_t)(3+rng.range(48)),shared_value^rng.next(),false,false);
            if (had_held) metrics.sustained_arrival_cycles++;
            return;
        }
        if (mem_free&&!model.full()&&(both||rng.range(4)!=0)) {
            TokenKind kind;
            unsigned selector=(cycle+seed_index)%11;
            if (selector==2||selector==7) kind=TK_LOAD;
            else if (selector==5) kind=TK_STORE;
            else kind=TK_MEM;
            uint8_t pdst=conflict?shared:(uint8_t)(3+rng.range(48));
            allocate(kind,COMPLETION_SOURCE_MEM_EXECUTE,pdst,shared_value,false,false);
        }
        if (int_free&&!model.full()&&(both||rng.range(4)!=0)) {
            TokenKind kind=TK_INT;
            unsigned selector=int_selector;
            if (!model.branch_active&&selector==4) kind=TK_BRANCH;
            else if (selector==9) kind=TK_EXCEPTION;
            uint8_t pdst=conflict?shared:(uint8_t)(3+rng.range(48));
            uint64_t value=conflict?(same?shared_value:shared_value^0x55aa55aa55aa55aaULL):
                (((uint64_t)rng.next()<<32)|rng.next());
            allocate(kind,COMPLETION_SOURCE_INT_EXECUTE,pdst,value,same,!same&&conflict);
            if (conflict) {
                if (same) metrics.same_pdst_same++; else metrics.same_pdst_different++;
            }
        }
        if (had_held&&model.source_mask()!=0) metrics.sustained_arrival_cycles++;
    }

    void offer_response(const LoadPlan& plan,bool deliberately_stale) {
        SourceEvent e; e.id=next_event++; e.source=COMPLETION_SOURCE_LSU_LOAD;
        e.kind=COMPLETION_LOAD_RESPONSE; e.transaction=deliberately_stale?
            (plan.transaction^0x80000000u):plan.transaction; e.value=plan.data;
        e.rob=plan.rob; e.allocation=plan.allocation; e.token=plan.token; e.offer_cycle=cycle;
        e.pdst=model.rob[e.rob].valid?model.rob[e.rob].pdst:0; e.writes=true; e.is_load=true;
        if (model.rob[e.rob].valid) { const Owner& owner=model.rob[e.rob];
            e.branch_mask=owner.branch_mask; e.signed_load=owner.signed_load;
            e.address=owner.address; e.memory_mask=owner.memory_mask; e.memory_size=owner.memory_size;
        }
        metrics.events_offered++; metrics.load_responses++;
        model.event_origins[e.id]=e;
        bool valid=model.load_pending&&e.transaction==model.pending_load_transaction&&
            e.rob<ROB_DEPTH&&model.rob[e.rob].valid&&model.rob[e.rob].busy&&
            model.rob[e.rob].is_load&&model.rob[e.rob].memory_request_sent&&
            model.rob[e.rob].allocation==e.allocation&&model.rob[e.rob].transaction==e.transaction;
        if (!valid) begin_stale_snapshot();
        DmemResponse d; d.transaction_id=e.transaction; d.data=d.read_data=e.value;
        pipe.dmem_resp.write(d);
        if (valid&&!model.slot_valid[0]) {
            model.slots[0]=e; model.slot_valid[0]=true; model.event_history[e.id]=e;
            model.tokens[e.token].source_events++;
        } else {
            model.event_history[e.id]=e; model.set_event_terminal(e,EVENT_REJECTED);
            metrics.events_rejected++; metrics.stale_responses++;
            if (plan.reset_seen) metrics.post_reset_stale_responses++;
        }
    }

    bool maybe_response(bool draining) {
        for (size_t i=0;i<responses.size();i++) if (responses[i].delay>0) {
            responses[i].delay--; metrics.delayed_responses++;
        }
        if (model.slot_valid[0]||responses.empty()) return false;
        size_t chosen=responses.size();
        for (size_t i=0;i<responses.size();i++) if (responses[i].delay<=0) { chosen=i; break; }
        if (chosen==responses.size()) return false;
        const LoadPlan& candidate=responses[chosen];
        bool candidate_valid=model.load_pending&&candidate.transaction==model.pending_load_transaction&&
            candidate.rob<ROB_DEPTH&&model.rob[candidate.rob].valid&&
            model.rob[candidate.rob].allocation==candidate.allocation;
        if (!candidate_valid&&model.source_mask()!=0) return false;
        if (!draining&&model.source_mask()==0&&rng.one_in(3)) {
            offer_response(responses[chosen],true);
            responses[chosen].delay=1+(int)rng.range(4);
            return true;
        }
        LoadPlan plan=responses[chosen]; responses.erase(responses.begin()+chosen);
        offer_response(plan,false); return true;
    }

    bool precise_fence(unsigned& fence_age) const {
        bool valid=false; fence_age=ROB_DEPTH;
        for (int i=0;i<3;i++) if (model.slot_valid[i]&&model.event_valid(model.slots[i])&&
            model.slots[i].exception) { unsigned a=model.age(model.slots[i].rob);
            if (!valid||a<fence_age) { valid=true; fence_age=a; } }
        for (int i=0;i<ROB_DEPTH;i++) if (model.rob[i].valid&&model.rob[i].exception) {
            unsigned a=model.age((uint8_t)i); if (!valid||a<fence_age) { valid=true; fence_age=a; }
        }
        return valid;
    }

    void resolve_branch(ExpectedCycle& expected) {
        int selected=-1; unsigned selected_age=ROB_DEPTH;
        for (int i=0;i<3;i++) if (model.slot_valid[i]&&model.event_valid(model.slots[i])&&
            model.slots[i].branch&&!model.slots[i].control_resolved) {
            unsigned a=model.age(model.slots[i].rob);
            if (selected<0||a<selected_age||(a==selected_age&&i<selected)) {
                selected=i; selected_age=a;
            }
        }
        if (selected<0) return;
        SourceEvent& branch=model.slots[selected]; expected.branch_valid=true;
        expected.branch_mispredict=branch.mispredict; expected.branch_rob=branch.rob;
        expected.branch_tag=0; expected.resolve_mask=1;
        expected.mispredict_mask=branch.mispredict?1:0;
        expected.branch_uop_mask=branch.branch_mask;
        expected.branch_allocation=branch.allocation; expected.branch_token=branch.token;
        expected.branch_target=branch.redirect_pc; expected.branch_taken=branch.mispredict;
        metrics.branch_resolves++; if (branch.mispredict) metrics.branch_mispredicts++;
        if (branch.mispredict) {
            for (int i=0;i<3;i++) if (model.slot_valid[i]&&i!=selected&&
                model.age(model.slots[i].rob)>selected_age) {
                expected.killed_events.push_back(model.slots[i].id);
                model.set_event_terminal(model.slots[i],EVENT_KILLED); model.slot_valid[i]=false;
            }
            for (int idx=0;idx<ROB_DEPTH;idx++)
                if (model.rob[idx].valid&&model.age((uint8_t)idx)>selected_age)
                    model.kill_owner((uint8_t)idx,metrics,false);
            model.tail=(uint8_t)((branch.rob+1)%ROB_DEPTH); model.maybe_full=false;
            model.rebuild_busy();
            bool pending_killed=false;
            for (size_t i=0;i<model.ldq.size();i++)
                if ((model.ldq[i].branch_mask&1)&&model.ldq[i].rob==model.pending_load_rob&&
                    model.ldq[i].allocation==model.pending_load_allocation) pending_killed=true;
            model.ldq.erase(std::remove_if(model.ldq.begin(),model.ldq.end(),
                [](const QueueOwner& q){return (q.branch_mask&1)!=0;}),model.ldq.end());
            model.stq.erase(std::remove_if(model.stq.begin(),model.stq.end(),
                [](const QueueOwner& q){return (q.branch_mask&1)!=0;}),model.stq.end());
            if (pending_killed) { model.load_pending=false; model.pending_load_transaction=0;
                model.pending_load_allocation=0; model.pending_load_rob=0; }
        }
        for (int i=0;i<ROB_DEPTH;i++) if (model.rob[i].valid) model.rob[i].branch_mask&=(uint8_t)~1;
        for (int i=0;i<3;i++) if (model.slot_valid[i]) model.slots[i].branch_mask&=(uint8_t)~1;
        for (size_t i=0;i<model.ldq.size();i++) model.ldq[i].branch_mask&=(uint8_t)~1;
        for (size_t i=0;i<model.stq.size();i++) model.stq[i].branch_mask&=(uint8_t)~1;
        model.branch_active=false; branch.control_resolved=true;
    }

    bool detect_conflict(ExpectedCycle& expected) {
        int first=-1; uint8_t pdst=0;
        for (int i=0;i<3;i++) if (model.slot_valid[i]&&model.forwardable(model.slots[i]))
            for (int j=i+1;j<3;j++) if (model.slot_valid[j]&&model.forwardable(model.slots[j])&&
                model.slots[i].pdst==model.slots[j].pdst&&model.slots[i].value!=model.slots[j].value) {
                first=i; pdst=model.slots[i].pdst;
            }
        if (first<0) return false;
        expected.validation_fault=true; model.fault_active=true; model.conflict_active=true;
        metrics.validation_faults++;
        int fault_source=-1; unsigned fault_age=ROB_DEPTH;
        for (int i=0;i<3;i++) if (model.slot_valid[i]&&model.event_valid(model.slots[i])&&
            model.writes_integer(model.slots[i])&&model.slots[i].pdst==pdst) {
            unsigned age=model.age(model.slots[i].rob);
            if (fault_source<0||age<fault_age||(age==fault_age&&i<fault_source)) {
                fault_source=i; fault_age=age;
            }
        }
        model.fault_pdst=pdst; model.fault_rob=model.slots[fault_source].rob;
        model.fault_allocation=model.slots[fault_source].allocation;
        model.fault_cause=WRITEBACK_VALIDATION_FAULT_CAUSE;
        for (int i=0;i<3;i++) if (model.slot_valid[i]&&model.event_valid(model.slots[i])&&
            model.writes_integer(model.slots[i])&&model.slots[i].pdst==pdst) {
            SourceEvent& e=model.slots[i]; Owner& o=model.rob[e.rob]; o.busy=false; o.exception=true;
            o.cause=WRITEBACK_VALIDATION_FAULT_CAUSE;
            Token& t=model.tokens[e.token]; if (t.terminal==TOKEN_LIVE) {
                model.set_token_terminal(t.id,TOKEN_FAULTED); metrics.tokens_faulted++;
            }
            expected.faulted_events.push_back(e.id); model.set_event_terminal(e,EVENT_FAULTED);
            model.slot_valid[i]=false;
        }
        return true;
    }

    void build_publications(ExpectedCycle& expected) {
        bool usable[3]={false,false,false};
        for (int i=0;i<3;i++) usable[i]=model.slot_valid[i]&&model.forwardable(model.slots[i]);
        unsigned exception_age=ROB_DEPTH; bool exception=precise_fence(exception_age);
        unsigned branch_age=ROB_DEPTH; bool branch=false;
        for (int i=0;i<3;i++) if (model.slot_valid[i]&&model.event_valid(model.slots[i])&&
            model.slots[i].branch) { unsigned a=model.age(model.slots[i].rob);
            if (!branch||a<branch_age) { branch=true; branch_age=a; } }
        for (int i=0;i<3;i++) if (usable[i]&&((exception&&model.age(model.slots[i].rob)>exception_age)||
            (branch&&model.age(model.slots[i].rob)>branch_age))) usable[i]=false;
        for (int i=0;i<3;i++) if (usable[i]) for (int j=i+1;j<3;j++) if (usable[j]&&
            model.slots[i].pdst==model.slots[j].pdst&&model.slots[i].value==model.slots[j].value) {
            usable[j]=false; model.slots[j].wakeup_sent=true;
        }
        for (int i=0;i<3;i++) if (usable[i]) {
            SourceEvent& e=model.slots[i];
            Publication p={e.id,e.token,e.value,e.pdst,e.rob,e.branch_mask,e.allocation,e.source};
            expected.wakeups.push_back(p);
            if (e.wakeup_sent) metrics.repeated_wakeup_publications++;
            e.wakeup_sent=true;
        }
    }

    bool candidate_conflict(int candidate) const {
        const SourceEvent& e=model.slots[candidate]; if (!model.writes_integer(e)) return false;
        for (int i=0;i<3;i++) if (i!=candidate&&model.slot_valid[i]&&model.event_valid(model.slots[i])&&
            model.writes_integer(model.slots[i])&&model.slots[i].pdst==e.pdst&&
            model.slots[i].value!=e.value) return true;
        return false;
    }

    void accept_event(int source,bool duplicate,ExpectedCycle& expected) {
        SourceEvent e=model.slots[source]; Owner& o=model.rob[e.rob];
        bool completes=e.kind!=COMPLETION_MEMORY_ADDRESS||!e.is_load;
        if (e.kind==COMPLETION_MEMORY_ADDRESS&&e.is_load) {
            o.memory_valid=true; o.is_load=true; o.signed_load=e.signed_load; o.address=e.address;
            o.memory_mask=e.memory_mask; o.memory_size=e.memory_size;
            model.ldq.push_back(QueueOwner(e.rob,e.allocation,e.branch_mask));
        } else if (e.kind==COMPLETION_STORE) {
            o.memory_valid=true; o.memory_completed=true; o.is_store=true; o.address=e.address;
            o.memory_data=e.store_data; o.memory_mask=e.memory_mask; o.memory_size=e.memory_size;
            model.stq.push_back(QueueOwner(e.rob,e.allocation,e.branch_mask));
        }
        if (completes) {
            o.busy=false; expected.rob_completes.push_back(e.token);
            Token& t=model.tokens[e.token]; t.rob_completes++;
            if (t.rob_completes>1) metrics.duplicate_tokens++;
            t.rob_complete_cycle=(int)cycle;
        }
        if (e.exception) {
            o.exception=true; o.cause=e.cause;
            if (model.set_token_terminal(e.token,TOKEN_FAULTED)) metrics.tokens_faulted++;
            model.fault_active=true;
        }
        if (model.writes_integer(e)) {
            model.busy[e.pdst]=false;
            if (!duplicate) {
                model.prf[e.pdst]=e.value;
                Publication p={e.id,e.token,e.value,e.pdst,e.rob,e.branch_mask,e.allocation,e.source};
                expected.writebacks.push_back(p);
                Token& t=model.tokens[e.token]; t.prf_writes++; t.writeback_cycle=(int)cycle;
                if (t.prf_writes>1) metrics.duplicate_tokens++;
            }
        }
        if (e.kind==COMPLETION_LOAD_RESPONSE) {
            o.memory_completed=true; if (!e.exception) o.memory_data=e.value;
            model.load_pending=false; model.pending_load_transaction=0;
            model.pending_load_allocation=0; model.pending_load_rob=0;
            model.ldq.erase(std::remove_if(model.ldq.begin(),model.ldq.end(),
                [&e](const QueueOwner& q){return q.rob==e.rob&&q.allocation==e.allocation;}),
                model.ldq.end());
        }
        Token& t=model.tokens[e.token]; t.ever_accepted=true;
        if (t.first_accept<0) t.first_accept=(int)cycle;
        expected.accepted_events.push_back(e.id); model.set_event_terminal(model.slots[source],EVENT_ACCEPTED);
        model.slot_valid[source]=false;
    }

    ExpectedCycle model_completion_step() {
        ExpectedCycle expected;
        for (int i=0;i<3;i++) { debug_valid[i]=model.slot_valid[i]; debug_slots[i]=model.slots[i]; }
        for (int i=0;i<3;i++) if (model.slot_valid[i]&&!model.event_valid(model.slots[i])) {
            model.set_event_terminal(model.slots[i],EVENT_REJECTED); metrics.events_rejected++;
            model.slot_valid[i]=false;
        }
        resolve_branch(expected);
        if (detect_conflict(expected)) return expected;
        build_publications(expected);
        unsigned exception_age=ROB_DEPTH; bool exception=precise_fence(exception_age);
        for (int service=0;service<3;service++) {
            int selected=-1; unsigned selected_age=ROB_DEPTH; bool selected_duplicate=false;
            unsigned branch_age=ROB_DEPTH; bool branch=false;
            for (int i=0;i<3;i++) if (model.slot_valid[i]&&model.event_valid(model.slots[i])&&
                model.slots[i].branch) { unsigned a=model.age(model.slots[i].rob);
                if (!branch||a<branch_age) { branch=true; branch_age=a; } }
            for (int i=0;i<3;i++) {
                if (!model.slot_valid[i]||!model.event_valid(model.slots[i])) continue;
                SourceEvent& e=model.slots[i]; unsigned a=model.age(e.rob);
                bool fenced=(exception&&a>exception_age)||(branch&&a>branch_age);
                if (fenced) { e.fence_blocked_cycles++; metrics.fence_blocked_cycles++; continue; }
                bool duplicate=false, eligible=true;
                if (e.kind==COMPLETION_MEMORY_ADDRESS&&e.is_load&&model.ldq.size()>=LDQ_DEPTH) {
                    eligible=false; metrics.ldq_capacity_blocked_cycles++;
                }
                if (e.kind==COMPLETION_STORE&&model.stq.size()>=STQ_DEPTH) {
                    eligible=false; metrics.stq_capacity_blocked_cycles++;
                }
                if (model.writes_integer(e)) {
                    if (candidate_conflict(i)) eligible=false;
                    for (size_t w=0;w<expected.writebacks.size();w++)
                        if (expected.writebacks[w].pdst==e.pdst&&expected.writebacks[w].value==e.value)
                            duplicate=true;
                    if (!duplicate&&expected.writebacks.size()>=NUM_INT_WRITEBACK_PORTS) eligible=false;
                }
                if (!eligible) continue;
                if (selected<0||a<selected_age||(a==selected_age&&i<selected)) {
                    selected=i; selected_age=a; selected_duplicate=duplicate;
                }
            }
            if (selected<0) break;
            bool stop=model.slots[selected].exception||model.slots[selected].branch;
            accept_event(selected,selected_duplicate,expected);
            if (stop) break;
        }
        for (int i=0;i<3;i++) if (model.slot_valid[i]&&model.event_valid(model.slots[i])) {
            SourceEvent& e=model.slots[i]; unsigned a=model.age(e.rob);
            unsigned ex_age=ROB_DEPTH; bool ex=precise_fence(ex_age);
            bool branch=false; unsigned br_age=ROB_DEPTH;
            for (int j=0;j<3;j++) if (model.slot_valid[j]&&model.event_valid(model.slots[j])&&
                model.slots[j].branch) { unsigned x=model.age(model.slots[j].rob);
                if (!branch||x<br_age) { branch=true; br_age=x; } }
            bool fenced=(ex&&a>ex_age)||(branch&&a>br_age)||model.fault_active;
            bool conflict=candidate_conflict(i);
            if (model.writes_integer(e)&&!fenced&&!conflict) {
                e.eligible_wait++;
                metrics.max_eligible_write_arbitration_wait=
                    std::max(metrics.max_eligible_write_arbitration_wait,e.eligible_wait);
            }
        }
        return expected;
    }

    void prepare_dut_cycle() {
        dut.brupdate=BranchUpdate();
        dut.completion.validation_fault_this_cycle=false;
    }

    bool compare_publication(const Publication& expected,const WakeupEvent& actual) const {
        return actual.valid&&actual.pdst==expected.pdst&&actual.value==expected.value&&
            actual.rob_idx==expected.rob&&actual.rob_allocation_id==expected.allocation&&
            actual.branch_mask==expected.branch_mask&&actual.source==expected.source;
    }
    bool compare_writeback(const Publication& expected,const WritebackEvent& actual) const {
        return actual.valid&&actual.pdst==expected.pdst&&actual.value==expected.value&&
            actual.rob_idx==expected.rob&&actual.rob_allocation_id==expected.allocation&&
            actual.source==expected.source;
    }

    bool compare_cycle(const ExpectedCycle& expected) {
        if (dut.completion.completion_accepts_this_cycle!=expected.accepted_events.size())
            std::printf("DETAIL expected_accepts=%zu actual_accepts=%u expected_fault=%d actual_fault=%d "
                        "expected_wb=%zu actual_wb=%u\n",expected.accepted_events.size(),
                        dut.completion.completion_accepts_this_cycle,expected.validation_fault,
                        dut.completion.validation_fault_this_cycle,expected.writebacks.size(),
                        dut.completion.prf_writes_this_cycle);
        if (!check(dut.completion.completion_accepts_this_cycle==expected.accepted_events.size(),
                   "completion","exact acceptance count")) return false;
        if (!check(dut.completion.rob_completes_this_cycle==expected.rob_completes.size(),
                   "completion","exact ROB-complete count")) return false;
        if (!check(dut.completion.prf_writes_this_cycle==expected.writebacks.size(),
                   "completion","exact writeback count")) return false;
        if (!check(dut.completion.wakeups_this_cycle==expected.wakeups.size()&&
                   dut.completion.bypass_this_cycle==expected.wakeups.size(),
                   "publication","exact wakeup/bypass count")) return false;
        for (size_t i=0;i<expected.writebacks.size();i++) {
            if (!check(compare_writeback(expected.writebacks[i],dut.completion.writebacks[i]),
                       "writeback","ordered identity/value mismatch")) return false;
            metrics.exact_writeback_checks++;
        }
        for (size_t i=expected.writebacks.size();i<NUM_INT_WRITEBACK_PORTS;i++)
            if (!check(!dut.completion.writebacks[i].valid,"writeback","unexpected port")) return false;
        for (size_t i=0;i<expected.wakeups.size();i++) {
            if (!check(compare_publication(expected.wakeups[i],dut.completion.wakeups[i]),
                       "wakeup","ordered identity/value mismatch")) return false;
            const BypassEvent& b=dut.completion.bypass[i];
            if (!check(b.valid&&b.pdst==expected.wakeups[i].pdst&&b.value==expected.wakeups[i].value&&
                b.rob_idx==expected.wakeups[i].rob&&b.rob_allocation_id==expected.wakeups[i].allocation&&
                b.branch_mask==expected.wakeups[i].branch_mask&&
                b.source==expected.wakeups[i].source,"bypass","ordered identity/value mismatch")) return false;
            metrics.exact_wakeup_checks++;
        }
        for (size_t i=expected.wakeups.size();i<NUM_INT_WAKEUP_PORTS;i++)
            if (!check(!dut.completion.wakeups[i].valid,
                       "wakeup","unexpected publication")) return false;
        for (size_t i=expected.wakeups.size();i<NUM_INT_BYPASS_PORTS;i++)
            if (!check(!dut.completion.bypass[i].valid,
                       "bypass","unexpected publication")) return false;
        const BranchUpdate& branch=dut.brupdate;
        if (!check(branch.valid==expected.branch_valid&&
            (!expected.branch_valid||(branch.mispredict==expected.branch_mispredict&&
             branch.taken==expected.branch_taken&&branch.jalr_target==expected.branch_target&&
             branch.resolve_mask==expected.resolve_mask&&branch.mispredict_mask==expected.mispredict_mask&&
             branch.br_tag==expected.branch_tag&&branch.cfi_type==0&&branch.pc_sel==0&&
             branch.target_offset==0&&branch.uop.queue.rob_idx==expected.branch_rob&&
             branch.uop.queue.rob_allocation_id==expected.branch_allocation&&
             branch.uop.debug_pc==expected.branch_token&&branch.uop.branch.br_tag==expected.branch_tag&&
             branch.uop.branch.br_mask==expected.branch_uop_mask)),
             "branch","full BranchUpdate mismatch")) return false;
        if (expected.branch_valid) metrics.full_branch_checks++;
        if (!check(dut.completion.validation_fault_this_cycle==expected.validation_fault,
                   "fault","validation fault mismatch")) return false;
        if (!check(dut.completion.writeback_conflict==model.conflict_active&&
            dut.completion.writeback_fault_valid==model.conflict_active&&
            (!model.conflict_active||(dut.completion.writeback_fault_pdst==model.fault_pdst&&
             dut.completion.writeback_fault_rob_idx==model.fault_rob&&
             dut.completion.writeback_fault_allocation_id==model.fault_allocation&&
             dut.completion.writeback_fault_cause==model.fault_cause)),
             "fault","sticky/current conflict metadata mismatch")) return false;
        metrics.fault_metadata_checks++;
        metrics.exact_rob_complete_checks+=expected.rob_completes.size();
        metrics.events_accepted+=expected.accepted_events.size();
        metrics.events_killed+=expected.killed_events.size();
        metrics.events_faulted+=expected.faulted_events.size();
        metrics.prf_writes+=expected.writebacks.size(); metrics.rob_completes+=expected.rob_completes.size();
        metrics.wakeup_publications+=expected.wakeups.size(); metrics.bypass_publications+=expected.wakeups.size();
        metrics.peak_writes=std::max(metrics.peak_writes,(unsigned)expected.writebacks.size());
        metrics.peak_wakeups=std::max(metrics.peak_wakeups,(unsigned)expected.wakeups.size());
        metrics.peak_bypass=std::max(metrics.peak_bypass,(unsigned)expected.wakeups.size());
        for (int i=0;i<ROB_DEPTH;i++) {
            const Owner& e=model.rob[i]; const RobEntry& a=dut.rob.entries[i];
            if (!check(a.valid==e.valid,"ROB","valid mismatch")) return false;
            if (!e.valid) continue;
            if (!check(a.busy==e.busy&&a.exception==e.exception&&
                a.exception_reported==e.exception_reported&&a.uop.queue.rob_idx==i&&
                a.uop.queue.rob_allocation_id==e.allocation&&a.uop.debug_pc==e.token&&
                a.uop.inst==(uint32_t)e.token&&a.uop.rename.pdst==e.pdst&&
                a.uop.rename.ldst==(uint8_t)(e.pdst%LOGICAL_REG_COUNT)&&
                a.uop.rename.dst_rtype==(e.writes?DST_INT:DST_N)&&
                a.uop.branch.br_mask==e.branch_mask&&
                (!e.exception||(a.uop.exception&&a.uop.exc_cause==e.cause))&&
                a.memory_valid==e.memory_valid&&a.is_load==e.is_load&&a.is_store==e.is_store&&
                a.signed_load==e.signed_load&&a.memory_request_sent==e.memory_request_sent&&
                a.memory_completed==e.memory_completed&&a.memory_address==e.address&&
                a.memory_data==e.memory_data&&a.memory_mask==e.memory_mask&&
                a.memory_size==e.memory_size&&a.memory_transaction_id==e.transaction,
                "ROB","full owner/exception/memory state mismatch")) return false;
        }
        for (int p=0;p<INT_PHYS_REGS;p++) {
            if (!check(boom::prf_read(dut,p)==model.prf[p],"PRF","value mismatch")) return false;
            if (!check(dut.rename.int_free_list.busy_table[p]==model.busy[p],"PRF","busy mismatch")) return false;
            metrics.prf_checks++; metrics.busy_checks++;
        }
        for (int i=0;i<3;i++) {
            const CompletionEvent* d=i==0?&dut.completion.load_response:
                (i==1?&dut.completion.mem_execute:&dut.completion.int_execute);
            if (!check(d->valid==model.slot_valid[i],"source-hold","valid mismatch")) return false;
            if (model.slot_valid[i]) { const SourceEvent& e=model.slots[i];
                if (!check(d->uop.queue.rob_idx==e.rob&&d->uop.queue.rob_allocation_id==e.allocation&&
                    d->uop.debug_pc==e.token&&d->uop.rename.pdst==e.pdst&&
                    d->uop.branch.br_mask==e.branch_mask&&d->value==e.value&&d->kind==e.kind&&
                    d->source==e.source&&d->writes_prf==e.writes&&d->exception==e.exception&&
                    d->exc_cause==e.cause&&d->mispredict==e.mispredict&&
                    d->control_resolved==e.control_resolved&&d->redirect_pc==e.redirect_pc&&
                    d->memory_valid==(e.is_load||e.is_store)&&
                    d->is_load==e.is_load&&d->is_store==e.is_store&&
                    d->signed_load==e.signed_load&&d->memory_address==e.address&&
                    d->store_data==e.store_data&&d->memory_mask==e.memory_mask&&
                    d->memory_size==e.memory_size&&d->transaction_id==e.transaction,
                    "source-hold","full identity/payload mismatch")) return false;
            }
        }
        if (!check(dut.rob.head==model.head&&dut.rob.tail==model.tail&&dut.rob.maybe_full==model.maybe_full,
                   "ROB","pointers mismatch")) return false;
        if (!check(dut.completion.dropped_completions==0&&
                   dut.completion.dropped_writebacks==0&&dut.completion.duplicate_writebacks==0,
                   "accounting","DUT drop/duplicate counter")) return false;
        return true;
    }

    bool iq_probe(const ExpectedCycle& expected) {
        if (expected.wakeups.empty()) return true;
        IssueSlotEntry q; q.valid=q.request=true; q.uop.uopc=1; q.uop.iq_type=IQ_ALU; q.uop.fu_code=FU_ALU;
        const Publication& a=expected.wakeups[0];
        const Publication& b=expected.wakeups[expected.wakeups.size()>1?1:0];
        const Publication& c=expected.wakeups[expected.wakeups.size()>2?2:0];
        q.uop.rename.prs1=a.pdst; q.uop.rename.prs2=b.pdst; q.uop.rename.prs3=c.pdst;
        dut.issue.alu_iq.entries[0]=q; dut.issue.alu_iq.count=1; dut.issue.alu_iq.tail=1;
        dut.issue.port_ready[0]=dut.issue.port_ready[1]=false;
        boom::issue_module(dut);
        const IssueSlotEntry& out=dut.issue.alu_iq.entries[0];
        bool ok=!out.prs1_busy&&!out.prs2_busy&&!out.prs3_busy&&
            out.prs1_data==a.value&&out.prs2_data==b.value&&out.prs3_data==c.value;
        dut.issue.alu_iq=IssueQueueState();
        metrics.iq_prs1_checks++; metrics.iq_prs2_checks++; metrics.iq_prs3_checks++;
        return check(ok,"IQ","prs1/prs2/prs3 publication value");
    }

    bool older_store(uint8_t load_idx) const {
        uint8_t idx=model.head;
        for (int i=0;i<ROB_DEPTH;i++) {
            if (idx==load_idx) return false;
            if (model.rob[idx].valid&&model.rob[idx].is_store) return true;
            idx=(uint8_t)((idx+1)%ROB_DEPTH);
        }
        return false;
    }

    bool lsu_issue() {
        int selected=-1;
        if (!model.load_pending&&!dmem_block) for (int i=0;i<ROB_DEPTH;i++) {
            uint8_t idx=(uint8_t)((model.head+i)%ROB_DEPTH); Owner& o=model.rob[idx];
            if (o.valid&&o.is_load&&o.memory_valid&&!o.memory_request_sent&&!older_store(idx)) {
                selected=idx; break;
            }
        }
        if (dmem_block) fill_dmem();
        boom::lsu_module(dut,pipe);
        if (dmem_block) drain_dmem();
        if (selected<0) return check(pipe.dmem_req.empty(),"LSU","unexpected load request");
        if (!check(!pipe.dmem_req.empty(),"LSU","missing load request")) return false;
        DmemRequest request=pipe.dmem_req.read(); Owner& o=model.rob[selected];
        uint32_t tx=model.next_transaction++;
        bool request_matches=request.transaction_id==tx&&request.rob_idx==selected&&
            request.command==DMEM_LOAD&&!request.is_store&&!request.committed&&
            request.address==o.address&&request.size==o.memory_size&&
            request.mask==o.memory_mask&&request.signed_load==o.signed_load&&
            request.branch_mask==o.branch_mask&&request.data==0&&request.write_data==0&&
            request.write_mask==0&&request.epoch==0;
        if (!request_matches)
            std::printf("LSU_DETAIL expected_tx=%u actual_tx=%u expected_rob=%d actual_rob=%u "
                        "expected_addr=%llu actual_addr=%llu\n",tx,request.transaction_id,
                        selected,request.rob_idx,(unsigned long long)o.address,
                        (unsigned long long)request.address);
        if (!check(request_matches,"LSU","full load DmemRequest mismatch")) return false;
        metrics.full_load_request_checks++;
        o.memory_request_sent=true; o.transaction=tx; model.load_pending=true;
        model.pending_load_rob=(uint8_t)selected; model.pending_load_allocation=o.allocation;
        model.pending_load_transaction=tx;
        LoadPlan plan; plan.token=o.token; plan.rob=(uint8_t)selected; plan.allocation=o.allocation;
        plan.transaction=tx; plan.data=((uint64_t)rng.next()<<32)|rng.next();
        plan.delay=2+(int)rng.range(12); responses.push_back(plan);
        return true;
    }

    void fill_trace() { CommitEntry e; while (!pipe.commit_trace.full()) pipe.commit_trace.write(e); }
    void fill_dmem() { DmemRequest e; while (!pipe.dmem_req.full()) pipe.dmem_req.write(e); }
    void drain_trace() { while (!pipe.commit_trace.empty()) pipe.commit_trace.read(); }
    void drain_dmem() { while (!pipe.dmem_req.empty()) pipe.dmem_req.read(); }

    bool commit(ExpectedCycle& expected) {
        if (trace_block) fill_trace();
        if (dmem_block) fill_dmem();
        if (!model.empty()&&model.rob[model.head].valid&&!model.rob[model.head].busy) {
            Owner& o=model.rob[model.head];
            bool blocked=trace_block||(o.is_load&&!o.memory_completed)||
                (o.is_store&&!o.memory_request_sent&&dmem_block);
            if (o.exception) {
                if (!trace_block&&!o.exception_reported) {
                    expected.commit_valid=true; expected.commit_exception=true;
                    expected.commit_token=o.token; expected.commit_inst=(uint32_t)o.token;
                    expected.commit_priv=PRV_M; expected.commit_cause=o.cause;
                    o.exception_reported=true;
                }
                if (!trace_block) { model.rob_state=ROB_EXCEPTION; }
            } else if (!blocked) {
                expected.commit_valid=true; expected.commit_token=o.token;
                expected.commit_value=(o.writes&&o.pdst)?model.prf[o.pdst]:0;
                expected.commit_inst=(uint32_t)o.token;
                expected.commit_rd=(uint8_t)(o.pdst%LOGICAL_REG_COUNT);
                expected.commit_rd_valid=o.writes&&o.pdst!=0; expected.commit_priv=PRV_M;
                expected.commit_memory_valid=o.memory_valid; expected.commit_is_store=o.is_store;
                expected.commit_memory_address=o.address; expected.commit_memory_data=o.memory_data;
                expected.commit_memory_mask=o.memory_mask; expected.commit_store_address=o.address;
                expected.commit_store_data=o.memory_data; expected.commit_store_mask=o.memory_mask;
                if (o.is_store&&!o.memory_request_sent) {
                    expected.store_request=true; o.transaction=model.next_transaction++;
                    expected.store_rob=model.head;
                    expected.store_transaction=o.transaction; expected.store_address=o.address;
                    expected.store_data=o.memory_data; expected.store_size=o.memory_size;
                    expected.store_mask=o.memory_mask; expected.store_branch_mask=o.branch_mask;
                    o.memory_request_sent=o.memory_completed=true;
                    uint8_t store_rob=model.head; uint32_t store_allocation=o.allocation;
                    model.stq.erase(std::remove_if(model.stq.begin(),model.stq.end(),
                        [store_rob,store_allocation](const QueueOwner& q){
                            return q.rob==store_rob&&q.allocation==store_allocation;
                        }),model.stq.end());
                }
                Token& t=model.tokens[o.token];
                if (!model.set_token_terminal(t.id,TOKEN_COMMITTED)) metrics.duplicate_tokens++;
                t.commits++; model.csr_instret++;
                t.commit_cycle=(int)cycle; metrics.tokens_committed++; metrics.commits++;
                if (t.rob_complete_cycle>=0) {
                    unsigned latency=(unsigned)((int)cycle-t.rob_complete_cycle);
                    metrics.rob_to_commit_samples++; metrics.rob_to_commit_sum+=latency;
                    metrics.max_rob_to_commit=std::max(metrics.max_rob_to_commit,latency);
                }
                model.rob[model.head]=Owner(); model.head=(uint8_t)((model.head+1)%ROB_DEPTH);
                model.maybe_full=false;
            }
        }
        boom::rob_commit_module(dut,pipe);
        if (trace_block) {
            drain_trace();
            if (!check(!expected.commit_valid&&!dut.rob.commit_valid,"commit","trace backpressure")) return false;
        } else {
            if (!check(dut.rob.commit_valid==expected.commit_valid,"commit","valid mismatch")) return false;
            if (expected.commit_valid) {
                if (!check(!pipe.commit_trace.empty(),"commit","trace absent")) return false;
                CommitEntry ce=pipe.commit_trace.read();
                if (ce.pc!=expected.commit_token||ce.exception!=expected.commit_exception||
                    (!ce.exception&&ce.rd_value!=expected.commit_value))
                    std::printf("COMMIT_DETAIL expected_token=%llu actual_token=%llu expected_value=%llu "
                                "actual_value=%llu expected_exception=%d actual_exception=%d\n",
                                (unsigned long long)expected.commit_token,(unsigned long long)ce.pc,
                                (unsigned long long)expected.commit_value,(unsigned long long)ce.rd_value,
                                expected.commit_exception,ce.exception);
                if (!check(ce.valid&&ce.pc==expected.commit_token&&ce.inst==expected.commit_inst&&
                    ce.priv==expected.commit_priv&&ce.exception==expected.commit_exception&&
                    ce.exc_cause==expected.commit_cause&&ce.rd==expected.commit_rd&&
                    ce.rd_valid==expected.commit_rd_valid&&
                    (!ce.rd_valid||ce.rd_value==expected.commit_value)&&
                    ce.branch_mispredict==false&&ce.memory_valid==expected.commit_memory_valid&&
                    ce.is_store==expected.commit_is_store&&
                    ce.memory_address==expected.commit_memory_address&&
                    ce.memory_data==expected.commit_memory_data&&
                    ce.memory_mask==expected.commit_memory_mask&&
                    ce.store_addr==expected.commit_store_address&&
                    ce.store_data==expected.commit_store_data&&ce.store_mask==expected.commit_store_mask,
                    "commit","full trace payload mismatch")) return false;
                metrics.commit_payload_checks++;
            }
        }
        if (dmem_block) drain_dmem();
        else if (expected.store_request) {
            if (!check(!pipe.dmem_req.empty(),"commit","store request absent")) return false;
            DmemRequest req=pipe.dmem_req.read();
            if (!req.is_store||req.rob_idx!=expected.store_rob)
                std::printf("STORE_DETAIL expected_rob=%u actual_rob=%u is_store=%d tx=%u\n",
                            expected.store_rob,req.rob_idx,req.is_store,req.transaction_id);
            if (!check(req.transaction_id==expected.store_transaction&&req.is_store&&req.committed&&
                req.command==DMEM_STORE&&req.rob_idx==expected.store_rob&&
                req.address==expected.store_address&&req.data==expected.store_data&&
                req.write_data==expected.store_data&&req.size==expected.store_size&&
                req.mask==expected.store_mask&&req.write_mask==expected.store_mask&&
                req.branch_mask==expected.store_branch_mask,
                "commit","full store request mismatch")) return false;
        }
        metrics.csr_counter_checks++;
        return check(dut.rob.head==model.head&&dut.rob.tail==model.tail&&
                     dut.rob.maybe_full==model.maybe_full&&dut.rob.state==model.rob_state&&
                     dut.csr.instret==model.csr_instret,"commit","ROB/CSR state mismatch");
    }

    bool compare_lsu() {
        unsigned loads=model.ldq.size(),stores=model.stq.size();
        bool ok=dut.lsu.load_response_pending==model.load_pending&&
            (!model.load_pending||(dut.lsu.pending_load_transaction_id==model.pending_load_transaction&&
             dut.lsu.pending_load_rob_idx==model.pending_load_rob&&
             dut.lsu.pending_load_allocation_id==model.pending_load_allocation))&&
            dut.lsu.ldq_count==loads&&dut.lsu.stq_count==stores&&
            dut.lsu.ldq_head==0&&dut.lsu.ldq_tail==loads%LDQ_DEPTH&&
            dut.lsu.stq_head==0&&dut.lsu.stq_tail==stores%STQ_DEPTH&&
            dut.lsu.next_transaction_id==model.next_transaction;
        if (!ok) std::printf("LSU_COUNT_DETAIL expected_loads=%u actual_loads=%u expected_stores=%u "
                             "actual_stores=%u expected_pending=%d actual_pending=%d\n",loads,
                             dut.lsu.ldq_count,stores,dut.lsu.stq_count,model.load_pending,
                             dut.lsu.load_response_pending);
        if (!check(ok,"LSU","independent ownership/count mismatch")) return false;
        for (size_t i=0;i<model.ldq.size();i++) {
            const QueueOwner& q=model.ldq[i]; const LoadQueueEntry& d=dut.lsu.ldq[i];
            const Owner& o=model.rob[q.rob];
            if (!check(d.valid&&d.rob_idx==q.rob&&d.rob_allocation_id==q.allocation&&
                d.address==o.address&&d.size==o.memory_size&&d.signed_load==o.signed_load&&
                d.branch_mask==q.branch_mask&&!d.killed,"LSU","full LDQ entry mismatch")) return false;
        }
        for (size_t i=model.ldq.size();i<LDQ_DEPTH;i++)
            if (!check(!dut.lsu.ldq[i].valid,"LSU","unexpected LDQ entry")) return false;
        for (size_t i=0;i<model.stq.size();i++) {
            const QueueOwner& q=model.stq[i]; const StoreQueueEntry& d=dut.lsu.stq[i];
            const Owner& o=model.rob[q.rob];
            if (!check(d.valid&&d.rob_idx==q.rob&&d.rob_allocation_id==q.allocation&&
                d.address_valid&&d.address==o.address&&d.data_valid&&d.data==o.memory_data&&
                d.mask==o.memory_mask&&d.size==o.memory_size&&d.branch_mask==q.branch_mask&&
                !d.killed,"LSU","full STQ entry mismatch")) return false;
        }
        for (size_t i=model.stq.size();i<STQ_DEPTH;i++)
            if (!check(!dut.lsu.stq[i].valid,"LSU","unexpected STQ entry")) return false;
        metrics.lsu_ownership_checks++;
        return true;
    }

    void classify_reset() {
        for (int i=0;i<3;i++) if (model.slot_valid[i]) {
            model.set_event_terminal(model.slots[i],EVENT_KILLED); metrics.events_killed++;
            model.slot_valid[i]=false;
        }
        for (int i=0;i<ROB_DEPTH;i++) model.kill_owner((uint8_t)i,metrics,true);
        for (size_t i=0;i<responses.size();i++) responses[i].reset_seen=true;
        model.head=model.tail=0; model.maybe_full=false; model.branch_active=false;
        model.fault_active=false; model.conflict_active=false; model.fault_pdst=0;
        model.fault_rob=0; model.fault_allocation=0; model.fault_cause=0;
        model.load_pending=false; model.pending_load_transaction=0;
        model.pending_load_allocation=0; model.pending_load_rob=0; model.rebuild_busy();
        model.ldq.clear(); model.stq.clear();
        model.csr_instret=0; model.rob_state=ROB_NORMAL;
    }

    bool reset() {
        if (!check(model.owner_count()>0,"reset","reset without in-flight work")) return false;
        classify_reset(); ResetControllerState controller;
        for (int i=0;i<256&&!controller.completed;i++) boom_core_reset_step(dut,controller);
        if (!check(controller.completed,"reset","controller timeout")) return false;
        dut.rob.state=ROB_NORMAL; dut.rob.next_allocation_id=model.next_allocation;
        dut.lsu.next_transaction_id=model.next_transaction;
        drain_trace(); drain_dmem(); while (!pipe.dmem_resp.empty()) pipe.dmem_resp.read();
        metrics.resets++; metrics.reset_with_inflight++; seed_resets++;
        return compare_quiescent_reset();
    }

    bool compare_quiescent_reset() {
        if (!check(dut.rob.head==0&&dut.rob.tail==0&&!dut.rob.maybe_full,
                   "reset","ROB not empty")) return false;
        if (!check(!dut.completion.load_response.valid&&!dut.completion.mem_execute.valid&&
            !dut.completion.int_execute.valid&&!dut.lsu.load_response_pending,
            "reset","pending state survived")) return false;
        for (int p=0;p<INT_PHYS_REGS;p++) if (!check(!dut.rename.int_free_list.busy_table[p],
            "reset","busy bit survived")) return false;
        for (int p=0;p<INT_PHYS_REGS;p++) if (!check(boom::prf_read(dut,p)==model.prf[p],
            "reset","PRF changed or diverged")) return false;
        return true;
    }

    bool should_reset(bool draining) {
        if (draining||model.owner_count()==0) return false;
        if (model.fault_active) return true;
        bool forced=(cycle==64&&seed_resets==0);
        bool load_reset=model.load_pending&&((cycle+seed_index)%37==0);
        return forced||load_reset||rng.one_in(67);
    }

    bool step(bool draining) {
        trace_block=!draining&&((cycle+seed_index)%29==0);
        dmem_block=!draining&&((cycle*3+seed_index)%31==0);
        if (trace_block) metrics.trace_backpressure_cycles++;
        if (dmem_block) metrics.dmem_backpressure_cycles++;
        if (should_reset(draining)) return reset();
        prepare_dut_cycle();
        bool response=maybe_response(draining);
        if (!stale_probe_active) generate_arrivals(draining);
        unsigned mask=model.source_mask(); metrics.source_combination_mask[mask]++;
        unsigned source_count=(mask&1)+((mask>>1)&1)+((mask>>2)&1);
        metrics.peak_sources=std::max(metrics.peak_sources,source_count);
        if (source_count>=2) {
            metrics.pending_pressure_cycles++;
            int previous=-1; bool permuted=false;
            for (int i=0;i<3;i++) if (model.slot_valid[i]) {
                int a=(int)model.age(model.slots[i].rob); if (previous>a) permuted=true; previous=a;
            }
            if (permuted) metrics.source_age_permutations++;
        }
        ExpectedCycle expected=model_completion_step();
        if (response) boom::completion_service_cycle(dut,pipe);
        else boom::completion_service_execute(dut);
        if (!check_stale_snapshot()) return false;
        if (!compare_cycle(expected)||!iq_probe(expected)) return false;
        if (!lsu_issue()) return false;
        if (!commit(expected)) return false;
        if (!compare_lsu()) return false;
        for (size_t i=0;i<expected.accepted_events.size();i++) {
            SourceEvent& e=model.event_history[expected.accepted_events[i]];
            Token& t=model.tokens[e.token];
            if (t.first_accept==(int)cycle&&t.first_offer>=0) {
                unsigned latency=(unsigned)((int)cycle-t.first_offer);
                metrics.offer_to_accept_samples++; metrics.offer_to_accept_sum+=latency;
                metrics.max_offer_to_accept=std::max(metrics.max_offer_to_accept,latency);
            }
        }
        return true;
    }

    bool drain() {
        unsigned used=0;
        while (used<kMaxDrainCycles) {
            bool any=model.owner_count()||model.source_mask()||model.load_pending||!responses.empty();
            if (!any) break;
            bool trapped=model.fault_active;
            for (int i=0;i<ROB_DEPTH;i++) trapped|=model.rob[i].valid&&model.rob[i].exception;
            if (trapped) { if (!reset()) return false; }
            else { cycle=kRandomCycles+used; if (!step(true)) return false; }
            used++;
        }
        metrics.drain_cycles+=used;
        if (!check(model.owner_count()==0&&model.source_mask()==0&&!model.load_pending&&responses.empty(),
                   "drain","unexplained in-flight work")) { metrics.unexplained_tokens++; return false; }
        metrics.scanned_tokens_offered+=model.token_origins.size();
        for (std::map<uint64_t,Token>::const_iterator it=model.token_origins.begin();
             it!=model.token_origins.end();++it) {
            uint64_t id=it->first; const Token& current=model.tokens[id];
            std::map<uint64_t,std::vector<TokenTerminal> >::const_iterator evidence=
                model.token_terminal_evidence.find(id);
            size_t classes=evidence==model.token_terminal_evidence.end()?0:evidence->second.size();
            if (classes==0) { metrics.scanned_token_pending++; metrics.terminal_missing_records++; }
            if (classes>1) metrics.terminal_duplicate_classifications+=classes-1;
            if (classes==1) {
                TokenTerminal terminal=evidence->second[0];
                if (terminal==TOKEN_COMMITTED) metrics.scanned_tokens_committed++;
                else if (terminal==TOKEN_KILLED) metrics.scanned_tokens_killed++;
                else if (terminal==TOKEN_FAULTED) metrics.scanned_tokens_faulted++;
                if (current.terminal!=terminal) metrics.terminal_duplicate_classifications++;
                if (current.ever_accepted) {
                    metrics.scanned_accepted_tokens++;
                    if (terminal==TOKEN_COMMITTED) metrics.scanned_accepted_committed++;
                    else if (terminal==TOKEN_KILLED) metrics.scanned_accepted_killed++;
                    else if (terminal==TOKEN_FAULTED) metrics.scanned_accepted_faulted++;
                }
            } else if (current.ever_accepted) metrics.scanned_accepted_pending++;
            if (current.prf_writes>1||current.rob_completes>1||current.commits>1) {
                metrics.duplicate_tokens++; return fail("drain","duplicate token side effect");
            }
        }
        metrics.scanned_events_offered+=model.event_origins.size();
        for (std::map<uint64_t,SourceEvent>::const_iterator it=model.event_origins.begin();
             it!=model.event_origins.end();++it) {
            std::map<uint64_t,std::vector<EventTerminal> >::const_iterator evidence=
                model.event_terminal_evidence.find(it->first);
            size_t classes=evidence==model.event_terminal_evidence.end()?0:evidence->second.size();
            if (classes==0) { metrics.scanned_event_pending++; metrics.terminal_missing_records++; }
            if (classes>1) metrics.terminal_duplicate_classifications+=classes-1;
            if (classes==1) {
                EventTerminal terminal=evidence->second[0];
                if (terminal==EVENT_ACCEPTED) metrics.scanned_events_accepted++;
                else if (terminal==EVENT_REJECTED) metrics.scanned_events_rejected++;
                else if (terminal==EVENT_KILLED) metrics.scanned_events_killed++;
                else if (terminal==EVENT_FAULTED) metrics.scanned_events_faulted++;
            }
        }
        for (std::map<uint64_t,std::vector<TokenTerminal> >::const_iterator it=
             model.token_terminal_evidence.begin();it!=model.token_terminal_evidence.end();++it)
            if (!model.token_origins.count(it->first)) metrics.terminal_missing_records++;
        for (std::map<uint64_t,std::vector<EventTerminal> >::const_iterator it=
             model.event_terminal_evidence.begin();it!=model.event_terminal_evidence.end();++it)
            if (!model.event_origins.count(it->first)) metrics.terminal_missing_records++;
        if (metrics.terminal_duplicate_classifications||metrics.terminal_missing_records)
            return fail("drain","immutable terminal classification failure");
        std::printf("SEED_PASS,index=%u,seed=0x%08x,random_cycles=%u,drain_cycles=%u,resets=%u,tokens=%zu,events=%zu\n",
                    seed_index,seed,kRandomCycles,used,seed_resets,model.tokens.size(),model.event_history.size());
        metrics.seed_passes++; return true;
    }
};

void metric(const char* name,uint64_t value) {
    std::printf("METRIC,%s,%llu\n",name,(unsigned long long)value);
}

bool production_order_probe(uint32_t seed) {
    BoomCoreState s; PipeSignals p; s.rob.tail=5;
    const uint64_t values[4] = {
        ((uint64_t)seed<<32)|0x1414, ((uint64_t)(seed^0x11111111u)<<32)|0x1515,
        ((uint64_t)(seed^0x22222222u)<<32)|0x1616, ((uint64_t)(seed^0x33333333u)<<32)|0x1717
    };
    for (int i=1;i<=4;i++) {
        RobEntry& entry=s.rob.entries[i]; entry.valid=entry.busy=true;
        entry.uop.uopc=1; entry.uop.queue.rob_idx=(uint8_t)i;
        entry.uop.queue.rob_allocation_id=700+i; entry.uop.rename.pdst=13+i;
        entry.uop.rename.dst_rtype=DST_INT;
        s.rename.int_free_list.busy_table[13+i]=true;
    }
    s.completion.load_response.valid=true;
    s.completion.load_response.kind=COMPLETION_EXECUTE;
    s.completion.load_response.source=COMPLETION_SOURCE_LSU_LOAD;
    s.completion.load_response.uop=s.rob.entries[1].uop;
    s.completion.load_response.writes_prf=true;
    s.completion.load_response.value=values[0];
    s.execute.alu_results[MEM_ISSUE_LANE].valid=true;
    s.execute.alu_results[MEM_ISSUE_LANE].uop=s.rob.entries[2].uop;
    s.execute.alu_results[MEM_ISSUE_LANE].result=values[1];
    s.execute.alu_results[INT_ISSUE_LANE].valid=true;
    s.execute.alu_results[INT_ISSUE_LANE].uop=s.rob.entries[3].uop;
    s.execute.alu_results[INT_ISSUE_LANE].result=values[2];
    RobEntry& load=s.rob.entries[4]; load.is_load=load.memory_valid=load.memory_request_sent=true;
    load.memory_size=3; load.memory_mask=0xff; load.memory_transaction_id=seed|1u;
    s.lsu.load_response_pending=true; s.lsu.pending_load_transaction_id=seed|1u;
    s.lsu.pending_load_rob_idx=4; s.lsu.pending_load_allocation_id=704;
    s.lsu.ldq_count=1; s.lsu.ldq_tail=1; s.lsu.ldq[0].valid=true;
    s.lsu.ldq[0].rob_idx=4; s.lsu.ldq[0].rob_allocation_id=704;
    DmemResponse response; response.transaction_id=seed|1u;
    response.data=response.read_data=values[3]; p.dmem_resp.write(response);
    s.completion.total_completion_accepts=40; s.completion.total_prf_writes=40;
    s.completion.total_wakeups=40;
    boom_core_step(s,p);
    if (p.dmem_resp.empty() || s.completion.prf_writes_this_cycle!=2 ||
        s.completion.wakeups_this_cycle!=3 || s.completion.total_completion_accepts!=42 ||
        s.completion.total_prf_writes!=42 || s.completion.total_wakeups!=43 ||
        s.completion.load_response.valid || !s.completion.int_execute.valid) return false;
    boom_core_step(s,p);
    return p.dmem_resp.empty() && s.completion.prf_writes_this_cycle==2 &&
        s.completion.completion_accepts_this_cycle==2 && !s.completion.int_execute.valid &&
        !s.completion.load_response.valid && !s.lsu.load_response_pending &&
        s.completion.peak_prf_writes==2 && s.completion.peak_wakeups==3 &&
        boom::prf_read(s,14)==values[0] && boom::prf_read(s,15)==values[1] &&
        boom::prf_read(s,16)==values[2] && boom::prf_read(s,17)==values[3];
}

} // namespace

int main() {
    static_assert(COMPLETION_PENDING_SLOTS==3,"W4E model requires three fixed source holds");
    static_assert(NUM_INT_WRITEBACK_PORTS==2,"W4E model requires two physical writers");
    Metrics metrics={}; metrics.seeds=kSeeds; metrics.random_cycles=kSeeds*kRandomCycles;
    uint64_t production_order_probes=0;
    for (unsigned index=0;index<kSeeds;index++) {
        uint32_t seed=0x243f6a88u^(0x9e3779b9u*(index+1));
        if (!production_order_probe(seed)) {
            std::printf("FAIL[production-order] seed=0x%08x\n",seed);
            return 1;
        }
        production_order_probes++;
    }
    uint64_t token_base=1,event_base=1;
    for (unsigned index=0;index<kSeeds;index++) {
        uint32_t seed=0x6a09e667u^(0x9e3779b9u*(index+1));
        Harness harness(index,seed,token_base,event_base,metrics);
        for (unsigned cycle=0;cycle<kRandomCycles;cycle++) {
            harness.cycle=cycle;
            if (!harness.step(false)) return 1;
        }
        if (!harness.drain()) return 1;
        token_base=harness.next_token+1; event_base=harness.next_event+1;
    }
    // Reconcile exclusively from immutable origins and append-only terminal evidence.
    metrics.tokens_offered=metrics.scanned_tokens_offered;
    metrics.tokens_committed=metrics.scanned_tokens_committed;
    metrics.tokens_killed=metrics.scanned_tokens_killed;
    metrics.tokens_faulted=metrics.scanned_tokens_faulted;
    metrics.accepted_tokens=metrics.scanned_accepted_tokens;
    metrics.accepted_terminal_committed=metrics.scanned_accepted_committed;
    metrics.accepted_then_killed=metrics.scanned_accepted_killed;
    metrics.accepted_terminal_faulted=metrics.scanned_accepted_faulted;
    metrics.events_offered=metrics.scanned_events_offered;
    metrics.events_accepted=metrics.scanned_events_accepted;
    metrics.events_rejected=metrics.scanned_events_rejected;
    metrics.events_killed=metrics.scanned_events_killed;
    metrics.events_faulted=metrics.scanned_events_faulted;
    metrics.source_conservation_lhs=metrics.events_offered;
    metrics.source_conservation_rhs=metrics.events_accepted+metrics.events_rejected+
        metrics.events_killed+metrics.events_faulted;
    metrics.token_conservation_lhs=metrics.tokens_offered;
    metrics.token_conservation_rhs=metrics.tokens_committed+metrics.tokens_killed+metrics.tokens_faulted;
    metrics.accepted_conservation_lhs=metrics.accepted_tokens;
    metrics.accepted_conservation_rhs=metrics.accepted_terminal_committed+
        metrics.accepted_then_killed+metrics.accepted_terminal_faulted;
    bool conservation=metrics.source_conservation_lhs==metrics.source_conservation_rhs&&
        metrics.token_conservation_lhs==metrics.token_conservation_rhs&&
        metrics.accepted_conservation_lhs==metrics.accepted_conservation_rhs;
    std::printf("W4E independent persistent completion random differential: %s\n",
                conservation?"PASS":"FAIL");
    metric("random_seeds",kSeeds); metric("seed_pass_records",metrics.seed_passes);
    metric("production_order_collision_probes",production_order_probes);
    metric("cycles_per_seed",kRandomCycles); metric("total_random_cycles",metrics.random_cycles);
    metric("bounded_drain_cycles",metrics.drain_cycles);
    metric("eligible_write_arbitration_wait_bound_cycles",kEligibleWriteArbitrationWaitBound);
    metric("max_eligible_write_arbitration_wait_cycles",
           metrics.max_eligible_write_arbitration_wait);
    metric("tokens_offered",metrics.tokens_offered); metric("tokens_committed",metrics.tokens_committed);
    metric("tokens_killed",metrics.tokens_killed); metric("tokens_faulted",metrics.tokens_faulted);
    metric("accepted_tokens",metrics.accepted_tokens);
    metric("accepted_terminal_committed",metrics.accepted_terminal_committed);
    metric("accepted_then_killed",metrics.accepted_then_killed);
    metric("accepted_terminal_faulted",metrics.accepted_terminal_faulted);
    metric("accepted_conservation_lhs",metrics.accepted_conservation_lhs);
    metric("accepted_conservation_rhs",metrics.accepted_conservation_rhs);
    metric("source_events_offered",metrics.events_offered);
    metric("source_events_accepted",metrics.events_accepted);
    metric("source_events_rejected",metrics.events_rejected);
    metric("source_events_killed",metrics.events_killed);
    metric("source_events_faulted",metrics.events_faulted);
    metric("source_events_pending_final",metrics.scanned_event_pending);
    metric("source_conservation_lhs",metrics.source_conservation_lhs);
    metric("source_conservation_rhs",metrics.source_conservation_rhs);
    metric("token_conservation_lhs",metrics.token_conservation_lhs);
    metric("token_conservation_rhs",metrics.token_conservation_rhs);
    metric("tokens_pending_final",metrics.scanned_token_pending);
    metric("accepted_pending_final",metrics.scanned_accepted_pending);
    metric("int_completion_events",metrics.int_events); metric("mem_completion_events",metrics.mem_events);
    metric("mem_agu_events",metrics.mem_agus); metric("store_events",metrics.stores);
    metric("load_response_events",metrics.load_responses);
    metric("stale_completion_events",metrics.stale_completions);
    metric("stale_response_events",metrics.stale_responses);
    metric("delayed_response_cycles",metrics.delayed_responses);
    metric("post_reset_stale_responses",metrics.post_reset_stale_responses);
    metric("branch_resolves",metrics.branch_resolves); metric("branch_mispredicts",metrics.branch_mispredicts);
    metric("precise_exception_events",metrics.exception_events);
    metric("validation_faults",metrics.validation_faults);
    metric("rob_index_wraps",metrics.rob_index_wraps);
    metric("rob_index_reuses",metrics.rob_index_reuses);
    metric("allocation_id_wraps",metrics.allocation_id_wraps);
    metric("resets",metrics.resets); metric("resets_with_inflight",metrics.reset_with_inflight);
    metric("reset_killed_tokens",metrics.reset_kills); metric("branch_killed_tokens",metrics.branch_kills);
    metric("fence_blocked_cycles",metrics.fence_blocked_cycles);
    metric("prf_writes",metrics.prf_writes); metric("rob_completes",metrics.rob_completes);
    metric("commits",metrics.commits); metric("wakeup_publications",metrics.wakeup_publications);
    metric("bypass_publications",metrics.bypass_publications);
    metric("repeated_wakeup_publications",metrics.repeated_wakeup_publications);
    metric("same_pdst_same_value",metrics.same_pdst_same);
    metric("same_pdst_different_value",metrics.same_pdst_different);
    for (int mask=0;mask<8;mask++) {
        char name[40]; std::snprintf(name,sizeof(name),"source_combination_mask_%u",mask);
        metric(name,metrics.source_combination_mask[mask]);
    }
    metric("source_age_permutations",metrics.source_age_permutations);
    metric("sustained_arrival_cycles",metrics.sustained_arrival_cycles);
    metric("pending_pressure_cycles",metrics.pending_pressure_cycles);
    metric("ldq_capacity_blocked_cycles",metrics.ldq_capacity_blocked_cycles);
    metric("stq_capacity_blocked_cycles",metrics.stq_capacity_blocked_cycles);
    metric("trace_backpressure_cycles",metrics.trace_backpressure_cycles);
    metric("dmem_backpressure_cycles",metrics.dmem_backpressure_cycles);
    metric("iq_prs1_checks",metrics.iq_prs1_checks); metric("iq_prs2_checks",metrics.iq_prs2_checks);
    metric("iq_prs3_checks",metrics.iq_prs3_checks); metric("prf_value_checks",metrics.prf_checks);
    metric("prf_busy_checks",metrics.busy_checks);
    metric("exact_writeback_checks",metrics.exact_writeback_checks);
    metric("exact_wakeup_checks",metrics.exact_wakeup_checks);
    metric("exact_rob_complete_checks",metrics.exact_rob_complete_checks);
    metric("full_branch_update_checks",metrics.full_branch_checks);
    metric("fault_metadata_checks",metrics.fault_metadata_checks);
    metric("commit_payload_checks",metrics.commit_payload_checks);
    metric("lsu_ownership_checks",metrics.lsu_ownership_checks);
    metric("csr_counter_checks",metrics.csr_counter_checks);
    metric("full_load_request_checks",metrics.full_load_request_checks);
    metric("offer_to_accept_latency_samples",metrics.offer_to_accept_samples);
    metric("offer_to_accept_latency_sum",metrics.offer_to_accept_sum);
    metric("offer_to_accept_latency_max",metrics.max_offer_to_accept);
    metric("rob_complete_to_commit_latency_samples",metrics.rob_to_commit_samples);
    metric("rob_complete_to_commit_latency_sum",metrics.rob_to_commit_sum);
    metric("rob_complete_to_commit_latency_max",metrics.max_rob_to_commit);
    metric("peak_completion_sources",metrics.peak_sources); metric("peak_prf_writes",metrics.peak_writes);
    metric("peak_wakeups",metrics.peak_wakeups); metric("peak_bypass",metrics.peak_bypass);
    metric("dropped_tokens",metrics.dropped_tokens); metric("duplicate_tokens",metrics.duplicate_tokens);
    metric("stale_side_effects",metrics.stale_side_effects);
    metric("stale_snapshot_checks",metrics.stale_snapshot_checks);
    metric("terminal_duplicate_classifications",metrics.terminal_duplicate_classifications);
    metric("terminal_missing_records",metrics.terminal_missing_records);
    metric("unexplained_tokens",metrics.unexplained_tokens);
    return conservation&&metrics.seed_passes==kSeeds&&
        metrics.max_eligible_write_arbitration_wait<=kEligibleWriteArbitrationWaitBound&&
        !metrics.dropped_tokens&&!metrics.duplicate_tokens&&!metrics.stale_side_effects&&
        !metrics.terminal_duplicate_classifications&&!metrics.terminal_missing_records&&
        !metrics.unexplained_tokens?0:1;
}
