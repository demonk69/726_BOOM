# Gate 3.5 Results

Date: 2026-07-26

## Verdict

Status: `SINGLE_VARIABLE_EXPERIMENTS_COMPLETE_NO_ACCEPTED_OPTIMIZATION`.

Gate 3.5 executed the requested single-variable structural experiments from the accepted Gate 3.3/Gate 3.4 baseline. No experiment met the minimum 10% full-core LUT reduction threshold. Gate 3.3 conservative `boom_core_top` remains the accepted PPA configuration.

## Baseline

| Metric | Accepted Baseline |
|---|---:|
| LUT | 83286 |
| FF | 16611 |
| BRAM_18K | 16 |
| DSP | 3 |
| Estimated period | 5.898 ns |
| CORE_CYCLE pipeline | disabled |

## Variant Results

| Variant | Functional | Trace | Arch Diff | LUT | FF | BRAM | DSP | Period | Verdict |
|---|---|---|---|---:|---:|---:|---:|---:|---|
| B1_PACKED_ALLOC_BITMAP | PASS | 10/10 byte-identical | 10/10 PASS | 83408 | 16754 | 16 | 3 | 5.898 | REJECTED_PPA |
| B4_SET_ROLLBACK | PASS | 10/10 byte-identical | 10/10 PASS | 84984 | 16943 | 16 | 3 | 5.898 | REJECTED_PPA |
| C1_RECOVERY_ENABLE | PASS | 10/10 byte-identical | 10/10 PASS | 83903 | 16705 | 16 | 3 | 5.898 | REJECTED_PPA |
| D1_SHARED_BRANCH_MASKS | PASS | 10/10 byte-identical | 10/10 PASS | 83301 | 16619 | 16 | 3 | 5.898 | REJECTED_PPA |
| D4_LOCAL_KILL_BITMAP | PASS | 10/10 byte-identical | 10/10 PASS | 82789 | 17041 | 16 | 3 | 5.898 | CANDIDATE_NOT_ACCEPTED_BELOW_10_PERCENT |
| D4_IQ_COMPACTION | PASS | 10/10 byte-identical | 10/10 PASS | 90802 | 18590 | 16 | 3 | 5.898 | REJECTED_PPA |

## Required Answers

| Question | Answer |
|---|---|
| 1. B1 resource result | `boom_core_top` PASS, 83408 LUT, 16754 FF, 16 BRAM_18K, 3 DSP, 5.898 ns. Rejected because LUT increased by 122. The packed bitmap remained `RAM_AUTO_1R1W` as 8x52 storage. |
| 2. B4 resource result | `boom_core_top` PASS, 84984 LUT, 16943 FF, 16 BRAM_18K, 3 DSP, 5.898 ns. Rejected because LUT increased by 1698. |
| 3. C1 resource result | `boom_core_top` PASS, 83903 LUT, 16705 FF, 16 BRAM_18K, 3 DSP, 5.898 ns. Rejected because LUT increased by 617. |
| 4. D1 resource result | `boom_core_top` PASS, 83301 LUT, 16619 FF, 16 BRAM_18K, 3 DSP, 5.898 ns. Rejected because LUT increased by 15. |
| 5. D4 resource result | `boom_core_top` PASS, 82789 LUT, 17041 FF, 16 BRAM_18K, 3 DSP, 5.898 ns. Area decreased by 497 LUT, but only 0.60%, below the 10% acceptance threshold. |
| 6. D4-IQ resource result | `boom_core_top` PASS, 90802 LUT, 18590 FF, 16 BRAM_18K, 3 DSP, 5.898 ns. Rejected because LUT increased by 7516. |
| 7. Functional status | All six variants passed directed 25/25, Gate 1 13/13, minimal LSU 14/14, branch snapshot directed 30/30, branch snapshot random 42/42, and applicable IQ directed tests. |
| 8. Cycle trace status | All six variants produced HLS C++ and Vitis csim complete traces byte-identical to the frozen accepted baseline for the 5 loaded programs. |
| 9. Full-core resources | Listed in the Variant Results table and `reports/gate3_5/variant_summary.csv`. |
| 10. Estimated period | All six full-core conservative csynth runs reported 5.898 ns. |
| 11. Best single-variable scheme | D4_LOCAL_KILL_BITMAP by LUT delta: -497 LUT. It is not accepted because it misses the 10% threshold and increases FF by 430. |
| 12. Best combo scheme | No combo was run. Only D4 reduced LUT, and failed/area-increasing variants were not eligible for combination. |
| 13. 10% LUT reduction | No. Required LUT <= 74957; best observed was 82789. |
| 14. 25% LUT reduction | No. Required LUT <= 62464; best observed was 82789. |
| 15. Final accepted configuration | Gate 3.3 conservative `boom_core_top` remains accepted: 83286 LUT, 16611 FF, 16 BRAM_18K, 3 DSP, 5.898 ns. |
| 16. M009 status | `PARTIALLY_VERIFIED`; not upgraded to full verified. |
| 17. READY_FOR_WIDE_ISSUE_IMPLEMENTATION | false. No accepted Gate 3.5 optimization and no >=10% LUT reduction. |
| 18. READY_FOR_CORE_PIPELINE_EXPERIMENT | false. No accepted Gate 3.5 configuration and recovery/kill logic reduction was not significant enough. |
| 19. READY_FOR_OFFICIAL_GATE_3 | false. Chipyard/FESVR/DRAMSim environment remains unavailable. |

## Combo Decision

Combination experiments were intentionally not run. The only LUT-reducing variant was D4_LOCAL_KILL_BITMAP, and it did not reach the acceptance threshold. B1, B4, C1, D1, and D4-IQ were rejected by PPA, so combining them would violate the Gate 3.5 rule against combining failed experiments.

## Primary Artifacts

| Artifact | Path |
|---|---|
| Baseline manifest | `reports/gate3_5/baseline_manifest.md` |
| Variant summary | `reports/gate3_5/variant_summary.csv` |
| Regression summary | `reports/gate3_5/regression_after.md` |
| B1 reports | `reports/gate3_5/variants/B1_PACKED_ALLOC_BITMAP/` |
| B4 reports | `reports/gate3_5/variants/B4_SET_ROLLBACK/` |
| C1 reports | `reports/gate3_5/variants/C1_RECOVERY_ENABLE/` |
| D1 reports | `reports/gate3_5/variants/D1_SHARED_BRANCH_MASKS/` |
| D4 reports | `reports/gate3_5/variants/D4_LOCAL_KILL_BITMAP/` |
| D4-IQ reports | `reports/gate3_5/variants/D4_IQ_COMPACTION/` |

## Constraints Preserved

- `CORE_CYCLE` pipeline remains disabled.
- No queue depth, branch count, state capacity, field width, ISA scope, execution-unit count, or memory-system scope was reduced.
- Snapshot storage directives were not prioritized or applied.
- Rejected structural source changes were restored; only Gate 3.5 scripts, reports, and test harness additions remain.
- Official Gate 3 remains blocked by missing Chipyard/FESVR/DRAMSim artifacts.
