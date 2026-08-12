# Gate 5.2 R2 Regression After

All statements below are tied to final logs or copied final matrices under this R2 evidence directory.

| Check | Final result | Evidence |
|---|---:|---|
| Focused native RVC fetch | PASS, 4,111 assertions/414 cases, 0 failures | `/home/lab_726/boom/hls_boom/reports/gate5_2_rvc/r2/logs/rvc_fetch_tests.log` |
| Persistent RVC fetch random | 256/256 seeds x 2,048 cycles, 0 errors | `/home/lab_726/boom/hls_boom/reports/gate5_2_rvc/r2/logs/rvc_fetch_random_tests.log` |
| Native throughput audit | Four scenarios PASS | `logs/rvc_throughput.log`, `throughput_analysis.md` |
| R1 decompressor/Decode cross preservation | 65,536/65,536 and 38,294/38,294 PASS | `logs/rvc_decompress_tests.log`, `logs/rvc_decode_cross_tests.log` |
| Focused R2 generated RTL | 58/58 PASS | `/home/lab_726/boom/hls_boom/reports/gate5_2_rvc/r2/logs/xsim.stdout.log` |
| Mixed full-core native/csim/RTL | 10/10, 10/10, 10/10 PASS | `logs/r2_native.log`, `logs/r2_csim.log`, `logs/r2_rtl/*.log`, `r2_full_core_program_matrix.csv` |
| Gate 5.1 focused RTL preservation | 33/33 PASS | `logs/gate5_1_preservation/xsim.stdout.log` |
| W3 software | 400/400 PASS | `old_regression/current_w4e/regression/w4e/product_full/regression_after.md` |
| W4 directed/persistent random | 95/95 and 128/128 PASS | `old_regression/current_w4e/regression_after.md` |
| Gate 3.9 generated RTL | 49/49 PASS | `old_regression/current_gate3_9/rtl_run_status.csv` |
| RV64M native/csim/RTL full-core | 15/15 each PASS | `old_regression/current_m3c/logs/rv64m_full_core_tests.log`, `old_regression/current_m3c_csim/vitis_csim.log`, `old_regression/current_m3c_full_rtl/full_core_rtl_matrix.csv` |
| M3C focused RTL | 30/30 PASS | `old_regression/current_m3c_focused_rtl/rtl_test_matrix.csv` |
| W3/W4 focused RTL | 11/11 and 20/20 PASS | `old_regression/current_w3_focused_rtl/rtl_test_matrix.csv`, `old_regression/current_w4_focused_rtl/rtl_test_matrix.csv` |
| Full-program/partial-order | 10/10 and 7/7 PASS | `old_regression/current_w4e/regression/w4e/product_full/regression_after.md` |

No regression evidence changes the backend topology or widths. Dispatch/decode/commit remain one wide, fixed MEM/INT execute acceptance remains at most two, completion sources remain three, PRF writes remain two, and the full LSU remains unimplemented.
