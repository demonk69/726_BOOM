#include "boom_config.hpp"
#include "boom_types.hpp"
#include "boom_state.hpp"
#include "boom_interfaces.hpp"

namespace boom {

extern bool rob_branch_kill(BoomCoreState& state);
extern void rob_complete(BoomCoreState& state);
extern bool lsu_reclaim_store(BoomCoreState& state, SqIndex sq_index,
                              uint16_t generation, uint8_t rob_idx,
                              uint32_t allocation_id);
extern bool lsu_store_owner_matches(const BoomCoreState& state, SqIndex sq_index,
                                    uint16_t generation, uint8_t rob_idx,
                                    uint32_t allocation_id);

static bool preg_is_committed(const RenameMapTableState& mt, uint8_t preg) {
    if (preg == 0) return true;
    for (int i=0; i<LOGICAL_REG_COUNT; i++)
        if (mt.committed_map_table[i] == preg) return true;
    return false;
}

static void restore_committed_rename(BoomCoreState& state) {
    RenameMapTableState& mt = state.rename.int_map_table;
    RenameFreeListState& fl = state.rename.int_free_list;
    for (int i=0; i<LOGICAL_REG_COUNT; i++) mt.map_table[i] = mt.committed_map_table[i];
    mt.map_table[0] = 0;

    uint8_t count = 0;
    for (int p=0; p<INT_PHYS_REGS; p++) fl.busy_table[p] = false;
    for (int p=1; p<INT_PHYS_REGS; p++)
        if (!preg_is_committed(mt, (uint8_t)p)) fl.free_list[count++] = (uint8_t)p;
    for (int p=count; p<INT_PHYS_REGS; p++) fl.free_list[p] = 0;
    fl.head = 0;
    fl.tail = (uint8_t)(count % INT_PHYS_REGS);
    fl.count = count;
}

static void clear_speculative_state(BoomCoreState& state) {
    const uint32_t next_allocation_id = state.rob.next_allocation_id;
    const uint32_t next_transaction_id = state.lsu.next_transaction_id;
    uint16_t lq_generations[LQ_DEPTH];
    uint16_t sq_generations[SQ_DEPTH];
    for (int i=0; i<LQ_DEPTH; i++) lq_generations[i] = state.lsu.ldq[i].generation;
    for (int i=0; i<SQ_DEPTH; i++) sq_generations[i] = state.lsu.stq[i].generation;
    for (int i=0; i<ROB_DEPTH; i++) state.rob.entries[i] = RobEntry();
    state.rob.head = 0;
    state.rob.tail = 0;
    state.rob.maybe_full = false;
    state.rob.state = ROB_NORMAL;
    state.rob.next_allocation_id = next_allocation_id;

    state.decode = DecodeState();
    for (int i=0; i<DISPATCH_WIDTH; i++)
        state.rename.dispatch_packets[i] = RenameDispatchPacket();
    state.issue.alu_iq = IssueQueueState();
    for (int i=0; i<ISSUE_WIDTH; i++) {
        state.issue.grants[i] = IssueGrant();
        state.issue.issued_valids[i] = false;
        state.issue.issued_uops[i] = MicroOp();
    }
    for (int i=0; i<EXECUTE_RESULT_LANES; i++)
        state.execute.alu_results[i] = ExecuteState::AluResult();
    divider_reset(state.execute.divider.arithmetic);
    state.execute.divider = DividerExecutionState();
    state.completion = CompletionPendingState();
    state.lsu = LsuState();
    state.lsu.next_transaction_id = next_transaction_id;
    for (int i=0; i<LQ_DEPTH; i++) state.lsu.ldq[i].generation = lq_generations[i];
    for (int i=0; i<SQ_DEPTH; i++) state.lsu.stq[i].generation = sq_generations[i];

    state.branch_state.active_mask = 0;
    for (int t=0; t<MAX_BRANCH_COUNT; t++) {
        state.branch_state.tag_valid[t] = false;
        state.branch_state.snapshot_valid[t] = false;
        for (int p=0; p<INT_PHYS_REGS; p++)
            state.branch_state.br_alloc_lists[t][p] = false;
    }
    restore_committed_rename(state);
    state.brupdate = BranchUpdate();
    state.global_flush = true;
}

static uint64_t exception_tval(const RobEntry& entry) {
    const MicroOp& uop = entry.uop;
    if (uop.exc_cause == 0 || uop.exc_cause == 1 ||
        uop.exc.xcpt_ma_if || uop.exc.xcpt_ae_if) return uop.debug_pc;
    if (uop.exc_cause == 2)
        return uop.is_rvc ? (uint64_t)(uop.debug_inst & 0xffffu) : (uint64_t)uop.inst;
    if (entry.memory_valid) return entry.memory_address;
    return 0;
}

void exception_recovery_apply(BoomCoreState& state, const RobEntry& owner) {
    const uint64_t target = BOOM_TRAP_VECTOR & ~0x3ULL;
    const uint64_t mie = (state.csr.mstatus >> 3) & 1ULL;
    state.csr.mepc = owner.uop.debug_pc;
    state.csr.mcause = owner.uop.exc_cause;
    state.csr.mtval = exception_tval(owner);
    state.csr.mstatus &= ~((1ULL << 3) | (1ULL << 7) | (3ULL << 11));
    state.csr.mstatus |= (mie << 7) | ((uint64_t)state.csr.priv << 11);
    state.csr.priv = PRV_M;

    state.exception_commit.valid = true;
    state.exception_commit.cause = state.csr.mcause;
    state.exception_commit.pc = state.csr.mepc;
    state.exception_commit.tval = state.csr.mtval;
    state.exception_commit.target = target;
    state.frontend_redirect.valid = true;
    state.frontend_redirect.target_pc = target;
    state.frontend_redirect.cause = FRONTEND_REDIRECT_EXCEPTION;
    state.frontend_redirect.rob_idx = owner.uop.queue.rob_idx;
    state.frontend_redirect.allocation_id = owner.uop.queue.rob_allocation_id;
    state.frontend_redirect.branch_mask = owner.uop.branch.br_mask;
    if (owner.uop.ftq_valid) {
        state.ftq_redirect_pending.valid = true;
        state.ftq_redirect_pending.owner_ftq_idx = owner.uop.ftq_idx;
        state.ftq_redirect_pending.owner_generation = owner.uop.ftq_generation;
        state.ftq_redirect_pending.surviving_lane_mask = static_cast<uint8_t>(
            1u << owner.uop.ftq_lane);
        state.ftq_exception_retire_deferred.valid = true;
        state.ftq_exception_retire_deferred.ftq_idx = owner.uop.ftq_idx;
        state.ftq_exception_retire_deferred.generation = owner.uop.ftq_generation;
        state.ftq_exception_retire_deferred.lane = owner.uop.ftq_lane;
    }
    clear_speculative_state(state);
}

static bool free_list_contains(const RenameFreeListState& fl, uint8_t preg) {
    uint8_t idx = fl.head;
    for (int i=0; i<fl.count && i<INT_PHYS_REGS; i++) {
        if (fl.free_list[idx] == preg) return true;
        idx = (uint8_t)((idx + 1) % INT_PHYS_REGS);
    }
    return false;
}

static void free_preg_unique(RenameFreeListState& fl, uint8_t preg) {
    if (preg == 0 || preg >= INT_PHYS_REGS) return;
    if (free_list_contains(fl, preg)) return;
    if (fl.count >= INT_PHYS_REGS) return;
    fl.busy_table[preg] = false;
    fl.free_list[fl.tail] = preg;
    fl.tail = (uint8_t)((fl.tail + 1) % INT_PHYS_REGS);
    fl.count++;
}

bool rob_commit_prepare_bim_update(const BoomCoreState& state,
                                   const RobEntry& entry,
                                   uint32_t ftq_generation,
                                   PredictorUpdate& update) {
    const MicroOp& uop = entry.uop;
    if (!uop.branch.is_br || !entry.branch_resolved || !uop.ftq_valid)
        return false;
    const FtqPredictionLookup metadata = state.ftq.lookup_prediction(
        uop.ftq_idx, ftq_generation, uop.ftq_lane,
        boom::CFI_CONDITIONAL_BRANCH);
    const uint16_t pc_index = static_cast<uint16_t>(
        (uop.debug_pc >> 1) & 255u);
    if (!metadata.reference_valid || !metadata.cfi_match ||
        metadata.predictor_generation != state.predictor_generation ||
        metadata.predictor_metadata_index != pc_index)
        return false;

    update.valid = true;
    update.commit_qualified = true;
    update.cfi_type = boom::CFI_CONDITIONAL_BRANCH;
    update.pc = uop.debug_pc;
    update.metadata_token = metadata.predictor_metadata_index;
    update.taken = entry.branch_actual_taken;
    update.generation = metadata.predictor_generation;
    return true;
}

void rob_commit_module(BoomCoreState& state, PipeSignals& pipe) {
    RobInternalState& rob = state.rob;
    rob.commit_valid = false;
    state.exception_commit.valid = false;
    state.bim_training.attempted_this_cycle = false;
    state.bim_training.accepted_this_cycle = false;
    state.bim_training.stale_rejected_this_cycle = false;

    if (rob_branch_kill(state)) return;

    bool is_empty = (rob.head==rob.tail) && !rob.maybe_full;
    if (!is_empty && rob.entries[rob.head].valid && !rob.entries[rob.head].busy) {
        RobEntry& he = rob.entries[rob.head];
        MicroOp& uop = he.uop;
        if (he.exception) {
            if (uop.uopc == 65) {
                uint8_t a0_pdst = state.rename.int_map_table.committed_map_table[10];
                uint64_t a0_val = prf_read(state, a0_pdst);
                if (a0_val==0) state.io_success = true;
                else state.io_trap = true;
                clear_speculative_state(state);
            } else {
                if (pipe.commit_trace.full()) return;
                CommitEntry ce;
                ce.valid = true;
                ce.pc = uop.debug_pc;
                ce.inst = uop.inst;
                ce.priv = state.csr.priv;
                ce.exception = true;
                ce.exc_cause = uop.exc_cause;
                pipe.commit_trace.write(ce);
                rob.last_commit = ce;
                rob.commit_valid = true;
                RobEntry owner = he;
#ifdef __SYNTHESIS__
                owner.uop.ftq_generation = rob.ftq_generations[rob.head];
#endif
                exception_recovery_apply(state, owner);
            }
        } else {
            if ((uop.ctrl.is_load || he.is_load) && !he.memory_completed) return;
            if (pipe.commit_trace.full()) return;
            uint32_t ftq_generation = uop.ftq_generation;
#ifdef __SYNTHESIS__
            ftq_generation = rob.ftq_generations[rob.head];
#endif
            PredictorUpdate training_update;
            const bool training_eligible = rob_commit_prepare_bim_update(
                state, he, ftq_generation, training_update);
            if (training_eligible && state.predictor_update_pending.valid) return;
            if (uop.ctrl.is_sta || he.is_store) {
                if (!he.memory_valid) return;
                if (!he.memory_request_sent) {
                    if (uop.queue.stq_idx >= SQ_DEPTH ||
                        !lsu_store_owner_matches(state, (SqIndex)uop.queue.stq_idx,
                                                 uop.queue.stq_generation,
                                                 uop.queue.rob_idx,
                                                 uop.queue.rob_allocation_id)) return;
                    if (pipe.dmem_req.full()) return;
                    DmemRequest req;
                    req.transaction_id = state.lsu.next_transaction_id++;
                    req.rob_idx = uop.queue.rob_idx;
                    req.command = DMEM_STORE;
                    req.is_store = true;
                    req.committed = true;
                    req.address = he.memory_address;
                    req.data = he.memory_data;
                    req.write_data = he.memory_data;
                    req.size = he.memory_size;
                    req.mask = he.memory_mask;
                    req.write_mask = he.memory_mask;
                    req.branch_mask = uop.branch.br_mask;
                    pipe.dmem_req.write(req);
                    he.memory_request_sent = true;
                    he.memory_completed = true;
                    state.tohost = he.memory_data;
                    lsu_reclaim_store(state, (SqIndex)uop.queue.stq_idx,
                                      uop.queue.stq_generation, uop.queue.rob_idx,
                                      uop.queue.rob_allocation_id);
                }
            }
            state.csr.instret++;
            CommitEntry ce;
            ce.valid=true; ce.pc=uop.debug_pc; ce.inst=uop.inst;
            ce.rd_valid=(uop.rename.dst_rtype==DST_INT && uop.rename.pdst!=0);
            ce.rd=uop.rename.ldst; ce.priv=state.csr.priv;
            ce.exception=false; ce.branch_mispredict=false;
            ce.rd_value=ce.rd_valid ? prf_read(state, uop.rename.pdst) : 0;
            ce.memory_valid = he.memory_valid;
            ce.is_store = he.is_store;
            ce.memory_address = he.memory_address;
            ce.memory_data = he.memory_data;
            ce.memory_mask = he.memory_mask;
            ce.store_addr = he.memory_address;
            ce.store_data = he.memory_data;
            ce.store_mask = he.memory_mask;
            if (uop.rename.pdst!=0) {
                state.rename.int_map_table.committed_map_table[uop.rename.ldst]=uop.rename.pdst;
                if (uop.rename.stale_pdst!=0 && uop.rename.stale_pdst<INT_PHYS_REGS) {
                    RenameFreeListState& fl=state.rename.int_free_list;
                    free_preg_unique(fl, uop.rename.stale_pdst);
                }
            }
            pipe.commit_trace.write(ce);
            rob.last_commit=ce; rob.commit_valid=true;
            if (uop.branch.is_br) {
                state.bim_training.attempted_this_cycle = true;
                state.bim_training.attempts++;
                if (!he.branch_resolved) {
                    state.bim_training.dropped++;
                } else if (!training_eligible) {
                    state.bim_training.stale_rejected_this_cycle = true;
                    state.bim_training.stale_rejected++;
                }
            }
            if (training_eligible) {
                state.predictor_update_pending = training_update;
                state.bim_training.accepted_this_cycle = true;
                state.bim_training.accepted++;
            }
            if (uop.ftq_valid) {
                state.ftq_retire_pending.valid = true;
                state.ftq_retire_pending.ftq_idx = uop.ftq_idx;
                state.ftq_retire_pending.generation = ftq_generation;
                state.ftq_retire_pending.lane = uop.ftq_lane;
            }
            he.valid=false; rob.head=(rob.head+1)%ROB_DEPTH; rob.maybe_full=false;
        }
    }
}

void commit_module_stub(BoomCoreState& state) { (void)state; }
}
