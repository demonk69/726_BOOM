# Gate 3.7 Final Accepted-Source Regression

| Gate | Result | Evidence |
|---|---:|---|
| g++ merged compile | PASS | `regression_after_artifacts/logs/merged_compile.log` |
| Directed tests including C++ reset/backpressure | 25/25 PASS | `regression_after_artifacts/logs/directed_tests.log` |
| Gate 1 regressions | 13/13 PASS | `regression_after_artifacts/logs/gate1_regression_tests.log` |
| Minimal LSU including stale response/C++ reset | 14/14 PASS | `regression_after_artifacts/logs/lsu_minimal_tests.log` |
| Branch snapshot directed | 30/30 PASS | `regression_after_artifacts/logs/branch_snapshot_tests.log` |
| Branch snapshot random | 42/42 PASS | `regression_after_artifacts/logs/branch_snapshot_random_tests.log` |
| IQ compaction | 10/10 PASS | `regression_after_artifacts/logs/iq_compaction_tests.log` |
| HLS C++ and Vitis csim traces | 10/10 byte-identical | `regression_after_artifacts/trace_diff.md` |
| BOOM vs HLS full-program diff | 10/10 PASS | `regression_after_artifacts/full_program_architectural_diff.md` |
| Partial order | 8 legal reorders, 0 real violations | `regression_after_artifacts/logs/partial_order.log` |

The source reset directive is retained and no pipeline directive is present in accepted source. These tests are native C++ and Vitis csim evidence; they do not validate generated RTL iteration overlap, pin-level AXIS backpressure, or mid-run `ap_rst_n` behavior.
