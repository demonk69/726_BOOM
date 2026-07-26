#include "boom_config.hpp"
#include "boom_types.hpp"
#include "boom_state.hpp"

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

    for (int i=0; i<ISSUE_WIDTH; i++) { iss.issued_valids[i]=false; iss.issued_uops[i]=MicroOp(); }

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
        if (state.brupdate.valid) s.uop.branch.br_mask &= (uint8_t)~state.brupdate.resolve_mask;
        if (s.uop.rename.prs1 != 0) s.prs1_busy = preg_busy(state, s.uop.rename.prs1);
        if (s.uop.rename.prs2 != 0) s.prs2_busy = preg_busy(state, s.uop.rename.prs2);
    }
    compact_iq(iss.alu_iq);

    for (int i=0; i<DISPATCH_WIDTH; i++) {
        if (!ren.renamed_valids[i]) continue;
        if (iss.alu_iq.count >= ISSUE_QUEUE_ALU_DEPTH) break;
        const MicroOp& uop = ren.renamed_uops[i];
        if (uop.iq_type != IQ_ALU && uop.iq_type != IQ_MEM) continue;
        IssueSlotEntry& slot = iss.alu_iq.entries[iss.alu_iq.tail];
        slot.valid=true; slot.request=true; slot.granted=false; slot.killed=false;
        slot.uop=uop; slot.prs1_busy=uop.rename.prs1_busy;
        slot.prs2_busy=uop.rename.prs2_busy; slot.pdst_busy=false;
        iss.alu_iq.tail=(iss.alu_iq.tail+1)%ISSUE_QUEUE_ALU_DEPTH; iss.alu_iq.count++;
    }

    int issued=0;
    const int implemented_alu_lanes = DISPATCH_WIDTH;
    for (int i=0; i<ISSUE_QUEUE_ALU_DEPTH && issued<ISSUE_WIDTH && issued<implemented_alu_lanes; i++) {
        IssueSlotEntry& s = iss.alu_iq.entries[i];
        if (s.valid && s.request && !s.killed && !s.prs1_busy && !s.prs2_busy && !s.pdst_busy) {
            s.granted=true; s.request=false;
            iss.issued_uops[issued]=s.uop; iss.issued_valids[issued]=true; issued++;
        }
    }

    compact_iq(iss.alu_iq);
}

}
