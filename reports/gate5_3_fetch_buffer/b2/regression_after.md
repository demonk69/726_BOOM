# Gate 5.3 B2 Phase E Preservation Regressions

All results below are based only on this current B2 modular-source run.

| Requirement | Status | Current-run result | Evidence |
|---|---:|---:|---|
| Gate 5.1 focused generated RTL | PASS | 33/33 | `regression/logs/gate5_1_xsim.log` |
| Gate 5.2 RVC exhaustive and Decode cross | PASS | 65,536/65,536; 38,551/38,551 | `regression/logs/rvc_decompress.log; regression/logs/rvc_decode_cross.log` |
| W3 canonical software | PASS | 400/400 | `regression/w4e/regression/w4e/product_full/regression_after.md` |
| W4E | PASS | 95/95 directed; 128/128 random seeds | `regression/w4e/regression_after.md` |
| Gate 3.9 generated RTL | PASS | 49/49 | `regression/gate3_9/rtl_test_matrix.csv` |
| M3C/RV64M native | PASS | directed/random and 15/15 | `regression/m3c/native/logs` |
| M3C/RV64M csim | PASS | 15/15 | `regression/m3c/csim/vitis_csim.log` |
| M3C/RV64M full-core RTL | PASS | 15/15 | `regression/m3c/full_core_rtl/full_core_rtl_matrix.csv` |
| M3C focused RTL | PASS | 30/30 | `regression/m3c/focused_rtl/rtl_test_matrix.csv` |
| W3 focused RTL | PASS | 11/11 | `regression/w3_w4_focused/w3_current/rtl_test_matrix.csv` |
| W4 focused RTL | PASS | 20/20 | `regression/w3_w4_focused/rtl_test_matrix.csv` |
| Full-program architectural diff | PASS | 10/10 | `regression/w4e/regression/w4e/product_full/full_program_architectural_diff.csv` |
| Partial-order | PASS | 7/7 | `regression/w4e/regression/w4e/product_full/logs/partial_order.log` |

Phase E closed: **YES**.
Canonical csynth phase F was run after this preservation gate and passed 11/11; see `phase_f_summary.md`.
