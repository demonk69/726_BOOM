# D4_IQ_COMPACTION Regression

| Gate | Result | Log |
|---|---:|---|
| g++ merged compile | PASS | logs/merged_compile.log |
| Directed tests | 25/25 PASS | logs/directed_tests.log |
| Gate 1 regressions | 13/13 PASS | logs/gate1_regression_tests.log |
| Minimal LSU tests | 14/14 PASS | logs/lsu_minimal_tests.log |
| Branch snapshot directed tests | 30/30 PASS | logs/branch_snapshot_tests.log |
| IQ compaction directed tests | PASS when present | logs/iq_compaction_tests.log |
| Branch snapshot random tests | 42/42 PASS | logs/branch_snapshot_random_tests.log |
| HLS C++ traces | Compared in trace_diff.md | logs/hls_cpp_trace.log |
| Vitis csim traces | Compared in trace_diff.md | logs/hls_csim_trace.log |
| BOOM vs HLS full-program diff | See full_program_architectural_diff.md | logs/full_program_arch_diff.log |
| Partial-order comparison | 8 legal reorders, 0 real exposed violations | logs/partial_order.log |

Additional branch snapshot directed coverage includes wrong-path store suppression, free-list near exhaustion, nested branch rollback, ROB wrap branch recovery, IQ full branch recovery, IMEM stale response, DMEM stale response, and trace backpressure scenarios from the accepted Gate 3.3/Gate 3.4 suite.
