# Implementation Status

Gate 1 update: M003, M004, M006 are closed for the implemented integer/control subset.

Gate 2 update: Gate 2.5 corrected the standalone generated-model traces; they are finite loadmem-backed traces with retired-store `tohost` termination. Official Chipyard simulator/toolchain dependencies remain absent. `READY_FOR_PROVISIONAL_GATE_3=true`, `READY_FOR_GATE_3=false`.

Provisional Gate 3 update: BOOM-vs-HLS csim architectural prefixes pass for five loadmem-backed programs, comparing 45 commits before the store-to-`tohost` boundary. Event-order and normalized-cycle diffs fail because the HLS prefix trace is serialized and does not reproduce BOOM branch-resolution interleaving.

Gate 3.1A update: corrected event comparison uses per-uop partial order. The old global event-order failure is a validation-method false positive for the current traces: 8 legal reorder events, 0 real exposed partial-order violations, commit order matches. RAW/WAR/WAW timing remains insufficient-signal because issue, wakeup, rename-source, and complete events are missing.

Gate 3.1C update: minimal integer LSU/store-to-`tohost` support is implemented and validated for the directed subset. BOOM-vs-HLS full loaded-program architectural diff passes 10/10 across native HLS C++ and Vitis csim complete traces. Baseline Vitis csynth after LSU timed out during HLS transformations before producing a report.

## Implemented And Gate 1 Tested

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

## Partial Or Mismatch

- Branch snapshots, branch tags, branch masks, and per-branch free-list recovery are not implemented.
- M004 JALR redirect is verified only as a concrete Gate 1 functional test; it does not implement BOOM branch snapshots.
- Full BOOM IssueWidth=3 execution is not implemented; Gate 1 caps IQ grants to one implemented ALU lane.
- Busy-table wakeup is simplified and not equivalent to BOOM's full bypass/wakeup network.
- Exception and flush handling are coarse compared with BOOM.
- Cycle timing is not verified against BOOM.
- Baseline post-LSU csynth scalability is unresolved; Vitis HLS timed out before producing a new report.

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
- Official Chipyard/FESVR/DRAMSim trace: BLOCKED by missing original `/root/chipyard` generated-source path, `libfesvr`, `libdramsim`, and RISC-V ELF/binutils tools.
