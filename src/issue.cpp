#include "boom_config.hpp"
#include "boom_types.hpp"
#include "boom_state.hpp"
#include "issue.hpp"

IssuePortClass classify_issue_port(const MicroOp& uop) {
    if (uop.exception) return ISSUE_PORT_UNSUPPORTED;
    if (uop.iq_type == IQ_MEM && uop.fu_code == FU_MEM && uop.uopc >= 39 && uop.uopc <= 49)
        return ISSUE_PORT_MEM;
    if (uop.iq_type != IQ_ALU) return ISSUE_PORT_UNSUPPORTED;
    if (uop.fu_code == FU_MUL) return uop.uopc == 16 ? ISSUE_PORT_INT : ISSUE_PORT_UNSUPPORTED;
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
    IssueState& iss = state.issue;
    const RenameState& ren = state.rename;

    for (int i=0; i<ISSUE_WIDTH; i++) {
        iss.grants[i]=IssueGrant(); iss.issued_valids[i]=false; iss.issued_uops[i]=MicroOp();
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
        if (s.uop.rename.prs1 != 0) s.prs1_busy = preg_busy(state, s.uop.rename.prs1);
        if (s.uop.rename.prs2 != 0) s.prs2_busy = preg_busy(state, s.uop.rename.prs2);
    }
    compact_iq(iss.alu_iq);

    int mem_index=-1, int_index=-1;
    for (int i=0; i<ISSUE_QUEUE_ALU_DEPTH; i++) {
        IssueSlotEntry& s = iss.alu_iq.entries[i];
        if (!s.valid || !s.request || s.killed || s.prs1_busy || s.prs2_busy || s.pdst_busy) continue;
        IssuePortClass port=classify_issue_port(s.uop);
        if (port==ISSUE_PORT_MEM && mem_index<0) mem_index=i;
        else if (port==ISSUE_PORT_INT && int_index<0) int_index=i;
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
    if (ren.renamed_valids[0]) {
        dispatch_uop=ren.renamed_uops[0];
        dispatch_pending=!dispatch_uop.exception;
        IssuePortClass port=classify_issue_port(dispatch_uop);
        int lane=(port==ISSUE_PORT_MEM) ? MEM_ISSUE_LANE : (port==ISSUE_PORT_INT ? INT_ISSUE_LANE : -1);
        bool dispatch_ready=!dispatch_uop.rename.prs1_busy && !dispatch_uop.rename.prs2_busy;
        bool old_grant_can_accept=false;
        for (int old_lane=0; old_lane<INTEGER_ISSUE_PORTS; old_lane++)
            old_grant_can_accept |= iss.grants[old_lane].valid && iss.port_ready[old_lane];
        bool dispatch_can_be_preserved=iss.alu_iq.count<ISSUE_QUEUE_ALU_DEPTH ||
                                       (lane>=0 && iss.port_ready[lane]) || old_grant_can_accept;
        if (lane>=0 && dispatch_ready && dispatch_can_be_preserved && !iss.grants[lane].valid) {
            iss.grants[lane].valid=true; iss.grants[lane].from_dispatch=true;
            iss.grants[lane].entry_index=0xff; iss.grants[lane].port_class=(uint8_t)port;
            iss.grants[lane].uop=dispatch_uop;
        }
    }

    for (int lane=0; lane<INTEGER_ISSUE_PORTS; lane++) {
        if (!iss.grants[lane].valid) continue;
        iss.grants_generated++; iss.total_grants++;
        if (lane==MEM_ISSUE_LANE) iss.mem_grants++; else iss.int_grants++;
    }
    if (iss.grants_generated==0) iss.cycles_with_0_grant++;
    else if (iss.grants_generated==1) iss.cycles_with_1_grant++;
    else iss.cycles_with_2_grants++;

    int accepted_lane=-1;
    for (int lane=0; lane<INTEGER_ISSUE_PORTS; lane++) {
        if (!iss.grants[lane].valid || !iss.port_ready[lane]) continue;
        if (accepted_lane<0 || iss.grants[lane].entry_index < iss.grants[accepted_lane].entry_index)
            accepted_lane=lane;
    }
    if (accepted_lane>=0) {
        IssueGrant& grant=iss.grants[accepted_lane];
        grant.accepted=true; iss.grants_accepted=1;
        iss.issued_valids[0]=true; iss.issued_uops[0]=grant.uop;
        if (grant.from_dispatch) dispatch_pending=false;
        else {
            IssueSlotEntry& selected=iss.alu_iq.entries[grant.entry_index];
            selected.granted=true; selected.request=false;
        }
    }
    iss.grants_retained=(uint8_t)(iss.grants_generated-iss.grants_accepted);
    if (iss.grants_retained) { iss.grant_stalls+=iss.grants_retained; iss.execute_acceptance_stalls+=iss.grants_retained; }

    compact_iq(iss.alu_iq);

    if (dispatch_pending && iss.alu_iq.count<ISSUE_QUEUE_ALU_DEPTH) {
        IssueSlotEntry& slot=iss.alu_iq.entries[iss.alu_iq.tail];
        slot.valid=true; slot.request=true; slot.granted=false; slot.killed=false;
        slot.uop=dispatch_uop; slot.prs1_busy=dispatch_uop.rename.prs1_busy;
        slot.prs2_busy=dispatch_uop.rename.prs2_busy; slot.pdst_busy=false;
        iss.alu_iq.tail=(iss.alu_iq.tail+1)%ISSUE_QUEUE_ALU_DEPTH; iss.alu_iq.count++;
    }
}

}
