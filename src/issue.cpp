#include "boom_config.hpp"
#include "boom_types.hpp"
#include "boom_state.hpp"
#include "issue.hpp"
#include "completion.hpp"

IssuePortClass classify_issue_port(const MicroOp& uop) {
    if (uop.exception) return ISSUE_PORT_UNSUPPORTED;
    if (uop.iq_type == IQ_MEM && uop.fu_code == FU_MEM && uop.uopc >= 39 && uop.uopc <= 49)
        return ISSUE_PORT_MEM;
    if (uop.iq_type != IQ_ALU) return ISSUE_PORT_UNSUPPORTED;
    if (uop.fu_code == FU_MUL)
        return uop.uopc >= 16 && uop.uopc <= 20 ? ISSUE_PORT_INT : ISSUE_PORT_UNSUPPORTED;
    if (uop.fu_code == FU_DIV)
        return uop.uopc >= 21 && uop.uopc <= 28 ? ISSUE_PORT_INT : ISSUE_PORT_UNSUPPORTED;
    if (uop.fu_code != FU_ALU) return ISSUE_PORT_UNSUPPORTED;
    if ((uop.uopc >= 1 && uop.uopc <= 13) || uop.uopc == 15 ||
        (uop.uopc >= 29 && uop.uopc <= 38) ||
        (uop.uopc >= 50 && uop.uopc <= 60) || uop.uopc == 62)
        return ISSUE_PORT_INT;
    return ISSUE_PORT_UNSUPPORTED;
}

