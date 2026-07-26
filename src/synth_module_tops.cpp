#include "boom_config.hpp"
#include "boom_types.hpp"
#include "boom_state.hpp"
#include "boom_interfaces.hpp"

extern void boom_core_step(BoomCoreState& state, PipeSignals& pipe);

namespace boom {
extern void frontend_module(BoomCoreState& state, PipeSignals& pipe);
extern void decode_module(BoomCoreState& state);
extern void rename_module(BoomCoreState& state);
extern void rob_allocate(BoomCoreState& state);
extern void rob_complete(BoomCoreState& state);
extern void issue_module(BoomCoreState& state);
extern void execute_module(BoomCoreState& state);
extern void branch_module(BoomCoreState& state);
extern void lsu_module(BoomCoreState& state, PipeSignals& pipe);
extern void rob_commit_module(BoomCoreState& state, PipeSignals& pipe);
}

void synth_frontend_top(hls::stream<ImemRequest>& imem_req_out,
                        hls::stream<ImemResponse>& imem_resp_in,
                        uint64_t seed,
                        uint64_t& observable) {
    static BoomCoreState state;
    if ((seed & 1ULL) != 0) state.frontend.pc = seed & ~0x3ULL;
    PipeSignals pipe;
    if (!imem_resp_in.empty()) pipe.imem_resp.write(imem_resp_in.read());
    boom::frontend_module(state, pipe);
    if (!pipe.imem_req.empty()) imem_req_out.write(pipe.imem_req.read());
    observable = state.frontend.pc;
}

void synth_decode_top(uint32_t seed_inst, uint64_t seed_pc, uint64_t& observable) {
    static BoomCoreState state;
    state.frontend.fetch_packet_valid = true;
    state.frontend.fetch_uop.inst = seed_inst;
    state.frontend.fetch_uop.debug_pc = seed_pc;
    boom::decode_module(state);
    observable = state.decode.dec_valids[0] ? state.decode.dec_uops[0].debug_pc : 0;
}

void synth_rename_top(uint32_t seed_inst, uint64_t seed_pc, uint64_t& observable) {
    static BoomCoreState state;
    state.decode.dec_valids[0] = true;
    state.decode.dec_uops[0].inst = seed_inst;
    state.decode.dec_uops[0].debug_pc = seed_pc;
    state.decode.dec_uops[0].uopc = 50;
    state.decode.dec_uops[0].rename.lrs1 = static_cast<uint8_t>((seed_inst >> 15) & 0x1f);
    state.decode.dec_uops[0].rename.lrs2 = static_cast<uint8_t>((seed_inst >> 20) & 0x1f);
    state.decode.dec_uops[0].rename.ldst = static_cast<uint8_t>((seed_inst >> 7) & 0x1f);
    state.decode.dec_uops[0].rename.dst_rtype = DST_INT;
    boom::rename_module(state);
    observable = state.rename.renamed_valids[0] ? state.rename.renamed_uops[0].debug_pc : 0;
}

void synth_rob_top(uint32_t seed_inst, uint64_t seed_pc, uint64_t& observable) {
    static BoomCoreState state;
    state.rename.renamed_valids[0] = true;
    state.rename.renamed_uops[0].inst = seed_inst;
    state.rename.renamed_uops[0].debug_pc = seed_pc;
    state.rename.renamed_uops[0].uopc = 50;
    boom::rob_allocate(state);
    boom::rob_complete(state);
    observable = state.rob.head;
}

void synth_issue_top(uint32_t seed_inst, uint64_t seed_pc, uint64_t& observable) {
    static BoomCoreState state;
    state.rename.renamed_valids[0] = true;
    state.rename.renamed_uops[0].inst = seed_inst;
    state.rename.renamed_uops[0].debug_pc = seed_pc;
    state.rename.renamed_uops[0].uopc = 50;
    state.rename.renamed_uops[0].iq_type = IQ_ALU;
    state.rename.renamed_uops[0].fu_code = FU_ALU;
    boom::issue_module(state);
    observable = state.issue.alu_iq.count;
}

void synth_execute_top(uint8_t seed_uopc, uint64_t seed_rs1, uint64_t seed_rs2, uint64_t& observable) {
    static BoomCoreState state;
    state.issue.issued_valids[0] = true;
    state.issue.issued_uops[0].uopc = seed_uopc;
    state.issue.issued_uops[0].rename.prs1 = 1;
    state.issue.issued_uops[0].rename.prs2 = 2;
    state.issue.issued_uops[0].rename.pdst = 3;
    state.issue.issued_uops[0].rename.dst_rtype = DST_INT;
    state.int_rf[1] = seed_rs1;
    state.int_rf[2] = seed_rs2;
    boom::execute_module(state);
    observable = state.execute.alu_results[0].result;
}

