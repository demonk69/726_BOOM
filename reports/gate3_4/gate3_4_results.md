# Gate 3.4 Results

Date: 2026-07-26

## Verdict

Status: `ANALYSIS_AND_MODULE_BASELINE_COMPLETE_NO_ACCEPTED_OPTIMIZATION`.

Gate 3.4 completed baseline freeze, resource attribution, RTL/storage inventory, automatic-partition inventory, and a repeatable module csynth framework. No optimization candidate was applied or accepted, so Gate 3.3 remains the accepted PPA configuration.

## Resource Attribution Answers

| Question | Answer |
|---|---|
| 1. Main source of +42661 LUT | Vitis reports the entire full-core delta under the synthesized `boom_core_cycle_io` instance: 40156 instance LUT in Gate 3.2 to 82817 in Gate 3.3. Direct new branch recovery diagnostics identify the dominant cones: `branch_module` 15506 LUT, `recover_mispredict` 11724 LUT, `kill_issue_state` 5799 LUT, `compact_issue_queue` 5473 LUT, `kill_lsu_state` 2269 LUT, `kill_rob_younger_than` 1501 LUT. These reports overlap and are not additive. |
| 2. Snapshot actual structure | `rename.int_map_table.br_snapshots` is generated as `RAM_AUTO_1R1W`, `DataWidth=8`, `AddressRange=256`, reg-array RAM. It is not complete-partitioned and did not add BRAM in the accepted top. |
| 3. `br_alloc_lists` actual structure | `branch_state.br_alloc_lists` is generated as `RAM_AUTO_1R1W`, `DataWidth=1`, `AddressRange=416`, reg-array RAM. It is not complete-partitioned and did not add BRAM. The cost is in rollback/update logic, not BRAM. |
| 4. Busy recovery cost | Busy recovery is inlined into `recover_mispredict`: 52-bit busy-table clear plus 32-entry ROB scan. It is included in the 11724 LUT `recover_mispredict` report, not isolated as a standalone direct report. |
| 5. Branch kill network cost | `kill_issue_state` reports 5799 LUT, `compact_issue_queue` 5473 LUT, `kill_lsu_state` 2269 LUT, `kill_rob_younger_than` 1501 LUT, and `clear_resolved_masks_in_state` 366 LUT. Repeated entry-wise branch-mask compare plus queue compaction is the main visible cost. |
| 6. Best single-variable experiment | Not selected. Gate 3.4 stopped after attribution and module baseline as requested; A1-E5 remain `NOT_RUN_RESOURCE_ATTRIBUTION_PHASE`. |
| 7. Rejected experiments | None functionally rejected because no optimization experiment was applied. All non-baseline experiments are deferred pending single-variable implementation and full validation. |
| 8. Best candidate resources | No optimized candidate accepted. Accepted baseline remains Gate 3.3: 83286 LUT, 16611 FF, 16 BRAM_18K, 3 DSP. |
| 9. Best candidate period | No optimized candidate accepted. Accepted baseline remains 5.898 ns. |
| 10. Resource change vs Gate 3.3 | Gate 3.4 attribution baseline `boom_core_top` matches Gate 3.3: 83286 LUT, 16611 FF, 16 BRAM_18K, 3 DSP, 5.898 ns. |
| 11. Regression results | Directed 25/25, Gate 1 13/13, minimal LSU 14/14, branch snapshot directed 30/30, branch snapshot random 2/2. |
| 12. Trace preservation | HLS C++ 5/5 byte-identical and Vitis csim 5/5 byte-identical to the frozen Gate 3.3 baseline traces. |
| 13. Architectural diff | 10/10 PASS using isolated Gate 3.4 traces. |
| 14. Partial-order | 8 legal reorders, 0 real exposed violations, inherited by byte-identical traces. |
| 15. Accepted configuration | Gate 3.3 conservative no-pipeline baseline remains accepted. Gate 3.4 adds analysis scripts/tops only. |
| 16. 10% LUT reduction | No. No optimization candidate accepted. |
| 17. 25% LUT target | No. No optimization candidate accepted. |
| 18. M009 status | `PARTIALLY_VERIFIED`; not upgraded to full VERIFIED. |
| 19. READY_FOR_WIDE_ISSUE_IMPLEMENTATION | false. Gate 3.4 has no accepted optimized baseline and LUT has not been reduced. |
| 20. READY_FOR_CORE_PIPELINE_EXPERIMENT | false. Conservative synthesis is stable and attribution is clearer, but structural optimization experiments are not complete. |
| READY_FOR_OFFICIAL_GATE_3 | false. Chipyard/FESVR/DRAMSim path remains blocked. |

## Module Baseline

| Module | Status | Period | LUT | FF | BRAM_18K | DSP |
|---|---|---:|---:|---:|---:|---:|
| `synth_branch_tag_top` | PASS | 1.950 ns | 341 | 87 | 0 | 0 |
| `synth_branch_mask_top` | PASS | 2.203 ns | 11494 | 6246 | 0 | 0 |
| `synth_map_snapshot_top` | PASS | 2.629 ns | 11868 | 6444 | 1 | 0 |
| `synth_free_list_rollback_top` | PASS | 2.203 ns | 11895 | 6295 | 1 | 0 |
| `synth_busy_recovery_top` | PASS | 2.203 ns | 12291 | 6409 | 1 | 0 |
| `synth_branch_kill_top` | PASS | 2.203 ns | 12091 | 6321 | 1 | 0 |
| `synth_rename_top` | PASS | 1.950 ns | 268 | 229 | 0 | 0 |
| `synth_rob_top` | PASS | 1.829 ns | 71 | 13 | 0 | 0 |
| `synth_issue_top` | PASS | 2.203 ns | 11147 | 4147 | 0 | 0 |
| `synth_lsu_top` | PASS | 3.474 ns | 712 | 415 | 2 | 0 |
| `synth_core_step_top` | PASS | 5.898 ns | 45350 | 12111 | 12 | 3 |
| `boom_core_top` | PASS | 5.898 ns | 83286 | 16611 | 16 | 3 |

## Primary Artifacts

| Artifact | Path |
|---|---|
| Baseline manifest | `reports/gate3_4/baseline_manifest.md` |
| Baseline resources | `reports/gate3_4/baseline_resources.csv` |
| Resource attribution | `reports/gate3_4/resource_attribution.md` and `reports/gate3_4/resource_attribution.csv` |
| RTL structure inventory | `reports/gate3_4/rtl_structure_inventory.csv` |
| Automatic partition inventory | `reports/gate3_4/automatic_partition_inventory.csv` |
| Module baseline | `reports/gate3_4/module_baseline.csv` |
| Variant summary | `reports/gate3_4/variant_summary.csv` |
| Regression summary | `reports/gate3_4/regression_after.md` |
| Source hashes before/after | `reports/gate3_4/source_hashes_before.txt`, `reports/gate3_4/source_hashes_after.txt` |

## Constraints Preserved

- `CORE_CYCLE` pipeline remains disabled.
- No state depth or field width was reduced.
- No cache, MMU, FPU, predictor, TileLink, or L2 work was added.
- No test expectations were changed.
- No optimization candidate was accepted without full validation.
