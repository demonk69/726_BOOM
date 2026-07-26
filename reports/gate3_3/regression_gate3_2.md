# Gate 3.2 Regression After Refactor

| Suite | Result | Log |
|---|---:|---|
| Directed tests | 25/25 PASS | `reports/gate3_2/logs/directed_after.log` |
| Gate 1 regressions | 13/13 PASS | `reports/gate3_2/logs/gate1_after.log` |
| Minimal LSU tests | 14/14 PASS | `reports/gate3_2/logs/lsu_after.log` |
| HLS C++ complete trace vs frozen baseline | 5/5 byte-identical | `reports/gate3_2/logs/hls_cpp_trace_compare_after.log` |
| Vitis HLS csim complete trace vs frozen baseline | 5/5 byte-identical | `reports/gate3_2/logs/hls_csim_trace_compare_after.log` |
| BOOM vs HLS full-program architectural diff | 10/10 PASS | `reports/gate3_2/logs/full_program_diff_after.log` |
| Partial-order comparison | 8 legal reorders, 0 real exposed violations | `reports/gate3_2/logs/partial_order_after.log` |

Trace status: unchanged. PC, instruction, rd, rd_value, memory address, memory data, memory mask, exception status, commit order, and cycle values match the frozen complete-trace baseline.