void synth_lsu_top(hls::stream<DmemRequest>& dmem_req_out,
                   hls::stream<DmemResponse>& dmem_resp_in,
                   uint64_t seed_addr,
                   uint64_t& observable) {
    static BoomCoreState state;
    state.execute.alu_results[0].valid = true;
    state.execute.alu_results[0].memory_valid = true;
    state.execute.alu_results[0].is_load = true;
    state.execute.alu_results[0].memory_address = seed_addr;
    state.execute.alu_results[0].memory_size = 3;
    state.execute.alu_results[0].memory_mask = 0xff;
    state.execute.alu_results[0].uop.queue.rob_idx = 0;
    state.rob.entries[0].valid = true;
    PipeSignals pipe;
    if (!dmem_resp_in.empty()) pipe.dmem_resp.write(dmem_resp_in.read());
    boom::lsu_module(state, pipe);
    if (!pipe.dmem_req.empty()) dmem_req_out.write(pipe.dmem_req.read());
    observable = state.lsu.next_transaction_id;
}

void synth_commit_top(hls::stream<DmemRequest>& dmem_req_out,
                       hls::stream<CommitEntry>& commit_trace_out,
                       uint64_t seed_pc,
                       uint64_t& observable) {
    static BoomCoreState state;
    state.rob.entries[state.rob.head].valid = true;
    state.rob.entries[state.rob.head].busy = false;
    state.rob.entries[state.rob.head].uop.uopc = 50;
    state.rob.entries[state.rob.head].uop.debug_pc = seed_pc;
    state.rob.entries[state.rob.head].uop.inst = 0x00100093u;
    PipeSignals pipe;
    boom::rob_commit_module(state, pipe);
    if (!pipe.dmem_req.empty()) dmem_req_out.write(pipe.dmem_req.read());
    if (!pipe.commit_trace.empty()) commit_trace_out.write(pipe.commit_trace.read());
    observable = state.csr.instret;
}

void synth_branch_tag_top(uint32_t seed_inst, uint64_t seed_pc, uint64_t& observable) {
    static BoomCoreState state;
    state.decode.dec_uops[0] = MicroOp();
    state.decode.dec_valids[0] = true;
    state.decode.dec_uops[0].inst = seed_inst;
    state.decode.dec_uops[0].debug_pc = seed_pc;
    state.decode.dec_uops[0].uopc = 50;
    state.decode.dec_uops[0].branch.is_br = true;
    state.decode.dec_uops[0].rename.lrs1 = static_cast<uint8_t>((seed_inst >> 15) & 0x1f);
    state.decode.dec_uops[0].rename.lrs2 = static_cast<uint8_t>((seed_inst >> 20) & 0x1f);
    state.decode.dec_uops[0].rename.ldst = static_cast<uint8_t>((seed_inst >> 7) & 0x1f);
    state.decode.dec_uops[0].rename.dst_rtype = DST_INT;
    boom::rename_module(state);
    observable = state.branch_state.active_mask;
    observable |= static_cast<uint64_t>(state.branch_state.allocations) << 8;
}

void synth_branch_mask_top(uint8_t seed_mask, uint64_t seed_pc, uint64_t& observable) {
    static BoomCoreState state;
    uint8_t tag = static_cast<uint8_t>(seed_mask & 0x7);
    uint8_t tag_mask = static_cast<uint8_t>(1u << tag);
    state.branch_state.active_mask |= tag_mask;
    state.branch_state.tag_valid[tag] = true;
    state.branch_state.snapshot_valid[tag] = true;
    state.execute.alu_results[0] = ExecuteState::AluResult();
    state.execute.alu_results[0].valid = true;
    state.execute.alu_results[0].mispredict = (seed_mask & 0x80) != 0;
    state.execute.alu_results[0].redirect_pc = seed_pc;
    state.execute.alu_results[0].uop.branch.is_br = true;
    state.execute.alu_results[0].uop.branch.br_tag = tag;
    state.execute.alu_results[0].uop.branch.br_mask = static_cast<uint8_t>(seed_mask & ~tag_mask);
    state.execute.alu_results[0].uop.queue.rob_idx = state.rob.head;
    boom::branch_module(state);
    observable = state.branch_state.active_mask;
    observable |= static_cast<uint64_t>(state.brupdate.mispredict_mask) << 8;
}

