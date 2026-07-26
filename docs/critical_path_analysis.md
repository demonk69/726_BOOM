# Critical Path Analysis

Gate 3.4 preserves conservative baseline synthesis while adding resource attribution and module-baseline handles. This is an analysis result, not an accepted LUT optimization and not a strict BOOM cycle-equivalence claim.

## Current Status

- Target clock: 10.00 ns (100 MHz)
- Latest accepted baseline estimated period: 5.898 ns
- Latest accepted baseline status: PASS for `boom_core_top`
- Latest accepted baseline resources: 83286 LUT, 16611 FF, 16 BRAM_18K, 3 DSP
- Gate 3.4 attribution baseline runtime: 69.33 seconds, 1521180 KB peak memory
- Finite step-top status: PASS for `boom_core_step_top`, 69.78 seconds, 1521136 KB peak memory, 83353 LUT, 16808 FF, 16 BRAM_18K, 3 DSP
- Performance pipeline experiment: NOT_RUN_GATE3_4; prior Gate 3.2 timeout with `BOOM_HLS_ENABLE_CORE_PIPELINE=1` remains deferred

## Known Critical Areas

| Area | Cause | Gate 3.3 Status |
|---|---|---|
| `rename_module` | Sequential map/free-list loops plus branch tag allocation and snapshot capture | Diagnostic csynth PASS; no PPA directive applied |
| `rob_commit_module` | Loop-based ROB commit/store trace processing | Split into `commit.cpp`; diagnostic csynth PASS |
| `issue_module` | Loop-based IQ select/compact plus branch kill/clear handling | Diagnostic csynth PASS; still one implemented execute lane; estimated period 2.203 ns |
| `execute_module` | ALU/branch/address path plus single implemented result lane | Diagnostic csynth PASS; highest diagnostic period among leaf modules at 5.819 ns |
| `lsu_module` | Minimal LDQ/STQ scans and load-response/store-commit handling | Diagnostic csynth PASS; full BOOM cache/MMU/replay behavior absent |
| `branch_module` | Branch update selection, branch-tag release, mispredict dispatch | Branch recovery diagnostic csynth PASS at 2.763 ns |
| `recover_mispredict` | Map restore, free-list rollback, branch tag pruning, branch-mask clear, busy rebuild, younger-state kill | Branch recovery diagnostic csynth PASS at 2.763 ns |
| `boom_core_step` | Serialized module calls over persistent state | Whole-state copy removed; Gate 3.3 finite step-top csynth PASS |

## Deferred Safe Optimizations

Do not apply these until strict trace evidence exists for the target behavior:

- `ARRAY_PARTITION` or `ARRAY_RESHAPE` for map tables, busy table, PRF, ROB, and IQ arrays.
- `UNROLL` for real lane-parallel fixed loops.
- Tree priority encoder for IQ selection.
- `PIPELINE II=1` on `CORE_CYCLE` only after state dependencies are proven safe; the Gate 3.3 accepted baseline keeps this disabled and the prior Gate 3.2 performance experiment timed out.

Cycle equivalence remains INSUFFICIENT_EVIDENCE. Gate 3.4 claims resource attribution and module-baseline completion only; Gate 3.3 remains the accepted PPA configuration.
