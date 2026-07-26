# Gate 3.4 Baseline Manifest

Date: 2026-07-26

Frozen Git commit: `6645b3df8fdf2828713513ad22cf4dcceb0a89f0`

Worktree status at freeze: dirty. Gate 3.4 starts from the effective Gate 3.3 accepted baseline plus uncommitted Gate 3.2/Gate 3.3 artifacts. Captured in `reports/gate3_4/git_status_before.txt`.

## Frozen Artifacts

| Artifact | Path |
|---|---|
| Source hashes before Gate 3.4 | `reports/gate3_4/source_hashes_before.txt` |
| Git commit before Gate 3.4 | `reports/gate3_4/git_commit_before.txt` |
| Git status before Gate 3.4 | `reports/gate3_4/git_status_before.txt` |
| Modified files before Gate 3.4 | `reports/gate3_4/modified_files_before.txt` |
| Baseline resources | `reports/gate3_4/baseline_resources.csv` |
| HLS C++ baseline traces | `reports/gate3_4/baseline_traces/hls_cpp/` |
| Vitis csim baseline traces | `reports/gate3_4/baseline_traces/hls_csim/` |
| BOOM vs HLS baseline diff | `reports/gate3_4/full_program_architectural_diff_baseline.md` |
| Branch snapshot baseline results | `reports/gate3_4/branch_snapshot_test_results_baseline.md` |
| Gate 3.3 branch recovery diagnostics | `reports/gate3_4/branch_recovery_csynth_diagnostics_gate3_3.csv` |

## Baseline Status

| Check | Result |
|---|---|
| Conservative `boom_core_top` csynth | PASS, 71.69s, 5.898 ns, 83286 LUT, 16611 FF, 16 BRAM_18K, 3 DSP |
| `CORE_CYCLE` pipeline | Disabled by default |
| Directed tests | 25/25 PASS |
| Gate 1 regressions | 13/13 PASS |
| Minimal LSU tests | 14/14 PASS |
| Branch snapshot directed tests | 30/30 PASS |
| Branch snapshot random tests | 2/2 PASS |
| HLS C++ complete trace preservation | 5/5 byte-identical |
| Vitis HLS csim complete trace preservation | 5/5 byte-identical |
| BOOM vs HLS full-program architectural diff | 10/10 PASS |
| Partial-order comparison | 8 legal reorders, 0 real exposed violations |

Gate 3.4 does not overwrite Gate 3.3 reports.
