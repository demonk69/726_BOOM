# Gate 3.5 Structural Optimization

Gate 3.5 executed single-variable branch recovery structure experiments against the accepted Gate 3.3/Gate 3.4 conservative baseline. The goal was to reduce full-core LUT without changing architecture, cycle behavior, or enabling `CORE_CYCLE` pipeline.

## Result

No Gate 3.5 optimization was accepted. Gate 3.3 conservative `boom_core_top` remains the accepted PPA configuration.

| Variant | LUT | Delta vs 83286 | Status |
|---|---:|---:|---|
| B1_PACKED_ALLOC_BITMAP | 83408 | +122 | REJECTED_PPA |
| B4_SET_ROLLBACK | 84984 | +1698 | REJECTED_PPA |
| C1_RECOVERY_ENABLE | 83903 | +617 | REJECTED_PPA |
| D1_SHARED_BRANCH_MASKS | 83301 | +15 | REJECTED_PPA |
| D4_LOCAL_KILL_BITMAP | 82789 | -497 | CANDIDATE_NOT_ACCEPTED_BELOW_10_PERCENT |
| D4_IQ_COMPACTION | 90802 | +7516 | REJECTED_PPA |

## Interpretation

- B1 changed `br_alloc_lists` storage shape to an 8x52 `RAM_AUTO_1R1W`, but it did not reduce full-core LUT.
- B4, C1, and D1 preserved behavior but added or failed to remove enough logic in Vitis HLS 2021.2.
- D4 was the best single-variable result and reduced full-core LUT by 497, but this is only 0.60% and below the 10% acceptance threshold.
- D4-IQ passed dedicated IQ compaction tests but substantially increased top-level LUT.
- No combo was run because failed PPA variants were not eligible for combination and D4 alone did not provide enough reduction.

## Readiness

| Flag | Value | Reason |
|---|---|---|
| READY_FOR_WIDE_ISSUE_IMPLEMENTATION | false | No accepted Gate 3.5 optimization and no >=10% LUT reduction |
| READY_FOR_CORE_PIPELINE_EXPERIMENT | false | No accepted Gate 3.5 configuration; recovery/kill logic reduction was not significant |
| READY_FOR_OFFICIAL_GATE_3 | false | Chipyard/FESVR/DRAMSim path remains blocked |

See `reports/gate3_5/gate3_5_results.md` for full evidence.
