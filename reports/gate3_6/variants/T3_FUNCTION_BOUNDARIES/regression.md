# T3_FUNCTION_BOUNDARIES Regression

| Gate | Result | Log |
|---|---:|---|
| g++ merged compile | PASS | logs/merged_compile.log |
| Directed tests including reset/backpressure | 25/25 PASS | logs/directed_tests.log |
| Gate 1 regressions | 13/13 PASS | logs/gate1_regression_tests.log |
| Minimal LSU including stale response/reset | 14/14 PASS | logs/lsu_minimal_tests.log |
| Branch snapshot directed | 30/30 PASS | logs/branch_snapshot_tests.log |
| Branch snapshot random | 42/42 PASS | logs/branch_snapshot_random_tests.log |
| IQ compaction | 10/10 PASS | logs/iq_compaction_tests.log |
| HLS C++ and Vitis csim traces | 10/10 byte-identical | trace_diff.md |
| BOOM vs HLS full-program diff | 10/10 PASS | full_program_architectural_diff.md |
| Partial order | 8 legal reorders, 0 real violations | logs/partial_order.log |
