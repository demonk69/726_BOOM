# Gate 3.3 Results

Date: 2026-07-26

## Verdict

Gate 3.3 is closed for the supported HLS subset: branch recovery is implemented, targeted branch snapshot regressions pass, existing architectural traces are preserved, and conservative no-pipeline csynth passes.

This is not official full BOOM Gate 3 closure. Strict cycle equivalence remains `INSUFFICIENT_EVIDENCE`, and the official Chipyard/FESVR/DRAMSim path remains blocked.

## Scope

| Item | Status |
|---|---|
| Frozen baseline commit | `6645b3df8fdf2828713513ad22cf4dcceb0a89f0` |
| Baseline worktree | Dirty; uncommitted Gate 3.2 synthesis-closure artifacts are part of the effective baseline |
| BOOM source basis | Generated SmallBoomConfig FIRRTL and Verilog only |
| Original Chisel checkout | Absent |
| PPA directive changes | None for the accepted baseline |
| `CORE_CYCLE` pipeline | Disabled by default; `BOOM_HLS_ENABLE_CORE_PIPELINE=1` not rerun for Gate 3.3 |

## Implementation Summary

| Mechanism | Gate 3.3 Status | Evidence |
|---|---|---|
| Branch tag parameters | MATCH | `reports/gate3_3/branch_parameter_inventory.csv` |
| Active branch mask and tag allocation | IMPLEMENTED | `include/boom_state.hpp`, `src/rename.cpp`, `reports/gate3_3/branch_state_inventory.csv` |
| Rename-map snapshot and restore | IMPLEMENTED | `src/rename.cpp`, `src/branch.cpp`, branch snapshot directed tests |
| Free-list allocation rollback | IMPLEMENTED | `BranchRecoveryState::br_alloc_lists`, duplicate-safe rollback tests |
| Busy recovery | FUNCTIONAL_SUBSTITUTE | Busy state rebuilt from still-valid busy ROB entries |
| Resolved-mask clear | IMPLEMENTED | `src/branch.cpp`, `src/issue.cpp`, `src/execute.cpp`, `src/lsu.cpp` |
| Younger-state kill | IMPLEMENTED | ROB, IQ, execute result, LDQ, STQ, frontend/decode/rename cleanup |

## Regression Results

| Check | Result | Evidence |
|---|---:|---|
| Directed tests | 25/25 PASS | `reports/gate3_3/logs/directed_after.log` |
| Gate 1 regressions | 13/13 PASS | `reports/gate3_3/logs/gate1_after.log` |
| Minimal LSU tests | 14/14 PASS | `reports/gate3_3/logs/lsu_after.log` |
| Branch snapshot directed tests | 30/30 PASS | `reports/gate3_3/logs/branch_snapshot_tests.log` |
| Branch snapshot random tests | 2/2 PASS | `reports/gate3_3/logs/branch_snapshot_random_tests.log` |
| HLS C++ complete trace preservation | 5/5 byte-identical | `reports/gate3_3/logs/hls_cpp_trace_compare_after.log` |
| Vitis HLS csim trace preservation | 5/5 byte-identical | `reports/gate3_3/logs/hls_csim_trace_compare_after.log` |
| BOOM-vs-HLS full-program architectural diff | 10/10 PASS | `reports/gate3_3/full_program_architectural_diff.md` |
| Partial-order analysis | 8 legal reorders, 0 real exposed violations | `reports/gate3_3/logs/partial_order_after.log` |
| Merged source compile | PASS | `src/boom_core_merged.cpp` compiled with `g++ -std=c++11 -Iinclude` |

Random branch snapshot seed: `0x3a33b007`.

## Synthesis Results

| Scope | Status | Runtime | Estimated Period | LUT | FF | BRAM_18K | DSP |
|---|---|---:|---:|---:|---:|---:|---:|
| Module diagnostic tops | 9/9 PASS | see CSV | see CSV | see CSV | see CSV | see CSV | see CSV |
| `boom_core_step_top` | PASS | 69.78s | 5.898 ns | 83353 | 16808 | 16 | 3 |
| Conservative `boom_core_top` | PASS | 71.69s | 5.898 ns | 83286 | 16611 | 16 | 3 |
| Performance `boom_core_top` with `BOOM_HLS_ENABLE_CORE_PIPELINE=1` | NOT_RUN_GATE3_3 | - | - | - | - | - | - |

| Resource Delta | Gate 3.2 | Gate 3.3 | Delta |
|---|---:|---:|---:|
| `boom_core_top` LUT | 40625 | 83286 | +42661 (+105.01%) |
| `boom_core_top` FF | 15985 | 16611 | +626 (+3.92%) |
| `boom_core_top` BRAM_18K | 16 | 16 | 0 |
| `boom_core_top` DSP | 3 | 3 | 0 |
| `boom_core_top` estimated period | 5.898 ns | 5.898 ns | 0.000 ns |

## Equivalence Status

| Dimension | Gate 3.3 Result |
|---|---|
| Architectural subset | PARTIALLY_VERIFIED |
| Branch recovery structure | PARTIALLY_VERIFIED for supported subset |
| Branch recovery cycle equivalence | INSUFFICIENT_EVIDENCE |
| Conservative synthesis baseline | PASS |
| Official Gate 3 | BLOCKED |

## Blockers And Limits

| Item | Status |
|---|---|
| Official Chipyard simulator | BLOCKED: `simulator-chipyard-SmallBoomConfig` absent |
| Original Chipyard/BOOM Chisel source | BLOCKED: checkout absent |
| FESVR/DRAMSim libraries | BLOCKED: `libfesvr.a` and `libdramsim.a` absent |
| Generated Makefile paths | BLOCKED: generated `VTestHarness.mk` references `/root/chipyard/...` |
| Full BOOM modules | NOT_IMPLEMENTED: full Cache/MMU/FPU/TLB/predictor/TileLink/L2 remain out of scope |
| Strict cycle equivalence | INSUFFICIENT_EVIDENCE |

## Primary Artifacts

| Artifact | Path |
|---|---|
| Source mapping | `docs/branch_recovery_source_mapping.md` |
| Branch snapshot status | `docs/branch_snapshot_status.md` |
| Parameter inventory | `reports/gate3_3/branch_parameter_inventory.csv` |
| State inventory | `reports/gate3_3/branch_state_inventory.csv` |
| Branch test results | `reports/gate3_3/branch_snapshot_test_results.md` |
| Regression summary | `reports/gate3_3/regression_after.md` |
| Module csynth summary | `reports/gate3_3/module_csynth_summary.csv` |
| Branch recovery csynth diagnostics | `reports/gate3_3/branch_recovery_csynth_diagnostics.csv` |
| Step-top csynth summary | `reports/gate3_3/step_top_csynth_summary.md` |
| Baseline csynth summary | `reports/gate3_3/baseline_csynth_summary.md` |
| Top synthesis modes | `reports/gate3_3/top_csynth_modes_gate3_3.csv` |
| Resource delta | `reports/gate3_3/resource_delta.csv` |
| Source hashes after | `reports/gate3_3/source_hashes_after.txt` |
