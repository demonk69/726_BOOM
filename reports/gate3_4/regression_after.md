# Gate 3.4 Regression After Attribution Framework

Date: 2026-07-26

| Suite | Result | Log |
|---|---:|---|
| Directed tests | 25/25 PASS | `reports/gate3_4/logs/directed_after.log` |
| Gate 1 regressions | 13/13 PASS | `reports/gate3_4/logs/gate1_after.log` |
| Minimal LSU tests | 14/14 PASS | `reports/gate3_4/logs/lsu_after.log` |
| Branch snapshot directed tests | 30/30 PASS | `reports/gate3_4/logs/branch_snapshot_tests.log` |
| Branch snapshot random tests | 2/2 PASS | `reports/gate3_4/logs/branch_snapshot_random_tests.log` |
| HLS C++ complete trace vs frozen Gate 3.3 baseline | 5/5 byte-identical | `reports/gate3_4/logs/hls_trace_compare_after.log` |
| Vitis HLS csim complete trace vs frozen Gate 3.3 baseline | 5/5 byte-identical | `reports/gate3_4/logs/hls_trace_compare_after.log` |
| BOOM vs HLS full-program architectural diff | 10/10 PASS | `reports/gate3_4/full_program_architectural_diff.md` |
| Partial-order comparison | 8 legal reorders, 0 real exposed violations | `reports/gate3_4/logs/partial_order_after.log` |

Gate 3.4 added attribution-only synthesis tops and scripts. No branch recovery behavior, state capacity, or core cycle pipeline setting was changed.
