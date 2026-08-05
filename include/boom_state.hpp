#ifndef BOOM_STATE_HPP
#define BOOM_STATE_HPP

#include "boom_config.hpp"
#include "boom_types.hpp"
#include "divider.hpp"

struct FrontendState {
    uint64_t pc;
    bool     reset_done;
    bool     request_sent;
    uint32_t fetch_id;
    uint32_t pending_fetch_id;
    bool     response_received;
    uint64_t resp_address;
    uint32_t resp_instruction;
    bool     resp_exception;
    uint64_t resp_exc_cause;
    bool     stalled;
    bool     flush;
    bool     fetch_packet_valid;
    MicroOp  fetch_uop;

    FrontendState() : pc(RESET_VECTOR), reset_done(false), request_sent(false),
        fetch_id(0), pending_fetch_id(0), response_received(false),
        resp_address(0), resp_instruction(0), resp_exception(false), resp_exc_cause(0),
        stalled(false), flush(false), fetch_packet_valid(false), fetch_uop() {}
};

struct DecodeState {
    MicroOp dec_uops[DISPATCH_WIDTH];
    bool    dec_valids[DISPATCH_WIDTH];
    DecodeState() { for (int i=0; i<DISPATCH_WIDTH; i++) { dec_uops[i]=MicroOp(); dec_valids[i]=false; } }
};

struct RenameMapTableState {
    uint8_t map_table[LOGICAL_REG_COUNT];
    uint8_t committed_map_table[LOGICAL_REG_COUNT];
    uint8_t br_snapshots[LOGICAL_REG_COUNT][MAX_BRANCH_COUNT];
    RenameMapTableState() { for (int i=0; i<LOGICAL_REG_COUNT; i++) {
        map_table[i]=0; committed_map_table[i]=0;
        for (int j=0; j<MAX_BRANCH_COUNT; j++) br_snapshots[i][j]=0; }}
};

struct RenameFreeListState {
    uint8_t free_list[INT_PHYS_REGS];
    uint8_t head, tail, count;
    bool    busy_table[INT_PHYS_REGS];
    RenameFreeListState() : head(1), tail(0), count(INT_PHYS_REGS-1) {
        for (int i=0; i<INT_PHYS_REGS; i++) { free_list[i]=i; busy_table[i]=false; }
        free_list[0]=0; busy_table[0]=false; }
};

struct BranchRecoveryState {
    uint8_t active_mask;
    bool    tag_valid[MAX_BRANCH_COUNT];
    bool    snapshot_valid[MAX_BRANCH_COUNT];
    bool    br_alloc_lists[MAX_BRANCH_COUNT][INT_PHYS_REGS];
    uint32_t allocations, releases, mispredicts, rollbacks;

    BranchRecoveryState() : active_mask(0), allocations(0), releases(0),
        mispredicts(0), rollbacks(0) {
        for (int i=0; i<MAX_BRANCH_COUNT; i++) {
            tag_valid[i]=false; snapshot_valid[i]=false;
            for (int p=0; p<INT_PHYS_REGS; p++) br_alloc_lists[i][p]=false;
        }
    }
};

struct RenameDispatchPacket {
    bool valid;
    MicroOp uop;
    bool rob_allocated;
    RenameDispatchPacket() : valid(false), uop(), rob_allocated(false) {}
};

struct RenameState {
    RenameMapTableState int_map_table, fp_map_table;
    RenameFreeListState int_free_list, fp_free_list;
    RenameDispatchPacket dispatch_packets[DISPATCH_WIDTH];
};

struct IssueQueueState {
    IssueSlotEntry entries[ISSUE_QUEUE_ALU_DEPTH];
    uint8_t head, tail, count;
    IssueQueueState() : head(0), tail(0), count(0) {
        for (int i=0; i<ISSUE_QUEUE_ALU_DEPTH; i++) entries[i]=IssueSlotEntry(); }
};

struct IssueState {
    IssueQueueState alu_iq;
    IssueGrant grants[ISSUE_WIDTH];
    MicroOp issued_uops[ISSUE_WIDTH];
    bool    issued_valids[ISSUE_WIDTH];
    uint64_t issued_prs1_data[ISSUE_WIDTH], issued_prs2_data[ISSUE_WIDTH];
    uint64_t issued_prs3_data[ISSUE_WIDTH];
    bool    port_ready[ISSUE_WIDTH];
    uint8_t grants_generated, grants_accepted, grants_retained, grants_dropped;
    uint64_t cycles_with_0_grant, cycles_with_1_grant, cycles_with_2_grants;
    uint64_t total_grants, mem_grants, int_grants, grant_stalls;
    uint64_t execute_acceptance_stalls, dropped_grants;
    IssueState() : grants_generated(0), grants_accepted(0), grants_retained(0), grants_dropped(0),
        cycles_with_0_grant(0), cycles_with_1_grant(0), cycles_with_2_grants(0),
        total_grants(0), mem_grants(0), int_grants(0), grant_stalls(0),
        execute_acceptance_stalls(0), dropped_grants(0) {
        for (int i=0; i<ISSUE_WIDTH; i++) {
            grants[i]=IssueGrant(); issued_uops[i]=MicroOp(); issued_valids[i]=false;
            issued_prs1_data[i]=issued_prs2_data[i]=issued_prs3_data[i]=0;
            port_ready[i]=(i != FP_ISSUE_LANE);
        }
    }
};

