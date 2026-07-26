#include "boom_config.hpp"
#include "boom_types.hpp"
#include "boom_state.hpp"
#include "boom_interfaces.hpp"

namespace boom {

extern bool rob_branch_kill(BoomCoreState& state);
extern void rob_complete(BoomCoreState& state);

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

void rob_commit_module(BoomCoreState& state, PipeSignals& pipe) {
    RobInternalState& rob = state.rob;
    rob.commit_valid = false;

    if (rob_branch_kill(state)) return;
    rob_complete(state);

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
                    free_preg_unique(fl, uop.rename.stale_pdst);
                }
            }
            if (!pipe.commit_trace.full()) pipe.commit_trace.write(ce);
            rob.last_commit=ce; rob.commit_valid=true;
            he.valid=false; rob.head=(rob.head+1)%ROB_DEPTH; rob.maybe_full=false;
        }
    }
}

void commit_module_stub(BoomCoreState& state) { (void)state; }
}
