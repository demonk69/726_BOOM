# Gate 5.3 B2 Scenario Evidence Mapping

| Named scenario | Honest existing/new evidence | Scope |
|---|---|---|
| `buffer_decode_stall` | `../rtl_test_matrix.csv`: `decode_stall_retains_head`, `stall_release`; `../decoupling_metrics.csv` | Focused generated RTL plus native decoupling |
| `buffer_redirect` | `../rtl_test_matrix.csv`: `redirect_flush`, `redirect_no_old_pop` | Focused generated RTL |
| `buffer_rvc_mix` | `full_core_rtl_matrix.csv`: mixed RVC 11-program XSim matrix; `../rtl_test_matrix.csv`: RVC/cross-word cases | Generated full-core RTL plus focused generated RTL |
| `buffer_fault_flush` | `../rtl_test_matrix.csv`: `fault_entry`, `fault_cause`, `fault_flush_no_late_exception` | Focused generated RTL |

No new full-core programs are claimed for these names. The 11 reused programs retain their original Gate 5.2 identities.