namespace boom {

static bool killed_by_branch(const MicroOp& uop, uint8_t mask) {
    return (uop.branch.br_mask & mask) != 0;
}

static bool preg_busy(const BoomCoreState& state, uint8_t preg) {
    return preg != 0 && preg < INT_PHYS_REGS && state.rename.int_free_list.busy_table[preg];
}

static bool resolve_operand(const BoomCoreState& state, uint8_t prs,
                            uint64_t& data) {
#pragma HLS INLINE
    if (prs == 0) { data = 0; return true; }
    bool conflict = false;
    if (boom::wakeup_lookup(state, prs, data, conflict)) return true;
    if (conflict || preg_busy(state, prs)) return false;
    data = prf_read(state, prs);
    return true;
}

static bool divider_uop_ready(const BoomCoreState& state, const MicroOp& uop) {
#pragma HLS INLINE
    if (uop.fu_code != FU_DIV) return true;
    if (uop.uopc < 21 || uop.uopc > 28 || state.execute.divider.token_valid ||
        !divider_request_ready(state.execute.divider.arithmetic)) return false;
    uint8_t rob_idx = uop.queue.rob_idx;
    return uop.queue.rob_allocation_id != 0 && rob_idx < ROB_DEPTH &&
        state.rob.entries[rob_idx].valid && state.rob.entries[rob_idx].busy &&
        state.rob.entries[rob_idx].uop.queue.rob_allocation_id ==
            uop.queue.rob_allocation_id;
}

static void compact_iq(IssueQueueState& iq) {
    IssueSlotEntry temp[ISSUE_QUEUE_ALU_DEPTH];
    int w=0;
    for (int i=0; i<ISSUE_QUEUE_ALU_DEPTH; i++) {
        if (iq.entries[i].valid && !iq.entries[i].granted) temp[w++]=iq.entries[i];
    }
    for (int i=w; i<ISSUE_QUEUE_ALU_DEPTH; i++) temp[i]=IssueSlotEntry();
    for (int i=0; i<ISSUE_QUEUE_ALU_DEPTH; i++) iq.entries[i]=temp[i];
    iq.head=0; iq.tail=(uint8_t)(w%ISSUE_QUEUE_ALU_DEPTH); iq.count=(uint8_t)w;
}

void issue_module(BoomCoreState& state) {
#ifdef BOOM_HLS_W3_DIAGNOSTIC
#pragma HLS INLINE
#endif
    IssueState& iss = state.issue;
    RenameState& ren = state.rename;

    for (int i=0; i<ISSUE_WIDTH; i++) {
        iss.grants[i]=IssueGrant(); iss.issued_valids[i]=false; iss.issued_uops[i]=MicroOp();
        iss.issued_prs1_data[i]=iss.issued_prs2_data[i]=iss.issued_prs3_data[i]=0;
    }
    iss.grants_generated=0; iss.grants_accepted=0; iss.grants_retained=0; iss.grants_dropped=0;

    if (state.global_flush) {
        for (int j=0; j<ISSUE_QUEUE_ALU_DEPTH; j++) iss.alu_iq.entries[j].valid=false;
        iss.alu_iq.count=0; iss.alu_iq.head=0; iss.alu_iq.tail=0; return;
    }

    for (int j=0; j<ISSUE_QUEUE_ALU_DEPTH; j++) {
        IssueSlotEntry& s = iss.alu_iq.entries[j];
        if (!s.valid) continue;
        if (state.brupdate.valid && state.brupdate.mispredict && killed_by_branch(s.uop, state.brupdate.mispredict_mask)) {
            s.valid = false;
            s.killed = true;
            continue;
        }
        if (s.uop.exception) {
            s.valid = false;
            s.killed = true;
            continue;
        }
        if (state.brupdate.valid) s.uop.branch.br_mask &= (uint8_t)~state.brupdate.resolve_mask;
        s.prs1_busy = !resolve_operand(state, s.uop.rename.prs1, s.prs1_data);
        s.prs2_busy = !resolve_operand(state, s.uop.rename.prs2, s.prs2_data);
        s.prs3_busy = !resolve_operand(state, s.uop.rename.prs3, s.prs3_data);
    }
    compact_iq(iss.alu_iq);

    int mem_index=-1, int_index=-1;
    for (int i=0; i<ISSUE_QUEUE_ALU_DEPTH; i++) {
        IssueSlotEntry& s = iss.alu_iq.entries[i];
        if (!s.valid || !s.request || s.killed || s.prs1_busy || s.prs2_busy ||
            s.prs3_busy || s.pdst_busy) continue;
        IssuePortClass port=classify_issue_port(s.uop);
        if (port==ISSUE_PORT_MEM && mem_index<0) mem_index=i;
        else if (port==ISSUE_PORT_INT && int_index<0 && divider_uop_ready(state, s.uop)) int_index=i;
    }

    if (mem_index>=0) {
        iss.grants[MEM_ISSUE_LANE].valid=true;
        iss.grants[MEM_ISSUE_LANE].entry_index=(uint8_t)mem_index;
        iss.grants[MEM_ISSUE_LANE].port_class=ISSUE_PORT_MEM;
        iss.grants[MEM_ISSUE_LANE].uop=iss.alu_iq.entries[mem_index].uop;
    }
    if (int_index>=0) {
        iss.grants[INT_ISSUE_LANE].valid=true;
        iss.grants[INT_ISSUE_LANE].entry_index=(uint8_t)int_index;
        iss.grants[INT_ISSUE_LANE].port_class=ISSUE_PORT_INT;
        iss.grants[INT_ISSUE_LANE].uop=iss.alu_iq.entries[int_index].uop;
    }

    bool dispatch_pending=false;
    MicroOp dispatch_uop;
    RenameDispatchPacket& dispatch_packet = ren.dispatch_packets[0];
    if (dispatch_packet.valid && dispatch_packet.rob_allocated) {
        dispatch_uop=dispatch_packet.uop;
        dispatch_pending=!dispatch_uop.exception;
        IssuePortClass port=classify_issue_port(dispatch_uop);
        int lane=(port==ISSUE_PORT_MEM) ? MEM_ISSUE_LANE : (port==ISSUE_PORT_INT ? INT_ISSUE_LANE : -1);
        uint64_t dispatch_data1=0, dispatch_data2=0, dispatch_data3=0;
        bool dispatch_ready=resolve_operand(state, dispatch_uop.rename.prs1, dispatch_data1) &&
                            resolve_operand(state, dispatch_uop.rename.prs2, dispatch_data2) &&
                            resolve_operand(state, dispatch_uop.rename.prs3, dispatch_data3);
        bool old_grant_can_accept=false;
        for (int old_lane=0; old_lane<INTEGER_ISSUE_PORTS; old_lane++)
            old_grant_can_accept |= iss.grants[old_lane].valid && iss.port_ready[old_lane];
        bool dispatch_can_be_preserved=iss.alu_iq.count<ISSUE_QUEUE_ALU_DEPTH ||
                                       (lane>=0 && iss.port_ready[lane]) || old_grant_can_accept;
        if (lane>=0 && dispatch_ready && divider_uop_ready(state, dispatch_uop) &&
            dispatch_can_be_preserved && !iss.grants[lane].valid) {
            iss.grants[lane].valid=true; iss.grants[lane].from_dispatch=true;
            iss.grants[lane].entry_index=0xff; iss.grants[lane].port_class=(uint8_t)port;
            iss.grants[lane].uop=dispatch_uop;
        }
    }

    if (dispatch_packet.valid && dispatch_packet.rob_allocated && dispatch_packet.uop.exception) {
        dispatch_packet = RenameDispatchPacket();
    }

    for (int lane=0; lane<INTEGER_ISSUE_PORTS; lane++) {
        if (!iss.grants[lane].valid) continue;
        iss.grants_generated++; iss.total_grants++;
        if (lane==MEM_ISSUE_LANE) iss.mem_grants++; else iss.int_grants++;
    }
    if (iss.grants_generated==0) iss.cycles_with_0_grant++;
    else if (iss.grants_generated==1) iss.cycles_with_1_grant++;
    else iss.cycles_with_2_grants++;

    for (int lane=0; lane<INTEGER_ISSUE_PORTS; lane++) {
        IssueGrant& grant=iss.grants[lane];
        bool downstream_ready=iss.port_ready[lane];
        if (lane==INT_ISSUE_LANE && grant.valid)
            downstream_ready &= divider_uop_ready(state, grant.uop);
        if (lane==MEM_ISSUE_LANE && grant.valid) {
            bool is_load=grant.uop.ctrl.is_load || grant.uop.mem.uses_ldq ||
                         (grant.uop.uopc>=39 && grant.uop.uopc<=45);
            bool is_store=grant.uop.ctrl.is_sta || grant.uop.mem.uses_stq ||
                          (grant.uop.uopc>=46 && grant.uop.uopc<=49);
            if (is_load) downstream_ready &= state.lsu.ldq_count<LDQ_DEPTH;
            if (is_store) downstream_ready &= state.lsu.stq_count<STQ_DEPTH;
        }
        if (!grant.valid || !downstream_ready) continue;
        grant.accepted=true; iss.grants_accepted++;
        iss.issued_valids[lane]=true; iss.issued_uops[lane]=grant.uop;
        if (grant.from_dispatch) {
            resolve_operand(state, grant.uop.rename.prs1, iss.issued_prs1_data[lane]);
            resolve_operand(state, grant.uop.rename.prs2, iss.issued_prs2_data[lane]);
            resolve_operand(state, grant.uop.rename.prs3, iss.issued_prs3_data[lane]);
            dispatch_pending=false;
            dispatch_packet = RenameDispatchPacket();
        }
        else {
            IssueSlotEntry& selected=iss.alu_iq.entries[grant.entry_index];
            iss.issued_prs1_data[lane]=selected.prs1_data;
            iss.issued_prs2_data[lane]=selected.prs2_data;
            iss.issued_prs3_data[lane]=selected.prs3_data;
            selected.granted=true; selected.request=false;
        }
    }
    iss.grants_retained=(uint8_t)(iss.grants_generated-iss.grants_accepted);
    if (iss.grants_retained) { iss.grant_stalls+=iss.grants_retained; iss.execute_acceptance_stalls+=iss.grants_retained; }

    compact_iq(iss.alu_iq);

    if (dispatch_pending && iss.alu_iq.count<ISSUE_QUEUE_ALU_DEPTH) {
        IssueSlotEntry& slot=iss.alu_iq.entries[iss.alu_iq.tail];
        slot.valid=true; slot.request=true; slot.granted=false; slot.killed=false;
        slot.uop=dispatch_uop;
        slot.prs1_busy=!resolve_operand(state, dispatch_uop.rename.prs1, slot.prs1_data);
        slot.prs2_busy=!resolve_operand(state, dispatch_uop.rename.prs2, slot.prs2_data);
        slot.prs3_busy=!resolve_operand(state, dispatch_uop.rename.prs3, slot.prs3_data);
        slot.pdst_busy=false;
        iss.alu_iq.tail=(iss.alu_iq.tail+1)%ISSUE_QUEUE_ALU_DEPTH; iss.alu_iq.count++;
        dispatch_packet = RenameDispatchPacket();
    }
}

}
