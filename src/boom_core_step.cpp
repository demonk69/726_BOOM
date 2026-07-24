#include "boom_config.hpp"
#include "boom_types.hpp"
#include "boom_state.hpp"
#include "boom_interfaces.hpp"

namespace boom {
extern void frontend_module(BoomCoreState& state, PipeSignals& pipe);
extern void decode_module(BoomCoreState& state);
extern void rename_module(BoomCoreState& state);
extern void issue_module(BoomCoreState& state);
extern void execute_module(BoomCoreState& state);
extern void branch_module(BoomCoreState& state);
extern void lsu_module(BoomCoreState& state, PipeSignals& pipe);
extern void csr_module(BoomCoreState& state);
static void rob_allocate(BoomCoreState& state);
void rob_commit_module(BoomCoreState& state, PipeSignals& pipe);

static void rob_allocate(BoomCoreState& state) {
    RobInternalState& rob = state.rob;
    if (rob.state == ROB_INIT) rob.state = ROB_NORMAL;
    if (state.global_flush) return;
    for (int i=0; i<DISPATCH_WIDTH; i++) {
        if (!state.rename.renamed_valids[i]) continue;
        bool is_full = (rob.head==rob.tail) && rob.maybe_full;
        if (is_full) { state.rename.renamed_valids[i]=false; break; }
        MicroOp uop = state.rename.renamed_uops[i];
        if (uop.uopc == 0) continue;
        RobEntry& entry = rob.entries[rob.tail];
        entry = RobEntry();
        entry.valid=true; entry.busy=true; entry.exception=uop.exception;
        uop.queue.rob_idx = rob.tail; entry.uop = uop;
        state.rename.renamed_uops[i].queue.rob_idx = rob.tail;
        rob.tail=(rob.tail+1)%ROB_DEPTH;
        if (rob.tail==rob.head) rob.maybe_full=true;
    }
}

void rob_commit_module(BoomCoreState& state, PipeSignals& pipe) {
    RobInternalState& rob = state.rob;
    rob.commit_valid = false;

    if (state.brupdate.valid && state.brupdate.mispredict) {
        for (int i=0; i<ROB_DEPTH; i++) {
            if (rob.entries[i].valid) { rob.entries[i].valid=false; rob.entries[i].busy=false; }
        }
        rob.head=0; rob.tail=0; rob.maybe_full=false; rob.state=ROB_NORMAL;
        state.global_flush = true; return;
    }
    for (int i=0; i<DISPATCH_WIDTH; i++) {
        if (!state.execute.alu_results[i].valid) continue;
        const ExecuteState::AluResult& r = state.execute.alu_results[i];
        if (r.memory_valid || r.is_load || r.is_store || r.uop.ctrl.is_load || r.uop.ctrl.is_sta) continue;
        uint8_t ridx = r.uop.queue.rob_idx;
        if (ridx<ROB_DEPTH && rob.entries[ridx].valid) {
            rob.entries[ridx].busy=false;
            if (r.exception) rob.entries[ridx].exception=true;
        }
    }
    bool is_empty = (rob.head==rob.tail) && !rob.maybe_full;
    if (!is_empty && rob.entries[rob.head].valid && !rob.entries[rob.head].busy) {
        RobEntry& he = rob.entries[rob.head];
        MicroOp& uop = he.uop;
        if (he.exception) {
            if (uop.uopc == 65) {
                uint8_t a0_pdst = state.rename.int_map_table.map_table[10];
                uint64_t a0_val = (a0_pdst!=0) ? state.int_rf[a0_pdst] : 0;
                if (a0_val==0) state.io_success = true;
                else state.io_trap = true;
                he.valid=false; rob.head=(rob.head+1)%ROB_DEPTH; rob.maybe_full=false;
            } else { rob.state = ROB_EXCEPTION; state.io_trap = true; }
        } else {
            if ((uop.ctrl.is_load || he.is_load) && !he.memory_completed) return;
            if (uop.ctrl.is_sta || he.is_store) {
                if (!he.memory_valid) return;
                if (!he.memory_request_sent) {
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
                }
            }
            state.csr.instret++;
            CommitEntry ce;
            ce.valid=true; ce.pc=uop.debug_pc; ce.inst=uop.inst;
            ce.rd_valid=(uop.rename.dst_rtype==DST_INT && uop.rename.pdst!=0);
            ce.rd=uop.rename.ldst; ce.priv=state.csr.priv;
            ce.exception=false; ce.branch_mispredict=false;
            ce.rd_value=ce.rd_valid ? state.int_rf[uop.rename.pdst] : 0;
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
                if (fl.count<INT_PHYS_REGS) {
                    fl.busy_table[uop.rename.stale_pdst]=false;
                    fl.free_list[fl.tail]=uop.rename.stale_pdst;
                    fl.tail=(fl.tail+1)%INT_PHYS_REGS; fl.count++;
                }
            }
            }
            if (!pipe.commit_trace.full()) pipe.commit_trace.write(ce);
            rob.last_commit=ce; rob.commit_valid=true;
            he.valid=false; rob.head=(rob.head+1)%ROB_DEPTH; rob.maybe_full=false;
        }
    }
}

}

void boom_core_step(BoomCoreState& state, PipeSignals& pipe) {
    BoomCoreState next_state = state;
    next_state.global_flush = false;
    next_state.io_success = next_state.io_trap = false;
    next_state.brupdate.valid = next_state.brupdate.mispredict = false;

    boom::csr_module(next_state);
    boom::branch_module(next_state);
    boom::lsu_module(next_state, pipe);
    boom::frontend_module(next_state, pipe);
    boom::decode_module(next_state);
    boom::rename_module(next_state);
    boom::rob_allocate(next_state);
    boom::issue_module(next_state);
    boom::execute_module(next_state);
    boom::rob_commit_module(next_state, pipe);

    state = next_state;
}
