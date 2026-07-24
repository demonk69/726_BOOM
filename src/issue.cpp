#include "boom_config.hpp"
#include "boom_types.hpp"
#include "boom_state.hpp"

namespace boom {

void issue_module(BoomCoreState& state) {
    IssueState& iss = state.issue;
    const RenameState& ren = state.rename;

    for (int i=0; i<ISSUE_WIDTH; i++) { iss.issued_valids[i]=false; iss.issued_uops[i]=MicroOp(); }

    if (state.global_flush) {
        for (int j=0; j<ISSUE_QUEUE_ALU_DEPTH; j++) iss.alu_iq.entries[j].valid=false;
        iss.alu_iq.count=0; iss.alu_iq.head=0; iss.alu_iq.tail=0; return;
    }

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

    IssueSlotEntry temp[ISSUE_QUEUE_ALU_DEPTH];
    int w=0;
    for (int i=0; i<ISSUE_QUEUE_ALU_DEPTH; i++) {
        if (iss.alu_iq.entries[i].valid && !iss.alu_iq.entries[i].granted)
            temp[w++]=iss.alu_iq.entries[i];
    }
    for (int i=w; i<ISSUE_QUEUE_ALU_DEPTH; i++) temp[i].valid=false;
    for (int i=0; i<ISSUE_QUEUE_ALU_DEPTH; i++) iss.alu_iq.entries[i]=temp[i];
    iss.alu_iq.tail=w%ISSUE_QUEUE_ALU_DEPTH; iss.alu_iq.count=w;
}

}