void synth_map_snapshot_top(uint8_t tag_seed, uint64_t seed_pc, uint64_t& observable) {
    static BoomCoreState state;
    uint8_t tag = static_cast<uint8_t>(tag_seed & 0x7);
    uint8_t tag_mask = static_cast<uint8_t>(1u << tag);
    for (int i=0; i<LOGICAL_REG_COUNT; i++) {
        state.rename.int_map_table.map_table[i] = static_cast<uint8_t>(i % INT_PHYS_REGS);
        state.rename.int_map_table.committed_map_table[i] = static_cast<uint8_t>(i % INT_PHYS_REGS);
        state.rename.int_map_table.br_snapshots[i][tag] = static_cast<uint8_t>((i + tag_seed) % INT_PHYS_REGS);
    }
    state.rename.int_map_table.br_snapshots[0][tag] = 0;
    state.branch_state.active_mask |= tag_mask;
    state.branch_state.tag_valid[tag] = true;
    state.branch_state.snapshot_valid[tag] = true;
    state.rob.entries[state.rob.head].valid = true;
    state.execute.alu_results[0] = ExecuteState::AluResult();
    state.execute.alu_results[0].valid = true;
    state.execute.alu_results[0].mispredict = true;
    state.execute.alu_results[0].redirect_pc = seed_pc;
    state.execute.alu_results[0].uop.branch.is_br = true;
    state.execute.alu_results[0].uop.branch.br_tag = tag;
    state.execute.alu_results[0].uop.branch.br_mask = static_cast<uint8_t>(tag_mask >> 1);
    state.execute.alu_results[0].uop.queue.rob_idx = state.rob.head;
    boom::branch_module(state);
    observable = state.rename.int_map_table.map_table[1];
    observable |= static_cast<uint64_t>(state.rename.int_map_table.map_table[31]) << 8;
}

void synth_free_list_rollback_top(uint8_t tag_seed, uint64_t seed_pc, uint64_t& observable) {
    static BoomCoreState state;
    uint8_t tag = static_cast<uint8_t>(tag_seed & 0x7);
    uint8_t tag_mask = static_cast<uint8_t>(1u << tag);
    for (int i=0; i<LOGICAL_REG_COUNT; i++) {
        state.rename.int_map_table.map_table[i] = static_cast<uint8_t>(i % INT_PHYS_REGS);
        state.rename.int_map_table.br_snapshots[i][tag] = state.rename.int_map_table.map_table[i];
    }
    state.rename.int_map_table.map_table[0] = 0;
    state.rename.int_map_table.br_snapshots[0][tag] = 0;
    for (int p=1; p<INT_PHYS_REGS; p++) {
        state.branch_state.br_alloc_lists[tag][p] = ((p + tag_seed) & 3) == 0;
        state.rename.int_free_list.busy_table[p] = state.branch_state.br_alloc_lists[tag][p];
    }
    state.branch_state.active_mask |= tag_mask;
    state.branch_state.tag_valid[tag] = true;
    state.branch_state.snapshot_valid[tag] = true;
    state.rob.entries[state.rob.head].valid = true;
    state.execute.alu_results[0] = ExecuteState::AluResult();
    state.execute.alu_results[0].valid = true;
    state.execute.alu_results[0].mispredict = true;
    state.execute.alu_results[0].redirect_pc = seed_pc;
    state.execute.alu_results[0].uop.branch.is_br = true;
    state.execute.alu_results[0].uop.branch.br_tag = tag;
    state.execute.alu_results[0].uop.queue.rob_idx = state.rob.head;
    boom::branch_module(state);
    observable = state.rename.int_free_list.count;
    observable |= static_cast<uint64_t>(state.branch_state.rollbacks) << 8;
}

