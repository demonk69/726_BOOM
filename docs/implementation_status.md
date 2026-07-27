# Implementation Status

Gate 1 update: M003, M004, M006 are closed for the implemented integer/control subset.

Gate 2 update: Gate 2.5 corrected the standalone generated-model traces; they are finite loadmem-backed traces with retired-store `tohost` termination. Official Chipyard simulator/toolchain dependencies remain absent. `READY_FOR_PROVISIONAL_GATE_3=true`, `READY_FOR_GATE_3=false`.

Provisional Gate 3 update: BOOM-vs-HLS csim architectural prefixes pass for five loadmem-backed programs, comparing 45 commits before the store-to-`tohost` boundary. Event-order and normalized-cycle diffs fail because the HLS prefix trace is serialized and does not reproduce BOOM branch-resolution interleaving.

Gate 3.1A update: corrected event comparison uses per-uop partial order. The old global event-order failure is a validation-method false positive for the current traces: 8 legal reorder events, 0 real exposed partial-order violations, commit order matches. RAW/WAR/WAW timing remains insufficient-signal because issue, wakeup, rename-source, and complete events are missing.

Gate 3.1C update: minimal integer LSU/store-to-`tohost` support is implemented and validated for the directed subset. BOOM-vs-HLS full loaded-program architectural diff passes 10/10 across native HLS C++ and Vitis csim complete traces. Baseline Vitis csynth after LSU timed out during HLS transformations before producing a report.

Gate 3.2 update: conservative post-LSU synthesis is closed. `BoomCoreState next_state = state` was removed from `boom_core_step.cpp`, baseline `CORE_CYCLE` pipelining is disabled by default and macro-gated for experiments, frozen complete traces remain byte-identical, module diagnostic csynth passes, finite step-top csynth passes, and baseline `boom_core_top` csynth passes in 45.56s with a 5.898 ns estimated period. The separate pipeline-enabled performance experiment still times out after 15 minutes.

Gate 3.3 update: branch recovery is implemented for the supported single-lane integer/minimal-LSU subset. HLS now allocates branch tags, propagates branch masks, snapshots/restores the integer rename map, tracks per-branch physical destination allocation lists, rolls back the free list on mispredict, rebuilds busy state from valid busy ROB entries, clears resolved branch bits, and kills younger ROB/IQ/execute/LDQ/STQ state. Existing regressions remain passing, branch snapshot directed/random tests pass, merged source compiles, module diagnostic csynth passes, finite step-top csynth passes, and conservative `boom_core_top` csynth passes in 71.69s with a 5.898 ns estimated period.

Gate 3.4 update: resource attribution and module csynth baselines are complete. Gate 3.4 adds analysis scripts and attribution-only synthesis tops, not architectural behavior. `boom_core_top` still synthesizes with 83286 LUT, 16611 FF, 16 BRAM_18K, 3 DSP, and 5.898 ns estimated period. No LUT-reducing optimization candidate is accepted.

## Implemented And Tested

- Frontend request/response FSM with monotonic fetch IDs and stale response drop.
- RV64 integer ALU subset used by directed tests.
- JAL, JALR, and conditional branches with always-not-taken baseline redirect behavior.
- Integer rename map/free-list/stale-pdst commit release for single dispatch lane.
- ROB allocate, complete, commit, full backpressure, and wrap behavior for current tests.
- ALU issue queue dispatch/select/compact for one implemented execute lane.
- CSR cycle/instret and ECALL success/trap subset.
- Commit trace output for directed tests.
- Minimal integer LSU path for LB/LBU/LH/LHU/LW/LWU/LD and SB/SH/SW/SD in the current conservative single-lane path.
- Committed store-to-`tohost` termination via LSU `DmemRequest`.
- Gate 3.3 branch tag allocation, branch mask propagation, rename-map snapshot/restore, free-list allocation-list rollback, resolved-mask clear, and selective younger-state kill for the supported subset.

## Partial Or Mismatch

- Branch recovery is implemented for the supported HLS subset, but strict BOOM event/cycle timing is not verified and the original Chisel source checkout is unavailable for direct inspection.
- M004 JALR redirect is verified only as a concrete Gate 1 functional test; Gate 3.3 branch recovery is tracked separately under M009.
- Full BOOM IssueWidth=3 execution is not implemented; Gate 1 caps IQ grants to one implemented ALU lane.
- Busy-table wakeup is simplified and not equivalent to BOOM's full bypass/wakeup network; Gate 3.3 recovery rebuilds busy state functionally from still-valid busy ROB entries.
- Exception and flush handling are coarse compared with BOOM.
- Cycle timing is not verified against BOOM.
- Conservative no-pipeline csynth scalability remains resolved after Gate 3.3; `BOOM_HLS_ENABLE_CORE_PIPELINE=1` remains unresolved and was not rerun for Gate 3.3 after the prior timeout.

## Not Implemented

- Full BOOM LSU behavior, DCache, ICache, MMU/Sv39, TLB, PTW, cache miss/replay, AMO/LRSC, and full memory-ordering semantics.
- FPU and FP issue/register-read/writeback paths.
- Branch predictor, BTB, BIM, TAGE, RAS, FTQ, fetch buffer.
- TileLink, L2, interrupts, full CSR file, privilege transitions.

