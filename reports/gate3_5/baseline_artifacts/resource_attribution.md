# Gate 3.4 Resource Attribution

## Summary

Gate 3.3 increases conservative `boom_core_top` LUTs by 42661 over Gate 3.2. The Vitis top report accounts for this entirely in the synthesized instance hierarchy: the top `Instance` LUT line moves from 40156 to 82817 while FIFO, expression, mux, BRAM, and DSP totals are effectively unchanged.

The direct branch recovery diagnostics are not additive, but they identify the new dominant cones: `branch_module` reports 15506 LUT, `recover_mispredict` reports 11724 LUT, `kill_issue_state` reports 5799 LUT, `compact_issue_queue` reports 5473 LUT, `kill_lsu_state` reports 2269 LUT, `kill_rob_younger_than` reports 1501 LUT, and `clear_resolved_masks_in_state` reports 366 LUT.

## Storage Findings

- Snapshot storage is `256x8` `1R1W` `reg_array_ram` at `boom_hls_gate3_3_baseline/solution_baseline/syn/verilog/boom_core_top_boom_core_cycle_io_state_rename_int_map_table_br_snapshots_RAM_AUTO_1R1W.v`. It is not complete-partitioned and did not add BRAM in the accepted top.
- `br_alloc_lists` storage is `416x1` `1R1W` `reg_array_ram` at `boom_hls_gate3_3_baseline/solution_baseline/syn/verilog/boom_core_top_boom_core_cycle_io_state_branch_state_br_alloc_lists_RAM_AUTO_1R1W.v`. It is not complete-partitioned and did not add BRAM in the accepted top.

## Attribution Table

| Component | Evidence | Estimated LUT | Confidence | Candidate | Risk |
|---|---|---:|---|---|---|
| full_core_delta | REPORT_DIRECT | 42661 | HIGH | target recovery helper boundaries and branch kill/rollback structures | medium: changes touch recovery cycle semantics |
| branch_recovery_dispatch | REPORT_DIRECT | 15506 | HIGH | structure audit | medium |
| mispredict_recovery_path | REPORT_DIRECT | 11724 | HIGH | function-boundary audit; local kill bitmap; packed allocation bitmap | high |
| issue_branch_kill | REPORT_DIRECT | 5799 | HIGH | function-boundary audit; local kill bitmap; packed allocation bitmap | medium |
| lsu_branch_kill | REPORT_DIRECT | 2269 | HIGH | structure audit | medium |
| rob_branch_kill | REPORT_DIRECT | 1501 | HIGH | structure audit | medium |
| resolved_mask_clear | REPORT_DIRECT | 366 | HIGH | structure audit | low |
| issue_queue_compaction | REPORT_DIRECT | 5473 | HIGH | function-boundary audit; local kill bitmap; packed allocation bitmap | medium |
| rename_branch_tag_and_snapshot_write | REPORT_DIRECT+RTL_INSTANCE | 191 | HIGH | snapshot storage and packed allocation-list experiments, but rename delta is not the dominant top-level source | medium |
| issue_branch_mask_kill_refresh | REPORT_DIRECT+RTL_INSTANCE | 3969 | HIGH | D1/D4 local kill bitmap and shared mask computation | medium: wrong-path uop must not survive with side effects |
| snapshot_storage | RTL_INSTANCE+REPORT_DIRECT | not_isolated_storage_logic_in_rename_and_recover | HIGH | A2/A4 only after access-port proof; packed row may trade mux shape for storage width | medium: restore cycle semantics and x0 mapping must remain exact |
| allocation_list_rollback | RTL_INSTANCE+LOG_EVIDENCE+ENGINEERING_INFERENCE | included_in_recover_mispredict_11724_and_branch_module_15506 | HIGH | B1 packed ap_uint<52> bitmap and B4 duplicate-safe set operation candidate | high: duplicate-safe recycle and same-cycle rollback semantics must be preserved |
| busy_recovery | LOG_EVIDENCE+ENGINEERING_INFERENCE | included_in_recover_mispredict_11724 | MEDIUM | C1 recovery_enable boundary and C2 single recovery bitmap candidate | medium/high: wakeup/issue timing evidence is insufficient |

## Optimization Direction

The first safe experiments should isolate structure before directives: packed allocation bitmaps, local branch-kill bitmaps, and explicit recovery-enable boundaries. Snapshot storage directives are lower priority because the accepted RTL already maps snapshots as RAM_AUTO_1R1W and the rename-module LUT delta is only 191.
