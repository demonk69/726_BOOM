# Critical Path Analysis

## Gate 3.10 Current Result

Gate 3.9 verbose schedules identify `lsu_module` load extraction as the longest state-local path at 5.90 ns, followed by execute multiply at 5.87 ns. The former includes ROB address RAM read, variable shift/mask/sign extension, and ROB data RAM write. This is current HLS schedule evidence, not post-route STA.

The named normal-path loops are not legal pipeline targets: free-list and issue/LSU scans carry state recurrence, ROB commit carries ordered external handshakes, and branch-mask recovery is same-cycle. R1 reset-only reaches II=1 but is rejected by RTL latency/cycle evidence. See `reports/gate3_10/critical_path_inventory.csv`.

Gate 3.7 preserves the conservative accepted baseline while characterizing the outer `CORE_CYCLE` pipeline transformation. This is not an accepted pipeline configuration and not a strict BOOM cycle-equivalence claim.

## Current Status

- Target clock: 10.00 ns (100 MHz)
- Gate 3 accepted baseline estimated period: 5.898 ns
- Gate 3 accepted baseline status: PASS for `boom_core_top`
- Gate 3 accepted baseline resources: 83286 LUT, 16611 FF, 16 BRAM_18K, 3 DSP
- Gate 3.4 attribution baseline runtime: 69.33 seconds, 1521180 KB peak memory
- Gate 3.5 best unaccepted variant: D4_LOCAL_KILL_BITMAP, 82789 LUT, 17041 FF, 16 BRAM_18K, 3 DSP, 5.898 ns; rejected because reduction is only 0.60%
- Finite step-top status: PASS for `boom_core_step_top`, 69.78 seconds, 1521136 KB peak memory, 83353 LUT, 16808 FF, 16 BRAM_18K, 3 DSP
- Gate 3.6 direct diagnostic: `synth_core_step_top`, 45350 LUT, 12111 FF, 12 BRAM_18K, 3 DSP, 5.898 ns, 342 automatic partitions
- Gate 3.6 T3 inline: 87388 LUT, 22117 FF, zero automatic partitions; rejected by PPA
- Gate 3.6 T4 no-reset attribution: 45602 LUT, 12119 FF, 12 BRAM_18K, 342 automatic partitions; rejected because required hardware reset is lost
- Gate 3.7 P0: exact accepted result reproduced in 71.40 seconds, 1520716 KB peak memory
- Gate 3.7 P1 no-II pipeline: TIMEOUT at 900 seconds in Presyn 2; 65 implied unrolls, no schedule/II/resource report

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
| resettable `BoomCoreState` elaboration | Whole-state HLS reset suppresses 342 automatic partitions and expands helper-port/state mux cones | 37684 LUT isolated by T4; reset is required and retained |
| `boom_core_cycle_io` boundary | Retained in accepted/N-cycle tops | T3 force-inline increases LUT and leaves partition count at zero; boundary alone is not causal |
| `CORE_CYCLE` pipeline transformation | Outer pipeline propagates complete-unroll requirements into nested ROB/IQ/LSU/branch loops | P1 no-II times out before scheduling; minimum II cannot be determined |
| loop-carried state recurrence | Execute->branch/LSU, PC/request, maps/free/busy, ROB/IQ/LSU, CSR/RF and FIFO feedback | Real dependencies; no false-dependence directive allowed |

## Deferred Safe Optimizations

Do not apply these until strict trace evidence exists for the target behavior:

- `ARRAY_PARTITION` or `ARRAY_RESHAPE` for map tables, busy table, PRF, ROB, and IQ arrays.
- `UNROLL` for real lane-parallel fixed loops.
- Tree priority encoder for IQ selection.
- Additional full `CORE_CYCLE` pipeline II experiments remain closed after P1 no-II timed out before scheduling. Do not bypass the report gate with II=1 or false-dependence directives.

Cycle equivalence remains INSUFFICIENT_EVIDENCE. Gate 3.7 status is `CORE_CYCLE_PIPELINE_TRANSFORMATION_TIMEOUT_NO_SYNTHESIS_CANDIDATE`; Gate 3.3 remains the accepted PPA configuration.

## Gate 4.0 W4E Final

The final W4E product estimate is 6.025 ns for both step and product tops. Verbose scheduling localizes the longest state to `execute_module` state 8 at 6.02 ns: selected source readiness, parallel 52x64 replicated-bank reads, LVT selection, operand/opcode selection, and execute result/control generation. Completion service is secondary. Canonical verbose evidence is under `reports/gate4_0/w4/csynth_final/{synth_core_step_top,boom_core_top}/verbose/`.

All tops are reported unpipelined and `CORE_CYCLE` has no `PipelineII`. The source has no DATAFLOW, false-dependence, or explicit complete-partition pragma. Waiver-free `apply_writeback_ports` reaches target/final II=1 and maps independent bank write enables. The LVT bank-select read cone moves `execute_module` state 8 to 6.02 ns; final product estimate is 6.025 ns.
