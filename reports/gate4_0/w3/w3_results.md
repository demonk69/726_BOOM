# Gate 4.0 W3 Results

Status: `W3_DUAL_EXECUTE_ACCEPTANCE_VERIFIED`.

The fixed MEM lane and fixed INT lane can now both be accepted and executed in one cycle for the supported HLS subset. Independent lane backpressure retains blocked grants, two completion slots preserve results until serialized completion/writeback accepts them, allocation identity rejects stale completions, and branch/reset handling clears killed pending work. Lane 2 remains the unsupported FP lane.

## Acceptance Evidence

| Requirement | Result | Canonical evidence |
|---|---:|---|
| Software checks | PASS, 400/400 | `regression/regression_after.md`, `regression/suite_results.csv` |
| Persistent random campaign | PASS, 100 seeds x 64 cycles | `regression/random_metrics.csv` |
| Random token accounting | PASS, 0 dropped and 0 duplicated | `regression/random_metrics.csv` |
| Focused generated RTL | PASS, 11/11 | `rtl_test_matrix.csv` |
| Full-core XSim and trace comparison | PASS, 49/49 | `full_core_rtl_summary.json`, `full_core_rtl_matrix.csv` |
| Canonical csynth targets | PASS, 5/5 | `resource_summary.csv`, `csynth/*/*_csynth.xml` |
| Synthesis guardrails | PASS | `guardrail_audit.md` |

The full-core matrix records `PASS` for XSim, normalization, and architecture comparison in every case. The software campaign additionally records 7/7 normalized C++/csim trace comparisons, 21/21 normalized architecture/event/cycle checks, and 10/10 full-program architectural comparisons.

W3 acceptance covers only the modular implementation translation units under `src/*.cpp`, the generated `src/boom_core_merged.cpp`, and the public `src/boom_core_top.cpp` top, as enumerated in `source_scope.md` and the source hash manifests. `src/boom_all.cpp` is a legacy, non-canonical monolithic snapshot that is not synchronized with the modular implementation. No active build, test, regression, RTL-generation, or csynth flow references it; it is excluded from W3 evidence and acceptance.

## W2 Delta

W2 generated simultaneous oldest-ready MEM/INT grants but accepted at most one. W3 raises fixed-lane acceptance and execute intake to two, adds two retained completion slots with oldest-first serialized completion/writeback service, makes blocked-IQ dispatch retry explicit, and preserves branch kill, reset, stale-allocation rejection, and independent lane backpressure.

The W3 `boom_core_top` estimate is 68055 LUT, 16149 FF, 15 BRAM_18K, 3 DSP, and 5.898 ns. Relative to the W2 checkpoint, this is +6295 LUT, +936 FF, +3 BRAM_18K, unchanged DSP, and unchanged estimated period. These are Vitis HLS 2021.2 estimates for a 10 ns target, not post-place-and-route timing or accepted product PPA. `CORE_CYCLE`, dispatch width, and commit width remain unpipelined/one.

## Scope And Decision

`READY_FOR_W4_MULTI_WAKEUP_WRITEBACK=true` because every defined W3 acceptance artifact passes and its hashes are frozen in this directory. This authorizes the next experiment only; no W4 multi-wakeup/writeback implementation is present or claimed. W3 still serializes completion/writeback and commit, the frontend/rename/dispatch path remains one-wide, and FP remains unsupported.

| Decision | Value |
|---|---|
| `W3_DUAL_EXECUTE_ACCEPTANCE_VERIFIED` | true |
| `READY_FOR_W4_MULTI_WAKEUP_WRITEBACK` | true |
| `READY_FOR_OFFICIAL_GATE_3` | false |
| `M009` | `PARTIALLY_VERIFIED` |
| `M014` | `VERIFIED` |

Official Gate 3 remains false because the external Chipyard/FESVR/DRAMSim full-system path and required dependencies are unavailable. The evidence verifies the supported HLS subset and generated RTL; it does not establish strict full-BOOM cycle equivalence.

The pre-existing modified tracked logs and untracked backup logs recorded by `git_status_before.txt` are excluded, non-deliverable workspace state. They were not modified, used as evidence, or added to an artifact manifest.

`artifact_manifest.csv` excludes itself to prevent a recursive hash. All manifest entries were independently checked against the current files after finalization.
