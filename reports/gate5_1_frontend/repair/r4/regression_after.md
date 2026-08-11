# Gate 5.1R R4 Regression After

All required current-source and generated-RTL gates pass.

| Gate | Result | Evidence |
|---|---|---|
| Focused Frontend generated RTL | 33/33 PASS | `repair/r2/rtl_test_matrix.csv` |
| W3 software/current-source campaign | 400/400 PASS | `repair/r3/w3/regression_after.md` |
| C++ vs csim normalized traces | 7/7 PASS | `repair/r3/w3/trace_comparison.csv` |
| Architecture/event/cycle normalized checks | 21/21 PASS | `repair/r3/w3/normalized_trace_diff.csv` |
| Full-program architectural diff | 10/10 PASS | `repair/r3/w3/full_program_architectural_diff.csv` |
| Partial order | 7/7 PASS | `repair/r3/w3/logs/partial_order.log` |
| M3C native full-core RV64M | 15/15 PASS | `repair/r4/m3c/logs/rv64m_full_core_tests.log` |
| M3C Vitis csim full-core RV64M | 15/15 PASS | `repair/r4/m3c_csim/vitis_csim.log` |
| M3C focused generated RTL | 30/30 PASS | `repair/r4/m3c_rtl/rtl_test_matrix.csv` |
| M3C generated full-core RTL | 15/15 PASS | `repair/r4/m3c_full_core_rtl/full_core_rtl_matrix.csv` |
| W3 focused generated RTL | 11/11 PASS | `repair/r4/w4_rtl_final/w3_current/rtl_test_matrix.csv` |
| W4 focused generated RTL | 20/20 PASS | `repair/r4/w4_rtl_final/rtl_test_matrix.csv` |
| Gate 3.9 full-core generated RTL | 49/49 PASS | `repair/r4/gate3_9/rtl_test_matrix.csv` |
| Canonical csynth | 9/9 PASS | `../gate5_1_frontend_repair_r4/module_csynth_summary.csv` |

The Gate 3.9 run was rebuilt from `boom_hls_gate5_1_frontend_repair_r4_boom_core_top/solution_module/syn/verilog` after fixing testbench response packing to echo the request epoch. No expected result was modified.
