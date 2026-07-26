# Gate 3.3 Regression Before Branch Snapshot Work

| Suite | Result | Log |
|---|---:|---|
| Directed tests | 25/25 PASS | `reports/gate3_3/logs/directed_before.log` |
| Gate 1 regressions | 13/13 PASS | `reports/gate3_3/logs/gate1_before.log` |
| Minimal LSU tests | 14/14 PASS | `reports/gate3_3/logs/lsu_before.log` |
| HLS C++ complete trace vs frozen Gate 3.2 baseline | 5/5 byte-identical | `reports/gate3_3/logs/hls_cpp_trace_compare_before.log` |
| Vitis HLS csim complete trace vs frozen Gate 3.2 baseline | 5/5 byte-identical | `reports/gate3_3/logs/hls_csim_trace_compare_before.log` |
| BOOM vs HLS full-program architectural diff | 10/10 PASS | `reports/gate3_3/logs/full_program_diff_before.log` |
| Partial-order comparison | 8 legal reorders, 0 real exposed violations | `reports/gate3_3/logs/partial_order_before.log` |
| Baseline `boom_core_top` csynth | PASS | `reports/gate3_3/baseline_csynth_gate3_2.md` |

The effective baseline includes uncommitted Gate 3.2 changes present in the dirty worktree at freeze time. See `reports/gate3_3/git_status_before.txt`.
