#include "boom_config.hpp"
#include "boom_types.hpp"
#include "boom_state.hpp"

namespace boom {

static uint8_t branch_tag_bit(uint8_t tag) {
    return (tag < MAX_BRANCH_COUNT) ? (uint8_t)(1u << tag) : 0;
}

static bool branch_is_control_uop(const MicroOp& uop) {
    return uop.branch.is_br || uop.branch.is_jal || uop.branch.is_jalr;
}

static bool killed_by_mask(const MicroOp& uop, uint8_t mask) {
    return (uop.branch.br_mask & mask) != 0;
}

static void clear_resolved_mask(MicroOp& uop, uint8_t resolve_mask) {
    uop.branch.br_mask &= (uint8_t)~resolve_mask;
}

static bool branch_free_list_contains(const RenameFreeListState& fl, uint8_t preg) {
    uint8_t idx = fl.head;
    for (int i=0; i<fl.count && i<INT_PHYS_REGS; i++) {
        if (fl.free_list[idx] == preg) return true;
        idx = (uint8_t)((idx + 1) % INT_PHYS_REGS);
    }
    return false;
}

static bool preg_in_map(const RenameMapTableState& mt, uint8_t preg) {
    if (preg == 0) return true;
    for (int i=0; i<LOGICAL_REG_COUNT; i++) if (mt.map_table[i] == preg) return true;
    return false;
}

static void branch_free_preg_unique(RenameFreeListState& fl, uint8_t preg) {
    if (preg == 0 || preg >= INT_PHYS_REGS) return;
    if (branch_free_list_contains(fl, preg)) return;
    if (fl.count >= INT_PHYS_REGS) return;
    fl.free_list[fl.tail] = preg;
    fl.tail = (uint8_t)((fl.tail + 1) % INT_PHYS_REGS);
    fl.count++;
}

static void branch_clear_alloc_list(BranchRecoveryState& br, uint8_t tag) {
    if (tag >= MAX_BRANCH_COUNT) return;
    for (int p=0; p<INT_PHYS_REGS; p++) br.br_alloc_lists[tag][p] = false;
}

static void release_tag(BoomCoreState& state, uint8_t tag) {
    if (tag >= MAX_BRANCH_COUNT) return;
    uint8_t bit = branch_tag_bit(tag);
    state.branch_state.active_mask &= (uint8_t)~bit;
    state.branch_state.tag_valid[tag] = false;
    state.branch_state.snapshot_valid[tag] = false;
    branch_clear_alloc_list(state.branch_state, tag);
    state.branch_state.releases++;
}

static void clear_resolved_masks_in_state(BoomCoreState& state, uint8_t resolve_mask) {
    if (resolve_mask == 0) return;
    for (int i=0; i<DISPATCH_WIDTH; i++) {
        clear_resolved_mask(state.decode.dec_uops[i], resolve_mask);
        clear_resolved_mask(state.rename.renamed_uops[i], resolve_mask);
    }
    for (int i=0; i<EXECUTE_RESULT_LANES; i++) clear_resolved_mask(state.execute.alu_results[i].uop, resolve_mask);
    for (int i=0; i<ISSUE_WIDTH; i++) clear_resolved_mask(state.issue.issued_uops[i], resolve_mask);
    for (int i=0; i<ISSUE_QUEUE_ALU_DEPTH; i++) clear_resolved_mask(state.issue.alu_iq.entries[i].uop, resolve_mask);
    for (int i=0; i<ROB_DEPTH; i++) clear_resolved_mask(state.rob.entries[i].uop, resolve_mask);
    for (int i=0; i<LDQ_DEPTH; i++) state.lsu.ldq[i].branch_mask &= (uint8_t)~resolve_mask;
    for (int i=0; i<STQ_DEPTH; i++) state.lsu.stq[i].branch_mask &= (uint8_t)~resolve_mask;
}

static void compact_issue_queue(IssueQueueState& iq) {
    IssueSlotEntry temp[ISSUE_QUEUE_ALU_DEPTH];
    int w = 0;
    for (int i=0; i<ISSUE_QUEUE_ALU_DEPTH; i++) {
        if (iq.entries[i].valid && !iq.entries[i].killed) temp[w++] = iq.entries[i];
    }
    for (int i=w; i<ISSUE_QUEUE_ALU_DEPTH; i++) temp[i] = IssueSlotEntry();
    for (int i=0; i<ISSUE_QUEUE_ALU_DEPTH; i++) iq.entries[i] = temp[i];
    iq.head = 0;
    iq.tail = (uint8_t)(w % ISSUE_QUEUE_ALU_DEPTH);
    iq.count = (uint8_t)w;
}

static void kill_issue_state(BoomCoreState& state, uint8_t mispredict_mask) {
    for (int i=0; i<ISSUE_QUEUE_ALU_DEPTH; i++) {
        IssueSlotEntry& slot = state.issue.alu_iq.entries[i];
        if (slot.valid && killed_by_mask(slot.uop, mispredict_mask)) {
            slot.valid = false;
            slot.killed = true;
        }
    }
    compact_issue_queue(state.issue.alu_iq);
    for (int i=0; i<ISSUE_WIDTH; i++) {
        if (state.issue.issued_valids[i] && killed_by_mask(state.issue.issued_uops[i], mispredict_mask)) {
            state.issue.issued_valids[i] = false;
            state.issue.issued_uops[i] = MicroOp();
        }
    }
}

static void kill_execute_state(BoomCoreState& state, uint8_t mispredict_mask) {
    for (int i=0; i<EXECUTE_RESULT_LANES; i++) {
        if (state.execute.alu_results[i].valid && killed_by_mask(state.execute.alu_results[i].uop, mispredict_mask)) {
            state.execute.alu_results[i] = ExecuteState::AluResult();
        }
    }
}

static void kill_lsu_state(BoomCoreState& state, uint8_t mispredict_mask) {
    LsuState& lsu = state.lsu;
    LoadQueueEntry new_ldq[LDQ_DEPTH];
    StoreQueueEntry new_stq[STQ_DEPTH];
    int lw = 0;
    int sw = 0;

    for (int i=0; i<LDQ_DEPTH; i++) {
        LoadQueueEntry& e = lsu.ldq[i];
        if (e.valid && (e.branch_mask & mispredict_mask) != 0) {
            if (lsu.load_response_pending && lsu.pending_load_rob_idx == e.rob_idx) {
                lsu.load_response_pending = false;
                lsu.pending_load_transaction_id = 0;
                lsu.pending_load_rob_idx = 0;
            }
            e = LoadQueueEntry();
        } else if (e.valid) {
            new_ldq[lw++] = e;
        }
    }
    for (int i=0; i<STQ_DEPTH; i++) {
        StoreQueueEntry& e = lsu.stq[i];
        if (e.valid && (e.branch_mask & mispredict_mask) != 0) {
            e = StoreQueueEntry();
        } else if (e.valid) {
            new_stq[sw++] = e;
        }
    }
    for (int i=lw; i<LDQ_DEPTH; i++) new_ldq[i] = LoadQueueEntry();
    for (int i=sw; i<STQ_DEPTH; i++) new_stq[i] = StoreQueueEntry();
    for (int i=0; i<LDQ_DEPTH; i++) lsu.ldq[i] = new_ldq[i];
    for (int i=0; i<STQ_DEPTH; i++) lsu.stq[i] = new_stq[i];
    lsu.ldq_head = 0;
    lsu.ldq_tail = (uint8_t)(lw % LDQ_DEPTH);
    lsu.ldq_count = (uint8_t)lw;
    lsu.stq_head = 0;
    lsu.stq_tail = (uint8_t)(sw % STQ_DEPTH);
    lsu.stq_count = (uint8_t)sw;
}

static void kill_rob_younger_than(BoomCoreState& state, uint8_t branch_rob_idx, uint8_t mispredict_mask) {
    RobInternalState& rob = state.rob;
    bool found_branch = false;
    uint8_t idx = rob.head;
    for (int i=0; i<ROB_DEPTH; i++) {
        RobEntry& entry = rob.entries[idx];
        if (entry.valid && idx == branch_rob_idx) {
            found_branch = true;
        } else if (found_branch && entry.valid) {
            entry = RobEntry();
        } else if (!found_branch && entry.valid && killed_by_mask(entry.uop, mispredict_mask)) {
            entry = RobEntry();
        }
        idx = (uint8_t)((idx + 1) % ROB_DEPTH);
    }
    if (found_branch) rob.tail = (uint8_t)((branch_rob_idx + 1) % ROB_DEPTH);
    rob.maybe_full = false;
}

static void rebuild_busy_after_recovery(BoomCoreState& state) {
    RenameFreeListState& fl = state.rename.int_free_list;
    for (int p=0; p<INT_PHYS_REGS; p++) fl.busy_table[p] = false;
    for (int i=0; i<ROB_DEPTH; i++) {
        const RobEntry& entry = state.rob.entries[i];
        uint8_t pdst = entry.uop.rename.pdst;
        if (entry.valid && entry.busy && pdst != 0 && pdst < INT_PHYS_REGS) fl.busy_table[pdst] = true;
    }
}

static void restore_map_snapshot(BoomCoreState& state, uint8_t tag) {
    RenameMapTableState& mt = state.rename.int_map_table;
    if (tag < MAX_BRANCH_COUNT && state.branch_state.snapshot_valid[tag]) {
        for (int i=0; i<LOGICAL_REG_COUNT; i++) mt.map_table[i] = mt.br_snapshots[i][tag];
    } else {
        for (int i=0; i<LOGICAL_REG_COUNT; i++) mt.map_table[i] = mt.committed_map_table[i];
    }
    mt.map_table[0] = 0;
}

static void rollback_free_list(BoomCoreState& state, uint8_t tag) {
    if (tag >= MAX_BRANCH_COUNT) return;
    RenameFreeListState& fl = state.rename.int_free_list;
    BranchRecoveryState& br = state.branch_state;

    for (int p=1; p<INT_PHYS_REGS; p++) {
        if (br.br_alloc_lists[tag][p] && !preg_in_map(state.rename.int_map_table, (uint8_t)p)) {
            fl.busy_table[p] = false;
            branch_free_preg_unique(fl, (uint8_t)p);
        }
    }
    for (int t=0; t<MAX_BRANCH_COUNT; t++) {
        if (!br.tag_valid[t]) continue;
        for (int p=1; p<INT_PHYS_REGS; p++) {
            if (br.br_alloc_lists[tag][p]) br.br_alloc_lists[t][p] = false;
        }
    }
}

static void prune_recovered_tags(BoomCoreState& state, uint8_t keep_mask, uint8_t resolved_tag) {
    BranchRecoveryState& br = state.branch_state;
    for (int t=0; t<MAX_BRANCH_COUNT; t++) {
        bool keep = (keep_mask & branch_tag_bit((uint8_t)t)) != 0;
        if (!keep || t == resolved_tag) {
            br.tag_valid[t] = false;
            br.snapshot_valid[t] = false;
            branch_clear_alloc_list(br, (uint8_t)t);
        }
    }
    br.active_mask = keep_mask;
}

static void recover_mispredict(BoomCoreState& state, const BranchUpdate& update) {
    uint8_t tag = update.br_tag;
    uint8_t mispredict_mask = update.mispredict_mask;
    uint8_t keep_mask = update.uop.branch.br_mask;

    state.branch_state.mispredicts++;
    state.branch_state.rollbacks++;
    kill_issue_state(state, mispredict_mask);
    kill_execute_state(state, mispredict_mask);
    kill_lsu_state(state, mispredict_mask);
    kill_rob_younger_than(state, update.uop.queue.rob_idx, mispredict_mask);
    state.decode.dec_valids[0] = false;
    state.decode.dec_uops[0] = MicroOp();
    state.rename.renamed_valids[0] = false;
    state.rename.renamed_uops[0] = MicroOp();
    state.frontend.fetch_packet_valid = false;
    state.frontend.response_received = false;
    state.frontend.request_sent = false;
    state.frontend.flush = false;
    state.frontend.pc = update.jalr_target & ~0x3ULL;

    restore_map_snapshot(state, tag);
    rollback_free_list(state, tag);
    prune_recovered_tags(state, keep_mask, tag);
    clear_resolved_masks_in_state(state, mispredict_mask);
    rebuild_busy_after_recovery(state);
}

static void release_resolved_branch(BoomCoreState& state, uint8_t tag, uint8_t resolve_mask) {
    release_tag(state, tag);
    clear_resolved_masks_in_state(state, resolve_mask);
}

void branch_module(BoomCoreState& state) {
    for (int i=0; i<EXECUTE_RESULT_LANES; i++) {
        if (!state.execute.alu_results[i].valid) continue;
        const ExecuteState::AluResult& r = state.execute.alu_results[i];
        if (branch_is_control_uop(r.uop)) {
            uint8_t tag = r.uop.branch.br_tag;
            uint8_t resolve_mask = branch_tag_bit(tag);
            state.brupdate.valid = true;
            state.brupdate.mispredict = r.mispredict;
            state.brupdate.jalr_target = r.redirect_pc;
            state.brupdate.resolve_mask = resolve_mask;
            state.brupdate.mispredict_mask = r.mispredict ? resolve_mask : 0;
            state.brupdate.br_tag = tag;
            state.brupdate.uop = r.uop;
            state.brupdate.taken = r.mispredict;
            if (r.mispredict) recover_mispredict(state, state.brupdate);
            else release_resolved_branch(state, tag, resolve_mask);
            return;
        }
    }
}

}
