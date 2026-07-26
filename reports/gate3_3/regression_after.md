# Gate 3.3 Regression After Branch Snapshot Work

Date: 2026-07-26

| Suite | Result | Log |
|---|---:|---|
| Directed tests | 25/25 PASS | `reports/gate3_3/logs/directed_after.log` |
| Gate 1 regressions | 13/13 PASS | `reports/gate3_3/logs/gate1_after.log` |
| Minimal LSU tests | 14/14 PASS | `reports/gate3_3/logs/lsu_after.log` |
| Branch snapshot directed tests | 30/30 PASS | `reports/gate3_3/logs/branch_snapshot_tests.log` |
| Branch snapshot random tests | 2/2 PASS | `reports/gate3_3/logs/branch_snapshot_random_tests.log` |
| HLS C++ complete trace vs frozen Gate 3.2 baseline | 5/5 byte-identical | `reports/gate3_3/logs/hls_cpp_trace_compare_after.log` |
| Vitis HLS csim complete trace vs frozen Gate 3.2 baseline | 5/5 byte-identical | `reports/gate3_3/logs/hls_csim_trace_compare_after.log` |
| BOOM vs HLS full-program architectural diff | 10/10 PASS | `reports/gate3_3/full_program_architectural_diff.md` |
| Partial-order comparison | 8 legal reorders, 0 real exposed violations | `reports/gate3_3/logs/partial_order_after.log` |

The supported architectural traces remain byte-identical to the frozen Gate 3.2 complete-trace baseline. Gate 3.3 adds focused branch snapshot and rollback coverage without changing the accepted architectural subset traces.
