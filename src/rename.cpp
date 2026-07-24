#include "boom_config.hpp"
#include "boom_types.hpp"
#include "boom_state.hpp"

namespace boom {

void rename_module(BoomCoreState& state) {
    RenameState& ren = state.rename;
    RenameMapTableState& mt = ren.int_map_table;
    RenameFreeListState& fl = ren.int_free_list;

    for (int i=0; i<DISPATCH_WIDTH; i++) {
        ren.renamed_valids[i]=false; ren.renamed_uops[i]=MicroOp();
    }
    if (state.global_flush) return;

    if (state.brupdate.valid && state.brupdate.mispredict) {
        for (int i=0; i<LOGICAL_REG_COUNT; i++) {
            mt.map_table[i] = mt.committed_map_table[i];
            for (int j=0; j<MAX_BRANCH_COUNT; j++) mt.br_snapshots[i][j]=mt.committed_map_table[i];
        }
    }

    for (int i=0; i<DISPATCH_WIDTH; i++) {
        if (!state.decode.dec_valids[i]) continue;
        MicroOp uop = state.decode.dec_uops[i];

        uop.rename.prs1 = mt.map_table[uop.rename.lrs1];
        uop.rename.prs2 = mt.map_table[uop.rename.lrs2];
        uop.rename.stale_pdst = mt.map_table[uop.rename.ldst];

        uop.rename.prs1_busy = false;
        uop.rename.prs2_busy = false;

        if (uop.rename.dst_rtype == DST_INT && uop.rename.ldst != 0) {
            if (fl.count > 0) {
                uint8_t pdst = fl.free_list[fl.head];
                fl.head = (fl.head+1)%INT_PHYS_REGS; fl.count--;
                fl.busy_table[pdst] = true;
                uop.rename.pdst = pdst;
                mt.map_table[uop.rename.ldst] = pdst;
            }
        } else { uop.rename.pdst = 0; uop.rename.ldst = 0; }

        ren.renamed_uops[i] = uop;
        ren.renamed_valids[i] = true;
    }
}

}