struct DividerExecutionState {
    boom::DividerState arithmetic;
    bool token_valid;
    MicroOp uop;
    uint8_t rob_idx;
    uint8_t pdst;
    uint32_t allocation_id;
    uint8_t branch_mask;

    DividerExecutionState() : arithmetic(), token_valid(false), uop(), rob_idx(0),
        pdst(0), allocation_id(0), branch_mask(0) {}
};

struct ExecuteState {
    struct AluResult {
        bool valid; MicroOp uop; uint64_t result;
        bool exception; uint64_t exc_cause;
        bool mispredict; uint64_t redirect_pc;
        bool memory_valid, is_load, is_store, signed_load;
        uint64_t memory_address, store_data;
        uint8_t memory_mask, memory_size;
        AluResult() : valid(false), uop(), result(0), exception(false),
            exc_cause(0), mispredict(false), redirect_pc(0), memory_valid(false),
            is_load(false), is_store(false), signed_load(false), memory_address(0),
            store_data(0), memory_mask(0), memory_size(0) {}
    };
    AluResult alu_results[EXECUTE_RESULT_LANES];
    DividerExecutionState divider;
    ExecuteState() { for (int i=0; i<EXECUTE_RESULT_LANES; i++) alu_results[i]=AluResult(); }
};

struct RobInternalState {
    RobEntry entries[ROB_DEPTH];
    uint8_t  head, tail;
    uint32_t next_allocation_id;
    bool     maybe_full;
    RobState state;
    uint8_t  commit_count;
    CommitEntry last_commit;
    bool     commit_valid;

    RobInternalState() : head(0), tail(0), next_allocation_id(1), maybe_full(false), state(ROB_INIT),
        commit_count(0), last_commit(), commit_valid(false) {
        for (int i=0; i<ROB_DEPTH; i++) entries[i]=RobEntry(); }
};

struct CsrState {
    uint64_t mstatus, misa, mie, mtvec, mscratch, mepc, mcause, mtval, mip, cycle, instret, satp;
    uint8_t  priv;
    CsrState() : mstatus(0x0000000a00000000ull), misa(0x800000000014112dull),
        mie(0), mtvec(0), mscratch(0), mepc(0), mcause(0), mtval(0), mip(0),
        cycle(0), instret(0), satp(0), priv(PRV_M) {}
};

struct StoreQueueEntry {
    bool valid, address_valid, data_valid, committed, issued_to_memory, completed, killed;
    uint8_t rob_idx, mask, size, branch_mask;
    uint32_t rob_allocation_id;
    uint64_t address, data;
    StoreQueueEntry() : valid(false), address_valid(false), data_valid(false), committed(false),
        issued_to_memory(false), completed(false), killed(false), rob_idx(0), mask(0),
        size(0), branch_mask(0), rob_allocation_id(0), address(0), data(0) {}
};

struct LoadQueueEntry {
    bool valid, signed_load, response_pending, completed, killed;
    uint8_t rob_idx, size, branch_mask;
    uint32_t transaction_id, rob_allocation_id;
    uint64_t address, result;
    LoadQueueEntry() : valid(false), signed_load(false), response_pending(false),
        completed(false), killed(false), rob_idx(0), size(0), branch_mask(0),
        transaction_id(0), rob_allocation_id(0), address(0), result(0) {}
};

struct LsuState {
    uint8_t ldq_head, ldq_tail, ldq_count, stq_head, stq_tail, stq_count;
    StoreQueueEntry stq[STQ_DEPTH];
    LoadQueueEntry ldq[LDQ_DEPTH];
    uint32_t next_transaction_id;
    bool load_response_pending;
    uint32_t pending_load_transaction_id;
    uint8_t pending_load_rob_idx;
    uint32_t pending_load_allocation_id;
    LsuState() : ldq_head(0), ldq_tail(0), ldq_count(0), stq_head(0), stq_tail(0), stq_count(0),
        next_transaction_id(1), load_response_pending(false), pending_load_transaction_id(0),
        pending_load_rob_idx(0), pending_load_allocation_id(0) {}
};

