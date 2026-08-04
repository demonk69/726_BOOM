# Gate 4.0 W4A Completion Interfaces

Result: **PASS**. Scope is W4A only; W4B, W4C, and W4D are not enabled.

## Implementation

- Added the fixed three-source completion contract: LSU load response, MEM execute slot, and INT execute slot.
- Added `CompletionKind`, POD `CompletionEvent`, `CompletionSourceId`, execute/load-response conversion helpers, allocation ownership and validity checks, and wrap-safe ROB-age arbitration with deterministic source-priority ties. Branch events use the captured `uop.branch` plus compact mispredict and redirect metadata; application does not reread an execute slot.
- The serial datapath uses fixed MEM and INT event variables, explicit validity/age comparison, and explicit source calls. It has no event constructor, full-event array, dynamic full-event indexing, duplicate `BranchInfo`, or `AluResult` reconstruction.
- Routed execute, store, branch, exception, and LSU load-response completion side effects through one canonical function. No independent CSR or divider completion producer exists in the current implementation, so none was invented.
- Preserved W3 serial service: at most one PRF write, busy-table wakeup, and ROB completion per cycle. A queued DMEM response retains W3 ownership of the cycle; MEM and INT held results retain oldest-ROB-first arbitration.
- Enforced exactly-once acceptance while an allocation remains live: completed/non-busy, duplicate-memory, stale-allocation, killed-owner, and already-completed load events are rejected and discarded without side effects.
- LSU response cleanup requires a selected, valid, busy ROB owner, matching allocation and transaction IDs, and matching LDQ ownership. Reclamation occurs only after `apply_completion` accepts the event in the same serial service call.
- Preserved W3 exceptional-execute semantics intentionally: the ROB records the exception while an integer destination still receives the execute result and wakeup. Exceptional load responses complete the ROB and record the exception but explicitly suppress PRF write and wakeup.
- Preserved branch recovery before exception marking for a branch carrying both conditions, reset clearing, and held completion behavior under LSU backpressure.

## Regression Results

- W3 preservation software checks: **400/400 PASS**, 0 failed across 197 runs.
- W4A phase-directed completion checks: **19/19 PASS**. Independent cases cover event self-containment, duplicate events, stale load allocation, stale load transaction, killed owner, exceptional execute, exceptional load suppression, reset collision, branch/exception ordering, and simultaneous load-response/execute ownership.
- W2 random differential: **64/64 seeds PASS**, 32 cycles/seed, 2,048 cycles, 0 dropped grants.
- W3 persistent random differential: **100/100 seeds PASS**, 64 cycles/seed, 6,400 cycles, 0 dropped or duplicate tokens.
- Reset suites: default **14/14 PASS** and pipelined-reset variant **14/14 PASS**.
- C++ versus Vitis HLS csim traces: **7/7 normalized pairs PASS**.
- Normalized architecture/event/cycle comparisons: **21/21 PASS**.
- Full-program architectural comparison: **10/10 PASS**.
- Partial-order comparison: **7/7 PASS**.
- Merged generation, merged compilation, synthesis-top compilation, and core-top compilation: **4/4 PASS**.

Evidence is under `reports/gate4_0/w4/regression/`; no W1-W3 report was overwritten.

## Vitis HLS 2021.2

Part: `xczu7ev-ffvc1156-2-e`. Target period: 10.00 ns.

| Top | Estimated ns | Margin ns | LUT | FF | BRAM_18K | DSP | Pipeline |
|---|---:|---:|---:|---:|---:|---:|---|
| `synth_issue_top` | 4.570 | 5.430 | 15,105 | 4,699 | 0 | 0 | no |
| `synth_execute_top` | 5.081 | 4.919 | 1,486 | 861 | 4 | 3 | no |
| `synth_completion_top` | 2.702 | 7.298 | 1,866 | 674 | 4 | 0 | no |
| `synth_rob_top` | 1.829 | 8.171 | 144 | 46 | 0 | 0 | no |
| `synth_lsu_top` | 3.474 | 6.526 | 3,250 | 1,622 | 3 | 0 | no |
| `synth_core_step_top` | 5.875 | 4.125 | 62,408 | 16,584 | 14 | 3 | no |
| `boom_core_top` | 5.875 | 4.125 | 71,151 | 16,678 | 14 | 3 | no |

Canonical values and XML paths are in `w4a_resource_summary.csv`; per-top reports are under `csynth/`.

The HLS-safe event redesign removed the blocked artifact. Relative to the pre-fix review run:

- `synth_completion_top`: LUT 89,901 to 1,866; FF 11,550 to 674; BRAM 39 to 4; DSP 664 to 0; estimated period 7.300 ns to 2.702 ns.
- `boom_core_top`: LUT 200,964 to 71,151; FF 29,383 to 16,678; BRAM 61 to 14; DSP 697 to 3; estimated period 7.300 ns to 5.875 ns.
- The seven estimated periods are no longer an identical 7.300 ns schedule artifact.

Against the frozen W3 full-core baseline, final W4A is +3,096 LUT, +529 FF, -1 BRAM, unchanged at 3 DSP, and 0.023 ns faster in estimated period.

## Bind Audit

All generated `*.verbose.bind.rpt` files were inspected. `w4a_bind_audit.csv` records:

- Reciprocal operations: 0 for all seven tops.
- Completion multiply operations: 0 for all seven tops.
- Completion address-multiply operations: 0 for all seven tops.
- `i129` operations: 0 for six tops; 3 in `boom_core_top`, all in `boom_core_cycle_io` stream pack/unpack operations from `hls_stream_39.h`, with no multiplier binding.

Completion bind reports are preserved under each applicable `csynth/<top>/verbose/` directory.

## HLS Warnings

Vitis HLS 2021.2 emits HLS 200-805 conservative deadlock warnings for local `PipeSignals` streams with default depth:

- `synth_lsu_top`: 2 (`pipe.2`, `pipe.3`).
- `synth_core_step_top`: 5 (`pipe.0` through `pipe.4`).
- `boom_core_top`: 5 (`pipe.0` through `pipe.4`).
- The other four targets: 0 HLS 200-805 warnings.

These are not reported as warning-free or proven harmless. They concern existing internal request/response/trace streams, not a new completion-event stream; W4A adds no stream or multi-service state. Counts are recorded in `w4a_hls_warnings.csv`.

## Guardrails

- `CORE_CYCLE` is present and unpipelined in the `boom_core_top` XML; no `PipelineII` is reported.
- `DISPATCH_WIDTH=1` and `COMMIT_WIDTH=1` are unchanged.
- Fetch/decode/issue widths, ROB depth/index width, physical-register capacities/width, IQ depths/index width, LDQ/STQ depths/index widths, branch mask/tag capacity, FTQ depth/index width, and address widths are unchanged from commit `d0de7f5`.
- No multi-service completion state or behavior is enabled.
- No W1-W3 report was written by the W4A scripts.
- Final csynth report hashes are in `w4a_csynth_hashes.sha256`; regression artifact and source hashes were refreshed by `run_w4a_regressions.sh`.

## Equivalence

W3 behavioral preservation is **PASS**: C++ and Vitis HLS csim agree on all seven normalized traces, all 21 architecture/event/cycle checks pass, all seven partial-order checks pass, and the loaded-program architectural comparison is 10/10.

## Blockers

No W4A gate blocker remains. Official external Chipyard/FESVR equivalence is outside this W4A preservation suite and remains subject to the pre-existing simulator availability noted by the architectural-diff report.
