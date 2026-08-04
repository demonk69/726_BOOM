#include "boom_config.hpp"
#include "boom_types.hpp"
#include "boom_state.hpp"
#include "boom_interfaces.hpp"
#include "reset.hpp"
#include "completion.hpp"

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
extern void branch_complete_event(BoomCoreState& state, const MicroOp& uop,
                                  bool mispredict, uint64_t redirect_pc);
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
    observable = state.rename.dispatch_packets[0].valid ? state.rename.dispatch_packets[0].uop.debug_pc : 0;
}

void synth_rob_top(uint32_t seed_inst, uint64_t seed_pc, uint64_t& observable) {
    static BoomCoreState state;
    state.rename.dispatch_packets[0].valid = true;
    state.rename.dispatch_packets[0].uop.inst = seed_inst;
    state.rename.dispatch_packets[0].uop.debug_pc = seed_pc;
    state.rename.dispatch_packets[0].uop.uopc = 50;
    boom::rob_allocate(state);
    boom::rob_complete(state);
    observable = state.rob.head;
}

void synth_issue_top(uint32_t seed_inst, uint64_t seed_pc, uint64_t& observable) {
    static BoomCoreState state;
    for (int i=0; i<ISSUE_QUEUE_ALU_DEPTH; i++) state.issue.alu_iq.entries[i]=IssueSlotEntry();
    state.issue.alu_iq.head=0; state.issue.alu_iq.tail=0; state.issue.alu_iq.count=0;
    state.rename.dispatch_packets[0]=RenameDispatchPacket();
    state.brupdate=BranchUpdate();
    state.issue.port_ready[MEM_ISSUE_LANE]=(seed_inst & 4u)!=0;
    state.issue.port_ready[INT_ISSUE_LANE]=(seed_inst & 8u)!=0;
    state.issue.port_ready[FP_ISSUE_LANE]=false;
    int count=0;
    if ((seed_inst & 1u)!=0) {
        IssueSlotEntry& mem=state.issue.alu_iq.entries[count++];
        mem.valid=true; mem.request=true; mem.uop.uopc=39; mem.uop.iq_type=IQ_MEM;
        mem.uop.fu_code=FU_MEM; mem.uop.queue.rob_idx=1; mem.uop.debug_pc=seed_pc;
        if ((seed_inst & 16u)!=0) mem.uop.branch.br_mask=1;
    }
    if ((seed_inst & 2u)!=0) {
        IssueSlotEntry& integer=state.issue.alu_iq.entries[count++];
        integer.valid=true; integer.request=true; integer.uop.uopc=1; integer.uop.iq_type=IQ_ALU;
        integer.uop.fu_code=FU_ALU; integer.uop.queue.rob_idx=2; integer.uop.debug_pc=seed_pc+4;
    }
    state.issue.alu_iq.count=(uint8_t)count;
    state.issue.alu_iq.tail=(uint8_t)(count%ISSUE_QUEUE_ALU_DEPTH);
    if ((seed_inst & 16u)!=0) {
        state.brupdate.valid=true; state.brupdate.mispredict=true;
        state.brupdate.resolve_mask=1; state.brupdate.mispredict_mask=1;
    }
    boom::issue_module(state);
    observable=(uint64_t)state.issue.alu_iq.count;
    observable|=(uint64_t)state.issue.grants_generated<<8;
    observable|=(uint64_t)state.issue.grants_accepted<<16;
    observable|=(uint64_t)state.issue.grants_retained<<24;
    observable|=(uint64_t)state.issue.grants_dropped<<32;
    observable|=(uint64_t)state.issue.grants[MEM_ISSUE_LANE].valid<<40;
    observable|=(uint64_t)state.issue.grants[MEM_ISSUE_LANE].accepted<<41;
    observable|=(uint64_t)state.issue.grants[INT_ISSUE_LANE].valid<<42;
    observable|=(uint64_t)state.issue.grants[INT_ISSUE_LANE].accepted<<43;
    observable|=(uint64_t)state.issue.grants[FP_ISSUE_LANE].valid<<44;
    uint8_t retained_rob=state.issue.grants[MEM_ISSUE_LANE].valid &&
        !state.issue.grants[MEM_ISSUE_LANE].accepted ? state.issue.grants[MEM_ISSUE_LANE].uop.queue.rob_idx :
        (state.issue.grants[INT_ISSUE_LANE].valid && !state.issue.grants[INT_ISSUE_LANE].accepted ?
         state.issue.grants[INT_ISSUE_LANE].uop.queue.rob_idx : 0);
    observable|=(uint64_t)retained_rob<<48;
    observable|=(uint64_t)(state.issue.issued_valids[0] ? state.issue.issued_uops[0].queue.rob_idx : 0)<<56;
}

void synth_execute_top(uint8_t seed_uopc, uint64_t seed_rs1, uint64_t seed_rs2, uint64_t& observable) {
    static BoomCoreState state;
    state.execute.alu_results[INT_ISSUE_LANE] = ExecuteState::AluResult();
    state.issue.issued_valids[INT_ISSUE_LANE] = true;
    state.issue.issued_uops[INT_ISSUE_LANE].uopc = seed_uopc;
    state.issue.issued_uops[INT_ISSUE_LANE].iq_type = IQ_ALU;
    state.issue.issued_uops[INT_ISSUE_LANE].fu_code = FU_ALU;
    state.issue.issued_uops[INT_ISSUE_LANE].rename.prs1 = 1;
    state.issue.issued_uops[INT_ISSUE_LANE].rename.prs2 = 2;
    state.issue.issued_uops[INT_ISSUE_LANE].rename.pdst = 3;
    state.issue.issued_uops[INT_ISSUE_LANE].rename.dst_rtype = DST_INT;
    boom::prf_seed(state, 1, seed_rs1);
    boom::prf_seed(state, 2, seed_rs2);
    boom::execute_module(state);
    observable = state.execute.alu_results[INT_ISSUE_LANE].result;
}