struct CompletionPendingState {
    RobCompleteEvent load_response;
    RobCompleteEvent mem_execute;
    RobCompleteEvent int_execute;
    WritebackEvent writebacks[2];
    WakeupEvent wakeups[NUM_INT_WAKEUP_PORTS];
    BypassEvent bypass[NUM_INT_BYPASS_PORTS];
    bool wakeup_sent[COMPLETION_PENDING_SLOTS];
    uint8_t completion_accepts_this_cycle, rob_completes_this_cycle;
    uint8_t prf_writes_this_cycle, wakeups_this_cycle;
    uint8_t bypass_this_cycle;
    uint8_t peak_completion_accepts, peak_rob_completes, peak_prf_writes;
    uint8_t peak_wakeups, peak_bypass;
    uint64_t total_completion_accepts, total_rob_completes, total_prf_writes;
    uint64_t total_wakeups, total_bypass;
    bool writeback_conflict;
    bool writeback_fault_valid;
    bool validation_fault_this_cycle;
    uint8_t writeback_fault_pdst, writeback_fault_rob_idx;
    uint32_t writeback_fault_allocation_id;
    uint64_t writeback_fault_cause;
    uint64_t writeback_conflicts, writeback_validation_faults;
    uint64_t writeback_fault_events, writeback_deduplications;
    uint64_t dropped_completions, dropped_writebacks, duplicate_writebacks;
    uint64_t wakeup_conflicts, bypass_conflicts;
    CompletionPendingState() : load_response(), mem_execute(), int_execute(),
        completion_accepts_this_cycle(0), rob_completes_this_cycle(0), prf_writes_this_cycle(0),
        wakeups_this_cycle(0), bypass_this_cycle(0), peak_completion_accepts(0),
        peak_rob_completes(0), peak_prf_writes(0), peak_wakeups(0), peak_bypass(0),
        total_completion_accepts(0), total_rob_completes(0), total_prf_writes(0), total_wakeups(0),
        total_bypass(0), writeback_conflict(false), writeback_fault_valid(false),
        validation_fault_this_cycle(false),
        writeback_fault_pdst(0), writeback_fault_rob_idx(0),
        writeback_fault_allocation_id(0), writeback_fault_cause(0),
        writeback_conflicts(0), writeback_validation_faults(0),
        writeback_fault_events(0), writeback_deduplications(0),
        dropped_completions(0), dropped_writebacks(0), duplicate_writebacks(0),
        wakeup_conflicts(0), bypass_conflicts(0) {
        for (int i=0; i<NUM_INT_WRITEBACK_PORTS; i++) writebacks[i]=WritebackEvent();
        for (int i=0; i<COMPLETION_PENDING_SLOTS; i++) wakeup_sent[i]=false;
        for (int i=0; i<NUM_INT_WAKEUP_PORTS; i++) wakeups[i]=WakeupEvent();
        for (int i=0; i<NUM_INT_BYPASS_PORTS; i++) bypass[i]=BypassEvent();
    }
};

struct BoomCoreState {
    uint64_t        cycle_count;
    FrontendState   frontend;
    DecodeState     decode;
    RenameState     rename;
    IssueState      issue;
    ExecuteState    execute;
    RobInternalState rob;
    CsrState        csr;
    LsuState        lsu;
    CompletionPendingState completion;
    BranchRecoveryState branch_state;
    uint64_t        int_rf_bank0[INT_PHYS_REGS];
    uint64_t        int_rf_bank1[INT_PHYS_REGS];
    uint64_t        int_rf_latest_bank;
    uint64_t        fp_rf[FP_PHYS_REGS];
    BranchUpdate    brupdate;
    bool            global_flush;
    bool            io_success;
    bool            io_halted;
    bool            io_trap;
    uint64_t        tohost;

    BoomCoreState() : cycle_count(0), int_rf_latest_bank(0), brupdate(), global_flush(false),
        io_success(false), io_halted(false), io_trap(false), tohost(0) {
        for (int i=0; i<INT_PHYS_REGS; i++) {
            int_rf_bank0[i]=0;
            int_rf_bank1[i]=0;
        }
        for (int i=0; i<FP_PHYS_REGS; i++) fp_rf[i]=0;
        int_rf_bank0[0]=0; int_rf_bank1[0]=0; fp_rf[0]=0;
    }
};

namespace boom {

inline uint64_t prf_read(const BoomCoreState& state, uint8_t pdst) {
#pragma HLS INLINE
    if (pdst == 0 || pdst >= INT_PHYS_REGS) return 0;
    return ((state.int_rf_latest_bank >> pdst) & 1ULL) ?
        state.int_rf_bank1[pdst] : state.int_rf_bank0[pdst];
}

inline void prf_seed(BoomCoreState& state, uint8_t pdst, uint64_t value) {
#pragma HLS INLINE
    if (pdst == 0 || pdst >= INT_PHYS_REGS) return;
    state.int_rf_bank0[pdst] = value;
    state.int_rf_bank1[pdst] = value;
    state.int_rf_latest_bank &= ~(1ULL << pdst);
}

inline void prf_force_x0(BoomCoreState& state) {
#pragma HLS INLINE
    state.int_rf_bank0[0] = 0;
    state.int_rf_bank1[0] = 0;
    state.int_rf_latest_bank &= ~1ULL;
}

}

#endif
