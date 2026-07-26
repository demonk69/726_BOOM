# Gate 3.3 Baseline Manifest

Date: 2026-07-26

Frozen Git commit: `6645b3df8fdf2828713513ad22cf4dcceb0a89f0`

Worktree status at freeze: dirty. The uncommitted Gate 3.2 synthesis-closure files and reports are part of the effective Gate 3.3 baseline. Captured in `reports/gate3_3/git_status_before.txt`.

## Frozen Artifacts

| Artifact | Path |
|---|---|
| Source hashes before Gate 3.3 | `reports/gate3_3/source_hashes_before.txt` |
| Git commit before Gate 3.3 | `reports/gate3_3/git_commit_before.txt` |
| Git status before Gate 3.3 | `reports/gate3_3/git_status_before.txt` |
| Gate 3.2 baseline csynth summary | `reports/gate3_3/baseline_csynth_gate3_2.md` |
| Gate 3.2 top csynth modes | `reports/gate3_3/top_csynth_modes_gate3_2.csv` |
| Gate 3.2 regression summary | `reports/gate3_3/regression_gate3_2.md` |
| Directed test log | `reports/gate3_3/logs/directed_before.log` |
| Gate 1 regression log | `reports/gate3_3/logs/gate1_before.log` |
| Minimal LSU regression log | `reports/gate3_3/logs/lsu_before.log` |
| HLS C++ trace compare log | `reports/gate3_3/logs/hls_cpp_trace_compare_before.log` |
| Vitis HLS csim trace compare log | `reports/gate3_3/logs/hls_csim_trace_compare_before.log` |
| BOOM vs HLS full-program diff log | `reports/gate3_3/logs/full_program_diff_before.log` |
| Partial-order log | `reports/gate3_3/logs/partial_order_before.log` |
| HLS C++ complete traces | `reports/gate3_3/baseline_traces/hls_cpp/` |
| Vitis HLS csim complete traces | `reports/gate3_3/baseline_traces/hls_csim/` |

## Baseline Status

| Check | Result |
|---|---|
| Directed tests | 25/25 PASS |
| Gate 1 regressions | 13/13 PASS |
| Minimal LSU tests | 14/14 PASS |
| HLS C++ complete trace vs frozen Gate 3.2 baseline | 5/5 byte-identical |
| Vitis HLS csim complete trace vs frozen Gate 3.2 baseline | 5/5 byte-identical |
| BOOM vs HLS full-program architectural diff | 10/10 PASS |
| Partial-order comparison | 8 legal reorders, 0 real exposed violations |
| Baseline `boom_core_top` csynth | PASS, 45.56s, 5.898 ns, 40625 LUT, 15985 FF, 16 BRAM_18K, 3 DSP |
| Performance `CORE_CYCLE` pipeline experiment | TIMEOUT after 15 minutes; not part of baseline |
| Official Chipyard/FESVR/DRAMSim Gate 3 | BLOCKED |

Gate 3.3 changes must preserve these functional results unless an internal-cycle-only change is explicitly documented.
