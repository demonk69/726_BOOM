#include "reset.hpp"

static void advance_reset(ResetControllerState& reset_ctrl, ResetPhase next) {
    reset_ctrl.phase = (uint8_t)next;
    reset_ctrl.index = 0;
}

void boom_core_reset_step(BoomCoreState& state, ResetControllerState& reset_ctrl) {
    uint8_t index = reset_ctrl.index;

    switch ((ResetPhase)reset_ctrl.phase) {
    case RESET_CONTROL:
        state.cycle_count = 0;
        state.global_flush = false;
        state.io_success = false;
        state.io_halted = false;
        state.io_trap = false;
        state.tohost = 0;
        state.brupdate.valid = false;
        state.brupdate.mispredict = false;
        state.decode.dec_valids[0] = false;
        state.rename.renamed_valids[0] = false;
        state.issue.issued_valids[0] = false;
        state.issue.issued_valids[1] = false;
        state.issue.issued_valids[2] = false;
        for (int i = 0; i < ISSUE_WIDTH; i++) {
            state.issue.grants[i] = IssueGrant();
            state.issue.port_ready[i] = (i != FP_ISSUE_LANE);
        }
        state.issue.grants_generated = 0;
        state.issue.grants_accepted = 0;
        state.issue.grants_retained = 0;
        state.issue.grants_dropped = 0;
        state.issue.cycles_with_0_grant = 0;
        state.issue.cycles_with_1_grant = 0;
        state.issue.cycles_with_2_grants = 0;
        state.issue.total_grants = 0;
        state.issue.mem_grants = 0;
        state.issue.int_grants = 0;
        state.issue.grant_stalls = 0;
        state.issue.execute_acceptance_stalls = 0;
        state.issue.dropped_grants = 0;
        state.rob.commit_valid = false;
        advance_reset(reset_ctrl, RESET_FRONTEND);
        break;

    case RESET_FRONTEND:
        state.frontend.pc = RESET_VECTOR;
        state.frontend.reset_done = false;
        state.frontend.request_sent = false;
        state.frontend.fetch_id = 0;
        state.frontend.pending_fetch_id = 0;
        state.frontend.response_received = false;
        state.frontend.resp_address = 0;
        state.frontend.resp_instruction = 0;
        state.frontend.resp_exception = false;
        state.frontend.resp_exc_cause = 0;
        state.frontend.stalled = false;
        state.frontend.flush = false;
        state.frontend.fetch_packet_valid = false;
        advance_reset(reset_ctrl, RESET_RENAME_MAP);
        break;

    case RESET_RENAME_MAP:
        state.rename.int_map_table.map_table[index] = 0;
        state.rename.int_map_table.committed_map_table[index] = 0;
        state.rename.fp_map_table.map_table[index] = 0;
        state.rename.fp_map_table.committed_map_table[index] = 0;
        if (index + 1 == LOGICAL_REG_COUNT) {
            advance_reset(reset_ctrl, RESET_FREE_BUSY);
        } else {
            reset_ctrl.index = index + 1;
        }
        break;

    case RESET_FREE_BUSY:
        if (index == 0) {
            state.rename.int_free_list.head = 1;
            state.rename.int_free_list.tail = 0;
            state.rename.int_free_list.count = INT_PHYS_REGS - 1;
            state.rename.fp_free_list.head = 1;
            state.rename.fp_free_list.tail = 0;
            state.rename.fp_free_list.count = INT_PHYS_REGS - 1;
        }
        state.rename.int_free_list.free_list[index] = index;
        state.rename.int_free_list.busy_table[index] = false;
        state.rename.fp_free_list.free_list[index] = index;
        state.rename.fp_free_list.busy_table[index] = false;
        if (index + 1 == INT_PHYS_REGS) {
            advance_reset(reset_ctrl, RESET_ROB);
        } else {
            reset_ctrl.index = index + 1;
        }
        break;

    case RESET_ROB:
#ifdef BOOM_HLS_GATE3_10_R1_RESET_ROB_PIPELINE
        state.rob.head = 0;
        state.rob.tail = 0;
        state.rob.maybe_full = false;
        state.rob.state = ROB_INIT;
        state.rob.commit_count = 0;
        state.rob.commit_valid = false;
RESET_ROB_INIT:
        for (int i = 0; i < ROB_DEPTH; i++) {
#pragma HLS PIPELINE II=1
            state.rob.entries[i].valid = false;
            state.rob.entries[i].busy = false;
        }
        advance_reset(reset_ctrl, RESET_IQ);
#else
        if (index == 0) {
            state.rob.head = 0;
            state.rob.tail = 0;
            state.rob.maybe_full = false;
            state.rob.state = ROB_INIT;
            state.rob.commit_count = 0;
            state.rob.commit_valid = false;
        }
        state.rob.entries[index].valid = false;
        state.rob.entries[index].busy = false;
        if (index + 1 == ROB_DEPTH) {
            advance_reset(reset_ctrl, RESET_IQ);
        } else {
            reset_ctrl.index = index + 1;
        }
#endif
        break;

    case RESET_IQ:
        if (index == 0) {
            state.issue.alu_iq.head = 0;
            state.issue.alu_iq.tail = 0;
            state.issue.alu_iq.count = 0;
        }
        state.issue.alu_iq.entries[index].valid = false;
        state.issue.alu_iq.entries[index].request = false;
        state.issue.alu_iq.entries[index].granted = false;
        if (index + 1 == ISSUE_QUEUE_ALU_DEPTH) {
            advance_reset(reset_ctrl, RESET_BRANCH);
        } else {
            reset_ctrl.index = index + 1;
        }
        break;

    case RESET_BRANCH:
        if (index == 0) {
            state.branch_state.active_mask = 0;
            state.branch_state.allocations = 0;
            state.branch_state.releases = 0;
            state.branch_state.mispredicts = 0;
            state.branch_state.rollbacks = 0;
        }
        state.branch_state.tag_valid[index] = false;
        state.branch_state.snapshot_valid[index] = false;
        if (index + 1 == MAX_BRANCH_COUNT) {
            advance_reset(reset_ctrl, RESET_EXECUTE);
        } else {
            reset_ctrl.index = index + 1;
        }
        break;

    case RESET_EXECUTE:
        for (int i = 0; i < EXECUTE_RESULT_LANES; i++) {
            state.execute.alu_results[i].valid = false;
            state.execute.alu_results[i].mispredict = false;
            state.execute.alu_results[i].memory_valid = false;
        }
        advance_reset(reset_ctrl, RESET_LSU);
        break;

    case RESET_LSU:
        if (index == 0) {
            state.lsu.ldq_head = 0;
            state.lsu.ldq_tail = 0;
            state.lsu.ldq_count = 0;
            state.lsu.stq_head = 0;
            state.lsu.stq_tail = 0;
            state.lsu.stq_count = 0;
            state.lsu.next_transaction_id = 1;
            state.lsu.load_response_pending = false;
            state.lsu.pending_load_transaction_id = 0;
            state.lsu.pending_load_rob_idx = 0;
        }
        state.lsu.ldq[index].valid = false;
        state.lsu.ldq[index].response_pending = false;
        state.lsu.stq[index].valid = false;
        state.lsu.stq[index].committed = false;
        state.lsu.stq[index].issued_to_memory = false;
        if (index + 1 == LDQ_DEPTH) {
            advance_reset(reset_ctrl, RESET_CSR);
        } else {
            reset_ctrl.index = index + 1;
        }
        break;

    case RESET_CSR:
        state.csr.mstatus = 0x0000000a00000000ull;
        state.csr.misa = 0x800000000014112dull;
        state.csr.mie = 0;
        state.csr.mtvec = 0;
        state.csr.mscratch = 0;
        state.csr.mepc = 0;
        state.csr.mcause = 0;
        state.csr.mtval = 0;
        state.csr.mip = 0;
        state.csr.cycle = 0;
        state.csr.instret = 0;
        state.csr.satp = 0;
        state.csr.priv = PRV_M;
        advance_reset(reset_ctrl, RESET_OUTPUTS);
        break;

    case RESET_OUTPUTS:
        state.int_rf[0] = 0;
        state.fp_rf[0] = 0;
        reset_ctrl.phase = RESET_DONE;
        reset_ctrl.index = 0;
        reset_ctrl.completed = true;
        break;

    case RESET_DONE:
    default:
        reset_ctrl.completed = true;
        break;
    }
}