void synth_completion_top(uint8_t seed_sources, uint8_t seed_head,
                           uint64_t seed_value, uint64_t& observable) {
    static BoomCoreState state;
    state.rob.head = (uint8_t)(seed_head % ROB_DEPTH);
    uint8_t mem_idx = (uint8_t)((state.rob.head + 1) % ROB_DEPTH);
    uint8_t int_idx = state.rob.head;
    state.rob.entries[mem_idx].valid = true;
    state.rob.entries[mem_idx].busy = true;
    state.rob.entries[mem_idx].uop.queue.rob_idx = mem_idx;
    state.rob.entries[mem_idx].uop.queue.rob_allocation_id = 1;
    state.rob.entries[int_idx].valid = true;
    state.rob.entries[int_idx].busy = true;
    state.rob.entries[int_idx].uop.queue.rob_idx = int_idx;
    state.rob.entries[int_idx].uop.queue.rob_allocation_id = 2;

    if (seed_sources & 1) {
        ExecuteState::AluResult& result = state.execute.alu_results[MEM_ISSUE_LANE];
        result.valid = true;
        result.uop = state.rob.entries[mem_idx].uop;
        result.uop.rename.pdst = 3;
        result.uop.rename.dst_rtype = DST_INT;
        result.result = seed_value;
    }
    if (seed_sources & 2) {
        ExecuteState::AluResult& result = state.execute.alu_results[INT_ISSUE_LANE];
        result.valid = true;
        result.uop = state.rob.entries[int_idx].uop;
        result.uop.rename.pdst = 4;
        result.uop.rename.dst_rtype = DST_INT;
        result.result = seed_value + 1;
    }
    boom::completion_service_execute(state);
    observable = state.execute.alu_results[MEM_ISSUE_LANE].valid;
    observable |= (uint64_t)state.execute.alu_results[INT_ISSUE_LANE].valid << 1;
    observable |= (uint64_t)state.rob.entries[mem_idx].busy << 2;
    observable |= (uint64_t)state.rob.entries[int_idx].busy << 3;
    observable |= (boom::prf_read(state, 3) ^ boom::prf_read(state, 4)) << 8;
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

static void w3_seed_rob(BoomCoreState& state, uint8_t index, uint32_t allocation_id) {
#pragma HLS INLINE
    state.rob.entries[index] = RobEntry();
    state.rob.entries[index].valid = true;
    state.rob.entries[index].busy = true;
    state.rob.entries[index].uop.queue.rob_idx = index;
    state.rob.entries[index].uop.queue.rob_allocation_id = allocation_id;
}

static ExecuteState::AluResult w3_result(uint8_t rob_idx, uint32_t allocation_id) {
    ExecuteState::AluResult result;
    result.valid = true;
    result.uop.uopc = 1;
    result.uop.iq_type = IQ_ALU;
    result.uop.fu_code = FU_ALU;
    result.uop.queue.rob_idx = rob_idx;
    result.uop.queue.rob_allocation_id = allocation_id;
    return result;
}

static void w3_seed_issue(BoomCoreState& state, int slot, bool memory,
                          uint8_t rob_idx, uint32_t allocation_id) {
#pragma HLS INLINE
    IssueSlotEntry& entry = state.issue.alu_iq.entries[slot];
    entry = IssueSlotEntry();
    entry.valid = true;
    entry.request = true;
    entry.uop.uopc = memory ? 39 : 1;
    entry.uop.iq_type = memory ? IQ_MEM : IQ_ALU;
    entry.uop.fu_code = memory ? FU_MEM : FU_ALU;
    entry.uop.ctrl.is_load = memory;
    entry.uop.mem.uses_ldq = memory;
    entry.uop.queue.rob_idx = rob_idx;
    entry.uop.queue.rob_allocation_id = allocation_id;
    state.issue.alu_iq.count++;
    state.issue.alu_iq.tail = state.issue.alu_iq.count % ISSUE_QUEUE_ALU_DEPTH;
}

static uint64_t w3_pack_observable(const BoomCoreState& state, uint8_t check0,
                                   uint8_t check1, bool trace_blocked,
                                   bool dmem_blocked) {
    uint64_t observable = state.execute.alu_results[MEM_ISSUE_LANE].valid;
    observable |= (uint64_t)state.execute.alu_results[INT_ISSUE_LANE].valid << 1;
    observable |= (uint64_t)(state.issue.grants_accepted & 3) << 2;
    observable |= (uint64_t)(state.issue.alu_iq.count & 15) << 4;
    observable |= (uint64_t)state.issue.issued_valids[MEM_ISSUE_LANE] << 8;
    observable |= (uint64_t)state.issue.issued_valids[INT_ISSUE_LANE] << 9;
    observable |= (uint64_t)(state.brupdate.valid && state.brupdate.mispredict) << 10;
    observable |= (uint64_t)state.lsu.load_response_pending << 11;
    observable |= (uint64_t)(state.lsu.ldq_count & 15) << 12;
    observable |= (uint64_t)(state.lsu.stq_count & 15) << 16;
    observable |= (uint64_t)state.rob.entries[check0].valid << 20;
    observable |= (uint64_t)state.rob.entries[check0].busy << 21;
    observable |= (uint64_t)state.rob.entries[check1].valid << 22;
    observable |= (uint64_t)state.rob.entries[check1].busy << 23;
    observable |= (uint64_t)state.rob.commit_valid << 24;
    observable |= (uint64_t)trace_blocked << 25;
    observable |= (uint64_t)dmem_blocked << 26;
    observable |= (uint64_t)(state.rob.head & 31) << 27;
    observable |= (uint64_t)(state.execute.alu_results[MEM_ISSUE_LANE].uop.queue.rob_allocation_id & 0xffff) << 32;
    observable |= (uint64_t)(state.execute.alu_results[INT_ISSUE_LANE].uop.queue.rob_allocation_id & 0xffff) << 48;
    return observable;
}

// Stateless directed diagnostic wrapper. It only seeds state and invokes the
// production modules; scenario selection does not exist in the core datapath.
void synth_w3_diagnostic_top(uint8_t scenario, uint64_t& observable) {
    BoomCoreState state;
    PipeSignals lsu_pipe;
    static PipeSignals commit_pipe;
#pragma HLS STREAM variable=commit_pipe.commit_trace depth=1
#pragma HLS STREAM variable=commit_pipe.dmem_req depth=1
    uint8_t check0 = 1;
    uint8_t check1 = 2;
    bool run_issue = false;
    bool run_execute = false;
    bool run_complete = false;
    bool run_lsu = false;
    bool run_commit = false;
    bool run_reset = false;
    bool carry_mem_id = false;
    bool carry_int_id = false;
    uint32_t held_mem_id = 0;
    uint32_t held_int_id = 0;

    if (scenario == 107) {
        CommitEntry entry;
#ifdef __SYNTHESIS__
        commit_pipe.commit_trace.write(entry);
#else
        for (int i = 0; i < 1024; i++) commit_pipe.commit_trace.write(entry);
#endif
    } else if (scenario == 108) {
        DmemRequest request;
#ifdef __SYNTHESIS__
        commit_pipe.dmem_req.write(request);
#else
        for (int i = 0; i < 1024; i++) commit_pipe.dmem_req.write(request);
#endif
    } else if (scenario <= 2) {
        w3_seed_issue(state, 0, true, 1, 11);
        w3_seed_issue(state, 1, false, 2, 12);
        if (scenario == 1) {
            state.execute.alu_results[MEM_ISSUE_LANE] = w3_result(9, 19);
            carry_mem_id = true;
            held_mem_id = 19;
        }
        if (scenario == 2) {
            state.execute.alu_results[INT_ISSUE_LANE] = w3_result(9, 29);
            carry_int_id = true;
            held_int_id = 29;
        }
        state.issue.port_ready[MEM_ISSUE_LANE] = !state.execute.alu_results[MEM_ISSUE_LANE].valid;
        state.issue.port_ready[INT_ISSUE_LANE] = !state.execute.alu_results[INT_ISSUE_LANE].valid;
        run_issue = true;
        run_execute = true;
    } else if (scenario == 3) {
        w3_seed_rob(state, 1, 31);
        w3_seed_rob(state, 2, 32);
        state.rob.head = 1;
        state.execute.alu_results[MEM_ISSUE_LANE] = w3_result(1, 31);
        state.execute.alu_results[INT_ISSUE_LANE] = w3_result(2, 32);
        run_complete = true;
    } else if (scenario == 4) {
        w3_seed_rob(state, 1, 41);
        w3_seed_rob(state, 2, 42);
        state.rob.head = 1;
        state.rob.tail = 3;
        state.branch_state.active_mask = 1;
        state.branch_state.tag_valid[0] = true;
        state.branch_state.snapshot_valid[0] = true;
        state.execute.alu_results[MEM_ISSUE_LANE] = w3_result(2, 42);
        state.execute.alu_results[MEM_ISSUE_LANE].uop.branch.br_mask = 1;
        state.execute.alu_results[INT_ISSUE_LANE] = w3_result(1, 41);
        state.execute.alu_results[INT_ISSUE_LANE].uop.branch.is_br = true;
        state.execute.alu_results[INT_ISSUE_LANE].uop.branch.br_tag = 0;
        state.execute.alu_results[INT_ISSUE_LANE].mispredict = true;
        run_complete = true;
    } else if (scenario == 5) {
        w3_seed_rob(state, 1, 51);
        w3_seed_rob(state, 2, 52);
        state.execute.alu_results[MEM_ISSUE_LANE] = w3_result(1, 51);
        state.execute.alu_results[INT_ISSUE_LANE] = w3_result(2, 52);
        state.lsu.load_response_pending = true;
        run_reset = true;
    } else if (scenario == 6) {
        w3_seed_rob(state, 1, 61);
        w3_seed_rob(state, 2, 62);
        state.rob.entries[1].is_load = true;
        state.rob.entries[1].memory_request_sent = true;
        state.rob.entries[1].memory_transaction_id = 7;
        state.rob.entries[1].uop.rename.pdst = 7;
        state.lsu.load_response_pending = true;
        state.lsu.pending_load_transaction_id = 7;
        state.lsu.pending_load_rob_idx = 1;
        state.lsu.pending_load_allocation_id = 61;
        state.lsu.ldq_count = 1;
        state.lsu.ldq[0].valid = true;
        state.lsu.ldq[0].rob_idx = 1;
        state.lsu.ldq[0].rob_allocation_id = 61;
        state.execute.alu_results[INT_ISSUE_LANE] = w3_result(2, 62);
        carry_int_id = true;
        held_int_id = 62;
        DmemResponse response;
        response.transaction_id = 7;
        response.data = 0x1234;
        response.read_data = 0x1234;
        lsu_pipe.dmem_resp.write(response);
        run_lsu = true;
    } else if (scenario == 7) {
        w3_seed_rob(state, 1, 71);
        state.rob.head = 1;
        state.rob.tail = 2;
        state.rob.entries[1].busy = false;
        state.rob.entries[1].uop.uopc = 1;
        run_commit = true;
    } else if (scenario == 8) {
        w3_seed_rob(state, 1, 81);
        state.rob.head = 1;
        state.rob.tail = 2;
        state.rob.entries[1].busy = false;
        state.rob.entries[1].is_store = true;
        state.rob.entries[1].memory_valid = true;
        state.rob.entries[1].uop.uopc = 49;
        state.rob.entries[1].uop.ctrl.is_sta = true;
        state.lsu.stq_count = 1;
        state.lsu.stq[0].valid = true;
        state.lsu.stq[0].rob_idx = 1;
        state.lsu.stq[0].rob_allocation_id = 81;
        run_commit = true;
    } else if (scenario == 9) {
        check0 = 31;
        check1 = 0;
        w3_seed_rob(state, 31, 91);
        w3_seed_rob(state, 0, 92);
        state.rob.head = 31;
        state.execute.alu_results[MEM_ISSUE_LANE] = w3_result(0, 92);
        state.execute.alu_results[INT_ISSUE_LANE] = w3_result(31, 91);
        run_complete = true;
    } else {
        check0 = 3;
        check1 = 4;
        w3_seed_rob(state, 3, 102);
        state.execute.alu_results[MEM_ISSUE_LANE] = w3_result(3, 101);
        run_complete = true;
    }

    if (run_reset) {
        ResetControllerState reset;
        for (int i = 0; i < 200 && !reset.completed; i++) boom_core_reset_step(state, reset);
    }
    bool completion_seed_ready = true;
    volatile uint32_t completion_seed_guard = 0;
    if (run_complete) {
        uint8_t mem_idx = state.execute.alu_results[MEM_ISSUE_LANE].uop.queue.rob_idx;
        uint8_t int_idx = state.execute.alu_results[INT_ISSUE_LANE].uop.queue.rob_idx;
        if (state.execute.alu_results[MEM_ISSUE_LANE].valid)
            completion_seed_ready &= mem_idx < ROB_DEPTH && state.rob.entries[mem_idx].valid;
        if (state.execute.alu_results[INT_ISSUE_LANE].valid)
            completion_seed_ready &= int_idx < ROB_DEPTH && state.rob.entries[int_idx].valid;
        if (mem_idx < ROB_DEPTH)
            completion_seed_guard ^= state.rob.entries[mem_idx].uop.queue.rob_allocation_id;
        if (int_idx < ROB_DEPTH)
            completion_seed_guard ^= state.rob.entries[int_idx].uop.queue.rob_allocation_id;
    }
    (void)completion_seed_guard;
    if (run_complete && completion_seed_ready) boom::rob_complete(state);
    if (run_lsu) boom::lsu_module(state, lsu_pipe);
    if (!lsu_pipe.dmem_req.empty()) lsu_pipe.dmem_req.read();
    if (run_commit) boom::rob_commit_module(state, commit_pipe);
    if (run_issue) boom::issue_module(state);
    if (run_execute) boom::execute_module(state);
    if (carry_mem_id && state.execute.alu_results[MEM_ISSUE_LANE].valid)
        state.execute.alu_results[MEM_ISSUE_LANE].uop.queue.rob_allocation_id = held_mem_id;
    if (carry_int_id && state.execute.alu_results[INT_ISSUE_LANE].valid)
        state.execute.alu_results[INT_ISSUE_LANE].uop.queue.rob_allocation_id = held_int_id;

    bool trace_blocked = scenario == 7 && state.rob.entries[1].valid &&
                         !state.rob.commit_valid;
    bool dmem_blocked = scenario == 8 && state.rob.entries[1].valid &&
                        !state.rob.entries[1].memory_request_sent;
    if (scenario < 100) {
#ifdef __SYNTHESIS__
        if (!commit_pipe.commit_trace.empty()) commit_pipe.commit_trace.read();
        if (!commit_pipe.dmem_req.empty()) commit_pipe.dmem_req.read();
#else
        while (!commit_pipe.commit_trace.empty()) commit_pipe.commit_trace.read();
        while (!commit_pipe.dmem_req.empty()) commit_pipe.dmem_req.read();
#endif
    }

    observable = w3_pack_observable(state, check0, check1, trace_blocked, dmem_blocked);
}

void synth_w3_completion_diagnostic_top(uint8_t scenario, uint64_t& observable) {
    BoomCoreState state;
    uint8_t check0 = 1;
    uint8_t check1 = 2;

    if (scenario == 3) {
        w3_seed_rob(state, 1, 31);
        w3_seed_rob(state, 2, 32);
        state.rob.head = 1;
        state.execute.alu_results[MEM_ISSUE_LANE] = w3_result(1, 31);
        state.execute.alu_results[INT_ISSUE_LANE] = w3_result(2, 32);
    } else if (scenario == 4) {
        w3_seed_rob(state, 1, 41);
        w3_seed_rob(state, 2, 42);
        state.rob.head = 1;
        state.rob.tail = 3;
        state.branch_state.active_mask = 1;
        state.branch_state.tag_valid[0] = true;
        state.branch_state.snapshot_valid[0] = true;
        state.execute.alu_results[MEM_ISSUE_LANE] = w3_result(2, 42);
        state.execute.alu_results[MEM_ISSUE_LANE].uop.branch.br_mask = 1;
        state.execute.alu_results[INT_ISSUE_LANE] = w3_result(1, 41);
        state.execute.alu_results[INT_ISSUE_LANE].uop.branch.is_br = true;
        state.execute.alu_results[INT_ISSUE_LANE].uop.branch.br_tag = 0;
        state.execute.alu_results[INT_ISSUE_LANE].mispredict = true;
    } else if (scenario == 9) {
        check0 = 31;
        check1 = 0;
        w3_seed_rob(state, 31, 91);
        w3_seed_rob(state, 0, 92);
        state.rob.head = 31;
        state.execute.alu_results[MEM_ISSUE_LANE] = w3_result(0, 92);
        state.execute.alu_results[INT_ISSUE_LANE] = w3_result(31, 91);
    } else {
        check0 = 3;
        check1 = 4;
        w3_seed_rob(state, 3, 102);
        state.execute.alu_results[MEM_ISSUE_LANE] = w3_result(3, 101);
    }

    boom::rob_complete(state);
    observable = w3_pack_observable(state, check0, check1, false, false);
}

void synth_w3_dual_pending_top(uint32_t allocation_base, uint64_t& observable) {
    BoomCoreState state;
    w3_seed_rob(state, 1, allocation_base);
    w3_seed_rob(state, 2, allocation_base + 1);
    state.rob.head = 1;
    state.execute.alu_results[MEM_ISSUE_LANE] = w3_result(1, allocation_base);
    state.execute.alu_results[INT_ISSUE_LANE] = w3_result(2, allocation_base + 1);
    boom::rob_complete(state);
    observable = w3_pack_observable(state, 1, 2, false, false);
}

void synth_w3_rob_wrap_top(uint32_t allocation_base, uint64_t& observable) {
    BoomCoreState state;
    w3_seed_rob(state, 31, allocation_base);
    w3_seed_rob(state, 0, allocation_base + 1);
    state.rob.head = 31;
    state.execute.alu_results[MEM_ISSUE_LANE] = w3_result(0, allocation_base + 1);
    state.execute.alu_results[INT_ISSUE_LANE] = w3_result(31, allocation_base);
    boom::rob_complete(state);
    observable = w3_pack_observable(state, 31, 0, false, false);
}

void synth_w3_branch_kill_top(uint32_t allocation_base, uint8_t control,
                              uint64_t& observable) {
    BoomCoreState state;
    w3_seed_rob(state, 1, allocation_base);
    w3_seed_rob(state, 2, allocation_base + 1);
    state.rob.head = 1;
    state.rob.tail = 3;
    state.branch_state.active_mask = (control & 1) ? 1 : 0;
    state.branch_state.tag_valid[0] = (control & 2) != 0;
    state.branch_state.snapshot_valid[0] = (control & 4) != 0;
    state.execute.alu_results[MEM_ISSUE_LANE] = w3_result(2, allocation_base + 1);
    state.execute.alu_results[MEM_ISSUE_LANE].uop.branch.br_mask =
        (control & 8) ? 1 : 0;
    state.execute.alu_results[INT_ISSUE_LANE] = w3_result(1, allocation_base);
    state.execute.alu_results[INT_ISSUE_LANE].uop.branch.is_br =
        (control & 16) != 0;
    state.execute.alu_results[INT_ISSUE_LANE].uop.branch.br_tag = 0;
    state.execute.alu_results[INT_ISSUE_LANE].mispredict =
        (control & 32) != 0;
    if ((control & 16) != 0) {
        boom::branch_complete_event(
            state, state.execute.alu_results[INT_ISSUE_LANE].uop,
            (control & 32) != 0, 0);
        // Production resolves control before completion service. Avoid asking
        // the legacy W3 serial service to resolve the same branch again.
        state.execute.alu_results[INT_ISSUE_LANE].uop.branch.is_br = false;
        state.execute.alu_results[INT_ISSUE_LANE].mispredict = false;
    }
    boom::rob_complete(state);
    observable = w3_pack_observable(state, 1, 2, false, false);
}

static void w4d_oracle_owner(BoomCoreState& state, uint8_t index,
                             uint32_t allocation, uint8_t pdst) {
#pragma HLS INLINE
    RobEntry& entry = state.rob.entries[index];
    entry = RobEntry();
    entry.valid = true;
    entry.busy = true;
    entry.uop.uopc = 1;
    entry.uop.queue.rob_idx = index;
    entry.uop.queue.rob_allocation_id = allocation;
    entry.uop.rename.pdst = pdst;
    entry.uop.rename.dst_rtype = pdst ? DST_INT : DST_N;
    if (pdst) state.rename.int_free_list.busy_table[pdst] = true;
}

static ExecuteState::AluResult w4d_oracle_result(
        const BoomCoreState& state, uint8_t index, uint64_t value) {
#pragma HLS INLINE
    ExecuteState::AluResult result;
    result.valid = true;
    result.uop = state.rob.entries[index].uop;
    result.result = value;
    return result;
}

// Scenario wrapper retained for an independent generated-RTL oracle. Bit 7
// continues retained state; otherwise each call starts from a clean machine.
void synth_w4d_oracle_top(uint8_t scenario, uint64_t* observable) {
#pragma HLS INTERFACE ap_ctrl_none port=return
#pragma HLS INTERFACE ap_none port=scenario
#pragma HLS INTERFACE ap_vld port=observable
    static BoomCoreState state;
    bool production_retained = false;
    bool production_first_limits = false;
    bool production_single_reset = false;
    bool production_consumed = false;
    bool production_values = false;
    bool production_peaks = false;
    if ((scenario & 0x80) == 0) {
        state = BoomCoreState();
        state.rob.head = 1;
        uint8_t operation = scenario & 0x7f;
        if (operation == 0) {
        w4d_oracle_owner(state, 1, 101, 10);
        w4d_oracle_owner(state, 2, 102, 11);
        state.execute.alu_results[MEM_ISSUE_LANE] = w4d_oracle_result(state, 1, 10);
        state.execute.alu_results[INT_ISSUE_LANE] = w4d_oracle_result(state, 2, 11);
        } else if (operation == 1) {
        w4d_oracle_owner(state, 1, 201, 12);
        w4d_oracle_owner(state, 2, 202, 12);
        state.rob.tail = 3;
        state.execute.alu_results[MEM_ISSUE_LANE] = w4d_oracle_result(state, 1, 12);
        state.execute.alu_results[INT_ISSUE_LANE] = w4d_oracle_result(state, 2, 13);
        } else if (operation == 2) {
        w4d_oracle_owner(state, 1, 301, 0);
        w4d_oracle_owner(state, 2, 302, 13);
        state.branch_state.active_mask = 1;
        state.branch_state.tag_valid[0] = true;
        state.execute.alu_results[INT_ISSUE_LANE] = w4d_oracle_result(state, 1, 0);
        state.execute.alu_results[INT_ISSUE_LANE].uop.branch.is_br = true;
        state.execute.alu_results[INT_ISSUE_LANE].uop.branch.br_tag = 0;
        state.execute.alu_results[MEM_ISSUE_LANE] = w4d_oracle_result(state, 2, 13);
        } else if (operation == 3) {
        w4d_oracle_owner(state, 1, 401, 14);
        w4d_oracle_owner(state, 2, 402, 15);
        w4d_oracle_owner(state, 3, 403, 16);
        state.completion.load_response.valid = true;
        state.completion.load_response.kind = COMPLETION_EXECUTE;
        state.completion.load_response.source = COMPLETION_SOURCE_LSU_LOAD;
        state.completion.load_response.uop = state.rob.entries[1].uop;
        state.completion.load_response.writes_prf = true;
        state.completion.load_response.value = 14;
        state.execute.alu_results[MEM_ISSUE_LANE] = w4d_oracle_result(state, 2, 15);
        state.execute.alu_results[INT_ISSUE_LANE] = w4d_oracle_result(state, 3, 16);
        } else if (operation == 4 || operation == 6) {
        w4d_oracle_owner(state, 1, 501, 24);
        w4d_oracle_owner(state, 2, 502, 0);
        w4d_oracle_owner(state, 3, 503, 24);
        state.rob.tail = 4;
        state.branch_state.active_mask = 1;
        state.branch_state.tag_valid[0] = true;
        state.branch_state.snapshot_valid[0] = true;
        state.completion.load_response.valid = true;
        state.completion.load_response.kind = COMPLETION_EXECUTE;
        state.completion.load_response.source = COMPLETION_SOURCE_LSU_LOAD;
        state.completion.load_response.uop = state.rob.entries[1].uop;
        state.completion.load_response.writes_prf = true;
        state.completion.load_response.value = 1;
        state.execute.alu_results[INT_ISSUE_LANE] = w4d_oracle_result(state, 2, 0);
        state.execute.alu_results[INT_ISSUE_LANE].uop.branch.is_br = true;
        state.execute.alu_results[INT_ISSUE_LANE].uop.branch.br_tag = 0;
        state.execute.alu_results[INT_ISSUE_LANE].mispredict = operation == 6;
        state.execute.alu_results[MEM_ISSUE_LANE] = w4d_oracle_result(state, 3, 2);
        state.execute.alu_results[MEM_ISSUE_LANE].uop.branch.br_mask = 1;
        } else if (operation == 5) {
        w4d_oracle_owner(state, 1, 601, 17);
        w4d_oracle_owner(state, 2, 602, 17);
        w4d_oracle_owner(state, 3, 603, 18);
        state.completion.load_response.valid = true;
        state.completion.load_response.kind = COMPLETION_EXECUTE;
        state.completion.load_response.source = COMPLETION_SOURCE_LSU_LOAD;
        state.completion.load_response.uop = state.rob.entries[1].uop;
        state.completion.load_response.writes_prf = true;
        state.completion.load_response.value = 1;
        state.execute.alu_results[MEM_ISSUE_LANE] = w4d_oracle_result(state, 2, 2);
        state.execute.alu_results[INT_ISSUE_LANE] = w4d_oracle_result(state, 3, 3);
        } else if (operation == 7) {
        w4d_oracle_owner(state, 1, 701, 19);
        w4d_oracle_owner(state, 2, 702, 19);
        w4d_oracle_owner(state, 3, 703, 20);
        state.completion.load_response.valid = true;
        state.completion.load_response.kind = COMPLETION_EXECUTE;
        state.completion.load_response.source = COMPLETION_SOURCE_LSU_LOAD;
        state.completion.load_response.uop = state.rob.entries[1].uop;
        state.completion.load_response.writes_prf = true;
        state.completion.load_response.value = 1;
        state.execute.alu_results[MEM_ISSUE_LANE] = w4d_oracle_result(state, 2, 2);
        state.execute.alu_results[INT_ISSUE_LANE] = w4d_oracle_result(state, 3, 3);
        boom::completion_service_execute(state);
        boom::completion_service_execute(state);
        } else if (operation >= 8 && operation <= 13) {
        w4d_oracle_owner(state, 1, 801, 21);
        w4d_oracle_owner(state, 2, 802, operation == 9 ? 0 : 22);
        state.rob.tail = 3;
        if (operation == 8 || operation == 9) {
            state.completion.load_response.valid = true;
            state.completion.load_response.kind = COMPLETION_EXECUTE;
            state.completion.load_response.source = COMPLETION_SOURCE_LSU_LOAD;
            state.completion.load_response.uop = state.rob.entries[1].uop;
            state.completion.load_response.writes_prf = true;
            state.completion.load_response.value = 21;
        } else {
            state.execute.alu_results[MEM_ISSUE_LANE] = w4d_oracle_result(state, 1, 21);
            state.execute.alu_results[MEM_ISSUE_LANE].memory_valid = true;
            state.execute.alu_results[MEM_ISSUE_LANE].is_store = true;
        }
        state.execute.alu_results[INT_ISSUE_LANE] = w4d_oracle_result(state, 2, 22);
        if (operation == 9) {
            state.branch_state.active_mask = 1;
            state.branch_state.tag_valid[0] = true;
            state.execute.alu_results[INT_ISSUE_LANE].uop.branch.is_br = true;
            state.execute.alu_results[INT_ISSUE_LANE].uop.branch.br_tag = 0;
        }
        if (operation == 13)
            state.execute.alu_results[MEM_ISSUE_LANE] = ExecuteState::AluResult();
        } else if (operation == 14) {
        state.tohost = 1;
        state.rob.commit_valid = true;
        } else if (operation == 15) {
        w4d_oracle_owner(state, 1, 1001, 25);
        w4d_oracle_owner(state, 2, 1002, 26);
        state.execute.alu_results[MEM_ISSUE_LANE] = w4d_oracle_result(state, 1, 25);
        state.execute.alu_results[INT_ISSUE_LANE] = w4d_oracle_result(state, 2, 26);
        boom::completion_service_execute(state);
        w4d_oracle_owner(state, 3, 1003, 27);
        w4d_oracle_owner(state, 4, 1004, 28);
        state.execute.alu_results[MEM_ISSUE_LANE] = w4d_oracle_result(state, 3, 27);
        state.execute.alu_results[INT_ISSUE_LANE] = w4d_oracle_result(state, 4, 28);
        } else if (operation == 16) {
        state.rob.tail = 5;
        w4d_oracle_owner(state, 1, 1101, 14);
        w4d_oracle_owner(state, 2, 1102, 15);
        w4d_oracle_owner(state, 3, 1103, 16);
        w4d_oracle_owner(state, 4, 1104, 17);
        state.completion.load_response.valid = true;
        state.completion.load_response.kind = COMPLETION_EXECUTE;
        state.completion.load_response.source = COMPLETION_SOURCE_LSU_LOAD;
        state.completion.load_response.uop = state.rob.entries[1].uop;
        state.completion.load_response.writes_prf = true;
        state.completion.load_response.value = 0x1414;
        state.execute.alu_results[MEM_ISSUE_LANE] = w4d_oracle_result(state, 2, 0x1515);
        state.execute.alu_results[INT_ISSUE_LANE] = w4d_oracle_result(state, 3, 0x1616);
        RobEntry& load = state.rob.entries[4];
        load.is_load = load.memory_valid = load.memory_request_sent = true;
        load.memory_size = 3; load.memory_mask = 0xff; load.memory_transaction_id = 77;
        state.lsu.load_response_pending = true;
        state.lsu.pending_load_transaction_id = 77;
        state.lsu.pending_load_rob_idx = 4;
        state.lsu.pending_load_allocation_id = 1104;
        state.lsu.ldq_count = 1; state.lsu.ldq_tail = 1; state.lsu.ldq[0].valid = true;
        state.lsu.ldq[0].rob_idx = 4; state.lsu.ldq[0].rob_allocation_id = 1104;
        DmemResponse response; response.transaction_id = 77;
        response.data = response.read_data = 0x1717;
        state.completion.total_completion_accepts = 40;
        state.completion.total_prf_writes = 40;
        state.completion.total_wakeups = 40;
        boom::completion_service_execute(state);
        production_retained = !state.completion.load_response.valid &&
                              state.completion.int_execute.valid;
        production_first_limits = state.completion.prf_writes_this_cycle == 2 &&
                                  state.completion.wakeups_this_cycle == 3;
        production_single_reset = state.completion.total_completion_accepts == 42 &&
                                  state.completion.total_prf_writes == 42 &&
                                  state.completion.total_wakeups == 43;
        boom::completion_from_load_response(state, response,
                                            state.completion.load_response);
        boom::completion_service_execute(state);
        production_consumed = !state.completion.load_response.valid &&
                              !state.completion.int_execute.valid &&
                              !state.lsu.load_response_pending &&
                              state.completion.completion_accepts_this_cycle == 2 &&
                              state.completion.prf_writes_this_cycle == 2;
        production_values = boom::prf_read(state, 14) == 0x1414 &&
                            boom::prf_read(state, 15) == 0x1515 &&
                            boom::prf_read(state, 16) == 0x1616 &&
                            boom::prf_read(state, 17) == 0x1717;
        production_peaks = state.completion.peak_prf_writes == 2 &&
                           state.completion.peak_wakeups == 3;
        }
    }
    uint8_t operation = scenario & 0x7f;
    if (operation == 16) {
        *observable = production_retained |
            (uint64_t)production_first_limits << 1 |
            (uint64_t)production_single_reset << 2 |
            (uint64_t)production_consumed << 3 |
            (uint64_t)production_values << 4 |
            (uint64_t)production_peaks << 5;
        return;
    }
    boom::completion_service_execute(state);
    uint8_t physical_writes = state.completion.writebacks[0].valid +
                              state.completion.writebacks[1].valid;
    uint8_t physical_wakeups = 0;
    for (int i = 0; i < NUM_INT_WAKEUP_PORTS; i++)
        physical_wakeups += state.completion.wakeups[i].valid;
    uint64_t packed = physical_writes;
    packed |= (uint64_t)physical_wakeups << 4;
    packed |= (uint64_t)state.completion.writeback_fault_valid << 8;
    packed |= (uint64_t)state.io_trap << 9;
    packed |= (uint64_t)state.completion.mem_execute.valid << 10;
    packed |= (uint64_t)state.completion.int_execute.valid << 11;
    packed |= (uint64_t)state.completion.load_response.valid << 12;
    packed |= (uint64_t)state.rob.entries[2].busy << 13;
    packed |= (uint64_t)(boom::prf_read(state, 10) == 10) << 16;
    packed |= (uint64_t)(boom::prf_read(state, 11) == 11) << 17;
    packed |= (uint64_t)(boom::prf_read(state, 13) == 13) << 18;
    packed |= (uint64_t)(boom::prf_read(state, 14) == 14) << 19;
    packed |= (uint64_t)(boom::prf_read(state, 15) == 15) << 20;
    packed |= (uint64_t)(boom::prf_read(state, 16) == 16) << 21;
    packed |= (uint64_t)state.brupdate.valid << 22;
    packed |= (uint64_t)state.brupdate.mispredict << 23;
    packed |= (uint64_t)(boom::prf_read(state, 24) == 1) << 24;
    packed |= (uint64_t)state.rob.entries[3].valid << 25;
    packed |= (uint64_t)(state.completion.total_wakeups == 0) << 26;
    packed |= (uint64_t)(state.completion.total_bypass == 0) << 27;
    packed |= (uint64_t)state.rob.entries[3].busy << 28;
    packed |= (uint64_t)(boom::prf_read(state, 20) == 0) << 29;
    packed |= (uint64_t)(boom::prf_read(state, 21) == 21) << 30;
    packed |= (uint64_t)(boom::prf_read(state, 22) == 22) << 31;
    uint64_t lookup_value = 0;
    bool lookup_conflict = false;
    if (operation == 11)
        packed |= (uint64_t)boom::wakeup_lookup(state, 22, lookup_value,
                                                lookup_conflict) << 32;
    if (operation == 12)
        packed |= (uint64_t)boom::bypass_lookup(state, 22, lookup_value,
                                                lookup_conflict) << 33;
    packed |= (uint64_t)(state.completion.rob_completes_this_cycle & 3) << 34;
    packed |= (uint64_t)(state.tohost == 1) << 36;
    packed |= (uint64_t)state.rob.commit_valid << 37;
    packed |= (uint64_t)(state.completion.total_rob_completes == 4) << 38;
    packed |= (uint64_t)(state.completion.total_prf_writes == 4) << 39;
    *observable = packed;
}

static uint64_t w4_retention_read_prf(const BoomCoreState& state,
                                      uint8_t pdst) {
#pragma HLS INLINE off
    return boom::prf_read(state, pdst);
}

void synth_w4_core_step_retention_top(uint8_t seed, uint8_t phase,
                                      uint64_t& observable) {
    static BoomCoreState state;
    static PipeSignals pipe;
    static uint64_t staged_observable;
    static uint64_t staged_value;
    static uint8_t staged_first_pdst;
#pragma HLS STREAM variable=pipe.dmem_resp depth=2
#pragma HLS STREAM variable=pipe.imem_resp depth=2
#pragma HLS STREAM variable=pipe.imem_req depth=2
#pragma HLS STREAM variable=pipe.dmem_req depth=2
#pragma HLS STREAM variable=pipe.commit_trace depth=2
    if (phase != 0) {
        uint64_t read0 = w4_retention_read_prf(state, staged_first_pdst);
        uint64_t read1 = w4_retention_read_prf(state, staged_first_pdst + 1);
        uint64_t read2 = w4_retention_read_prf(state, staged_first_pdst + 2);
        uint64_t read3 = w4_retention_read_prf(state, staged_first_pdst + 3);
        bool values = read0 == staged_value + 1 && read1 == staged_value + 2 &&
                      read2 == staged_value + 3 && read3 == staged_value + 4;
        observable = staged_observable | (uint64_t)values << 7;
        return;
    }

    state = BoomCoreState();
    uint32_t allocation = 1200 + seed;
    uint64_t value = 0x2000 + seed;

    state.rob.head = 0;
    state.rob.tail = 5;
    w4d_oracle_owner(state, 0, allocation, 0);
    w4d_oracle_owner(state, 1, allocation + 1, 14);
    w4d_oracle_owner(state, 2, allocation + 2, 15);
    w4d_oracle_owner(state, 3, allocation + 3, 16);
    w4d_oracle_owner(state, 4, allocation + 4, 17);

    state.completion.load_response.valid = true;
    state.completion.load_response.kind = COMPLETION_LOAD_RESPONSE;
    state.completion.load_response.source = COMPLETION_SOURCE_LSU_LOAD;
    state.completion.load_response.uop = state.rob.entries[1].uop;
    state.completion.load_response.writes_prf = true;
    state.completion.load_response.value = value + 1;
    state.completion.load_response.transaction_id = 70 + seed;
    state.rob.entries[1].is_load = true;
    state.rob.entries[1].memory_valid = true;
    state.rob.entries[1].memory_request_sent = true;
    state.rob.entries[1].memory_size = 3;
    state.rob.entries[1].memory_mask = 0xff;
    state.rob.entries[1].memory_transaction_id = 70 + seed;
    state.execute.alu_results[MEM_ISSUE_LANE] =
        w4d_oracle_result(state, 2, value + 2);
    state.execute.alu_results[INT_ISSUE_LANE] =
        w4d_oracle_result(state, 3, value + 3);

    state.rob.entries[4].is_load = true;
    state.rob.entries[4].memory_valid = true;
    state.rob.entries[4].memory_request_sent = true;
    state.rob.entries[4].memory_size = 3;
    state.rob.entries[4].memory_mask = 0xff;
    state.rob.entries[4].memory_transaction_id = 80 + seed;
    state.lsu.load_response_pending = true;
    state.lsu.pending_load_transaction_id = 80 + seed;
    state.lsu.pending_load_rob_idx = 4;
    state.lsu.pending_load_allocation_id = allocation + 4;
    state.lsu.ldq_count = 1;
    state.lsu.ldq_tail = 1;
    state.lsu.ldq[0].valid = true;
    state.lsu.ldq[0].rob_idx = 4;
    state.lsu.ldq[0].rob_allocation_id = allocation + 4;

    DmemResponse queued;
    queued.transaction_id = 80 + seed;
    queued.data = queued.read_data = value + 4;
    pipe.dmem_resp.write(queued);
    ImemResponse ignored_imem;
    ignored_imem.fetch_id = 0xff;
    ignored_imem.address = RESET_VECTOR;
    pipe.imem_resp.write(ignored_imem);
    pipe.imem_resp.write(ignored_imem);

    boom_core_step(state, pipe);
    bool first_limits = state.completion.completion_accepts_this_cycle == 2 &&
                        state.completion.rob_completes_this_cycle == 2 &&
                        state.completion.prf_writes_this_cycle == 2;
    bool first_queued = !pipe.dmem_resp.empty();
    bool first_retained = state.completion.int_execute.valid &&
                          !state.completion.load_response.valid;
    bool first_lsu_owned = state.lsu.load_response_pending;
    bool first_wakeups = state.completion.wakeups_this_cycle == 3;
    if (!pipe.imem_req.empty()) pipe.imem_req.read();
    if (!pipe.dmem_req.empty()) pipe.dmem_req.read();
    if (!pipe.commit_trace.empty()) pipe.commit_trace.read();

    boom_core_step(state, pipe);
    bool second_consumed = pipe.dmem_resp.empty() &&
                           !state.completion.load_response.valid &&
                           !state.completion.mem_execute.valid &&
                           !state.completion.int_execute.valid &&
                           !state.lsu.load_response_pending;
    bool second_limits = state.completion.completion_accepts_this_cycle == 2 &&
                         state.completion.rob_completes_this_cycle == 2 &&
                         state.completion.prf_writes_this_cycle == 2;
    bool totals = state.completion.total_completion_accepts == 4 &&
                  state.completion.total_rob_completes == 4 &&
                  state.completion.total_prf_writes == 4;
    bool integrity = state.completion.dropped_completions == 0 &&
                     state.completion.dropped_writebacks == 0 &&
                     state.completion.duplicate_writebacks == 0;
    bool second_wakeups = state.completion.wakeups_this_cycle == 2;

    staged_value = value;
    staged_first_pdst = 13 + (seed & 1);
    staged_observable = first_limits |
        (uint64_t)first_queued << 1 |
        (uint64_t)first_retained << 2 |
        (uint64_t)first_lsu_owned << 3 |
        (uint64_t)first_wakeups << 4 |
        (uint64_t)second_consumed << 5 |
        (uint64_t)second_limits << 6 |
        (uint64_t)totals << 8 |
        (uint64_t)integrity << 9 |
        (uint64_t)second_wakeups << 10;
    observable = staged_observable;
}