void synth_busy_recovery_top(uint8_t tag_seed, uint64_t seed_pc, uint64_t& observable) {
    static BoomCoreState state;
    uint8_t tag = static_cast<uint8_t>(tag_seed & 0x7);
    uint8_t tag_mask = static_cast<uint8_t>(1u << tag);
    for (int i=0; i<ROB_DEPTH; i++) {
        state.rob.entries[i].valid = (i & 1) == 0;
        state.rob.entries[i].busy = (i & 3) == 0;
        state.rob.entries[i].uop.rename.pdst = static_cast<uint8_t>((i + 1) % INT_PHYS_REGS);
    }
    for (int i=0; i<LOGICAL_REG_COUNT; i++) state.rename.int_map_table.br_snapshots[i][tag] = static_cast<uint8_t>(i % INT_PHYS_REGS);
    state.branch_state.active_mask |= tag_mask;
    state.branch_state.tag_valid[tag] = true;
    state.branch_state.snapshot_valid[tag] = true;
    state.execute.alu_results[0] = ExecuteState::AluResult();
    state.execute.alu_results[0].valid = true;
    state.execute.alu_results[0].mispredict = true;
    state.execute.alu_results[0].redirect_pc = seed_pc;
    state.execute.alu_results[0].uop.branch.is_br = true;
    state.execute.alu_results[0].uop.branch.br_tag = tag;
    state.execute.alu_results[0].uop.queue.rob_idx = state.rob.head;
    boom::branch_module(state);
    observable = 0;
    for (int p=0; p<INT_PHYS_REGS; p++) if (state.rename.int_free_list.busy_table[p]) observable++;
}

void synth_branch_kill_top(uint8_t seed_mask, uint64_t seed_pc, uint64_t& observable) {
    static BoomCoreState state;
    uint8_t tag = static_cast<uint8_t>(seed_mask & 0x7);
    uint8_t tag_mask = static_cast<uint8_t>(1u << tag);
    for (int i=0; i<ISSUE_QUEUE_ALU_DEPTH; i++) {
        state.issue.alu_iq.entries[i].valid = true;
        state.issue.alu_iq.entries[i].uop.branch.br_mask = (i & 1) ? tag_mask : 0;
    }
    state.issue.alu_iq.count = ISSUE_QUEUE_ALU_DEPTH;
    for (int i=0; i<LDQ_DEPTH; i++) {
        state.lsu.ldq[i].valid = true;
        state.lsu.ldq[i].branch_mask = (i & 1) ? tag_mask : 0;
    }
    for (int i=0; i<STQ_DEPTH; i++) {
        state.lsu.stq[i].valid = true;
        state.lsu.stq[i].branch_mask = (i & 1) ? tag_mask : 0;
    }
    for (int i=0; i<LOGICAL_REG_COUNT; i++) state.rename.int_map_table.br_snapshots[i][tag] = static_cast<uint8_t>(i % INT_PHYS_REGS);
    state.branch_state.active_mask |= tag_mask;
    state.branch_state.tag_valid[tag] = true;
    state.branch_state.snapshot_valid[tag] = true;
    state.rob.entries[state.rob.head].valid = true;
    state.execute.alu_results[0] = ExecuteState::AluResult();
    state.execute.alu_results[0].valid = true;
    state.execute.alu_results[0].mispredict = true;
    state.execute.alu_results[0].redirect_pc = seed_pc;
    state.execute.alu_results[0].uop.branch.is_br = true;
    state.execute.alu_results[0].uop.branch.br_tag = tag;
    state.execute.alu_results[0].uop.queue.rob_idx = state.rob.head;
    boom::branch_module(state);
    observable = state.issue.alu_iq.count;
    observable |= static_cast<uint64_t>(state.lsu.ldq_count) << 8;
    observable |= static_cast<uint64_t>(state.lsu.stq_count) << 16;
}

void synth_core_step_top(hls::stream<ImemRequest>& imem_req_out,
                          hls::stream<ImemResponse>& imem_resp_in,
                         hls::stream<DmemRequest>& dmem_req_out,
                         hls::stream<DmemResponse>& dmem_resp_in,
                         hls::stream<CommitEntry>& commit_trace_out,
                         uint64_t seed_pc,
                         uint64_t& observable) {
    static BoomCoreState state;
    if ((seed_pc & 1ULL) != 0) state.frontend.pc = seed_pc & ~0x3ULL;
    PipeSignals pipe;
    if (!imem_resp_in.empty()) pipe.imem_resp.write(imem_resp_in.read());
    if (!dmem_resp_in.empty()) pipe.dmem_resp.write(dmem_resp_in.read());
    boom_core_step(state, pipe);
    if (!pipe.imem_req.empty()) imem_req_out.write(pipe.imem_req.read());
    if (!pipe.dmem_req.empty()) dmem_req_out.write(pipe.dmem_req.read());
    if (!pipe.commit_trace.empty()) commit_trace_out.write(pipe.commit_trace.read());
    observable = state.csr.cycle;
}
