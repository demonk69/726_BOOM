#include "boom_config.hpp"
#include "boom_types.hpp"
#include "boom_state.hpp"

namespace boom {

static uint8_t tag_bit(uint8_t tag) {
    return (tag < MAX_BRANCH_COUNT) ? (uint8_t)(1u << tag) : 0;
}

static bool is_control_uop(const MicroOp& uop) {
    return uop.branch.is_br || uop.branch.is_jal || uop.branch.is_jalr;
}

static bool allocate_branch_tag(BranchRecoveryState& br, uint8_t& tag) {
    if (br.active_mask == ((1u << MAX_BRANCH_COUNT) - 1u)) return false;
    for (int i=0; i<MAX_BRANCH_COUNT; i++) {
        if ((br.active_mask & tag_bit((uint8_t)i)) == 0) {
            tag = (uint8_t)i;
            return true;
        }
    }
    return false;
}

static bool allocate_preg(RenameFreeListState& fl, uint8_t& pdst) {
    if (fl.count == 0) return false;
    for (int i=0; i<INT_PHYS_REGS; i++) {
        if (fl.count == 0) return false;
        uint8_t candidate = fl.free_list[fl.head];
        fl.head = (uint8_t)((fl.head + 1) % INT_PHYS_REGS);
        fl.count--;
        if (candidate != 0 && candidate < INT_PHYS_REGS) {
            pdst = candidate;
            return true;
        }
    }
    return false;
}

static void clear_branch_alloc_list(BranchRecoveryState& br, uint8_t tag) {
    if (tag >= MAX_BRANCH_COUNT) return;
    for (int p=0; p<INT_PHYS_REGS; p++) br.br_alloc_lists[tag][p] = false;
}

static void record_pdst_for_active_branches(BranchRecoveryState& br, uint8_t active_mask, uint8_t pdst) {
    if (pdst == 0 || pdst >= INT_PHYS_REGS) return;
    for (int t=0; t<MAX_BRANCH_COUNT; t++) {
        if ((active_mask & tag_bit((uint8_t)t)) != 0 && br.tag_valid[t]) br.br_alloc_lists[t][pdst] = true;
    }
}

static void save_map_snapshot(RenameMapTableState& mt, BranchRecoveryState& br, uint8_t tag) {
    if (tag >= MAX_BRANCH_COUNT) return;
    for (int i=0; i<LOGICAL_REG_COUNT; i++) mt.br_snapshots[i][tag] = mt.map_table[i];
    mt.br_snapshots[0][tag] = 0;
    br.snapshot_valid[tag] = true;
}

void rename_module(BoomCoreState& state) {
    RenameState& ren = state.rename;
    RenameMapTableState& mt = ren.int_map_table;
    RenameFreeListState& fl = ren.int_free_list;
    BranchRecoveryState& br = state.branch_state;

    if (state.global_flush) return;
    mt.map_table[0] = 0;

    for (int i=0; i<DISPATCH_WIDTH; i++) {
        RenameDispatchPacket& packet = ren.dispatch_packets[i];
        if (packet.valid) continue;
        if (!state.decode.dec_valids[i]) continue;
        MicroOp uop = state.decode.dec_uops[i];
        bool is_branch = is_control_uop(uop);
        bool allocates_dst = (uop.rename.dst_rtype == DST_INT && uop.rename.ldst != 0);
        uint8_t active_before = br.active_mask;
        uint8_t new_tag = 0;
        bool got_tag = false;

        if (is_branch) {
            if (!allocate_branch_tag(br, new_tag)) {
                break;
            }
            got_tag = true;
            uop.branch.br_tag = new_tag;
        }
        uop.branch.br_mask = active_before;

        uop.rename.prs1 = mt.map_table[uop.rename.lrs1];
        uop.rename.prs2 = mt.map_table[uop.rename.lrs2];
        uop.rename.stale_pdst = mt.map_table[uop.rename.ldst];

        uop.rename.prs1_busy = (uop.rename.prs1 != 0) && fl.busy_table[uop.rename.prs1];
        uop.rename.prs2_busy = (uop.rename.prs2 != 0) && fl.busy_table[uop.rename.prs2];

        if (allocates_dst) {
            uint8_t pdst = 0;
            if (!allocate_preg(fl, pdst)) {
                break;
            }
            fl.busy_table[pdst] = true;
            uop.rename.pdst = pdst;
            mt.map_table[uop.rename.ldst] = pdst;
            record_pdst_for_active_branches(br, active_before, pdst);
        } else { uop.rename.pdst = 0; uop.rename.ldst = 0; }

        if (got_tag) {
            clear_branch_alloc_list(br, new_tag);
            save_map_snapshot(mt, br, new_tag);
            br.tag_valid[new_tag] = true;
            br.active_mask |= tag_bit(new_tag);
            br.allocations++;
        }

        packet.uop = uop;
        packet.rob_allocated = false;
        packet.valid = true;
        state.decode.dec_valids[i] = false;
    }
}

}