## Latest Verification

- Directed tests: 25 passed, 0 failed.
- Gate 1 regressions: 13 passed, 0 failed.
- Vitis HLS 2021.2 csim: 5 passed, 0 failed.
- Merged HLS compilation unit: C++ compile PASS.
- Gate 2.5 BOOM standalone traces: PASS_LOADMEM, five traces checked with 40/40/41/42/42 commits and no max-cycle truncation.
- Provisional Gate 3: ARCHITECTURAL_PREFIX_PASS, five BOOM-vs-HLS csim prefixes checked with 8/8/9/10/10 commits; event-order and normalized-cycle checks FAIL as expected for the serialized HLS subset.
- Gate 3.1A partial-order analysis: PARTIAL_MATCH, old event-order failure classified `VALIDATION_METHOD_FALSE_POSITIVE`, 8 legal reorders, 0 real exposed partial-order violations.
- Gate 3.1C minimal LSU: 14/14 LSU tests pass; complete HLS C++ and Vitis csim traces reach retired `SD` to `tohost`; full loaded-program architectural diff passes 10/10.
- Gate 3.1C csynth: TIMEOUT after 30 minutes during HLS transformations; no updated `csynth.rpt` produced.
- Gate 3.2 baseline csynth: PASS for `boom_core_top`, 45.56s runtime, 1521072 KB peak memory, 5.898 ns estimated period, 40625 LUT, 15985 FF, 16 BRAM_18K, 3 DSP.
- Gate 3.2 finite step-top csynth: PASS for `boom_core_step_top`, 44.09s runtime, 1520876 KB peak memory, 5.898 ns estimated period.
- Gate 3.2 module diagnostic csynth: PASS for all 9 tops in `reports/gate3_2/module_csynth_summary.csv`.
- Gate 3.2 performance pipeline csynth: TIMEOUT after 15 minutes with `BOOM_HLS_ENABLE_CORE_PIPELINE=1`; not part of the accepted baseline.
- Gate 3.3 branch snapshot tests: directed 30/30 PASS; random 2/2 PASS with seed `0x3a33b007`.
- Gate 3.3 regression preservation: directed 25/25 PASS; Gate 1 regressions 13/13 PASS; minimal LSU 14/14 PASS; HLS C++ complete traces 5/5 byte-identical; Vitis HLS csim complete traces 5/5 byte-identical; full-program architectural diff 10/10 PASS; partial-order analysis has 8 legal reorders and 0 real exposed violations.
- Gate 3.3 merged source: `src/boom_core_merged.cpp` regenerated and C++ compile PASS.
- Gate 3.3 module diagnostic csynth: PASS for all 9 tops in `reports/gate3_3/module_csynth_summary.csv`.
- Gate 3.3 finite step-top csynth: PASS for `boom_core_step_top`, 69.78s runtime, 1521136 KB peak memory, 5.898 ns estimated period, 83353 LUT, 16808 FF, 16 BRAM_18K, 3 DSP.
- Gate 3.3 baseline csynth: PASS for conservative `boom_core_top`, 71.69s runtime, 1520472 KB peak memory, 5.898 ns estimated period, 83286 LUT, 16611 FF, 16 BRAM_18K, 3 DSP.
- Performance pipeline csynth: NOT_RUN through Gate 3.4; prior Gate 3.2 `BOOM_HLS_ENABLE_CORE_PIPELINE=1` timeout remains deferred.
- Gate 3.4 module baseline: PASS for all 12 requested attribution/module tops in `reports/gate3_4/module_baseline.csv`.
- Gate 3.4 conservative `boom_core_top` csynth: PASS, 69.33s runtime, 1521180 KB peak memory, 5.898 ns estimated period, 83286 LUT, 16611 FF, 16 BRAM_18K, 3 DSP.
- Gate 3.4 regressions: directed 25/25 PASS; Gate 1 13/13 PASS; minimal LSU 14/14 PASS; branch snapshot directed 30/30 PASS; branch snapshot random 2/2 PASS; HLS C++ and Vitis csim complete traces are each 5/5 byte-identical to Gate 3.3; full-program architectural diff 10/10 PASS.
- Gate 3.4 optimization status: no accepted optimization; Gate 3.3 remains accepted PPA configuration.
- Gate 3.5 single-variable structural optimization status: B1, B4, C1, D1, D4, and D4-IQ completed full functional/trace/csynth gates. Only D4 reduced LUT, from 83286 to 82789, but this is below the 10% acceptance threshold. No Gate 3.5 optimization accepted.
- Gate 3.5 regressions: every variant preserved directed 25/25 PASS, Gate 1 13/13 PASS, minimal LSU 14/14 PASS, branch snapshot directed 30/30 PASS, expanded branch snapshot random 42/42 PASS, HLS C++/Vitis csim byte-identical traces, full-program architectural diff 10/10 PASS, and partial-order 8 legal reorders with 0 real violations.
- Official Chipyard/FESVR/DRAMSim trace: BLOCKED by missing original `/root/chipyard` generated-source path, `libfesvr`, `libdramsim`, and RISC-V ELF/binutils tools.
