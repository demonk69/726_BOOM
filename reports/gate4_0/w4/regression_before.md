# Gate 4.0 W4 Regression Baseline

Status: `W3_BASELINE_HASH_VALIDATED`; no W4/W4A regression was run.

| Frozen result | Baseline | Validation |
|---|---:|---|
| Software checks | 400/400 PASS across 197 runs | W3 summary and suite hashes match |
| Software suites | 15/15 PASS | `suite_results.csv` hash matches |
| Persistent random | 100/100 seeds; 6400/6400 cycles | zero dropped and zero duplicate tokens; hash matches |
| C++/csim normalized traces | 7/7 PASS | `trace_comparison.csv` hash matches |
| Full-program architectural diff | 10/10 PASS | hash matches |
| Focused generated RTL | 11/11 PASS | `rtl_test_matrix.csv` hash matches |
| Full-core XSim/trace comparison | 49/49 PASS | matrix and summary hashes match |
| Canonical csynth targets | 5/5 PASS | `resource_summary.csv` and five XML hashes match |
| Synthesis guardrails | PASS | hash matches |

The complete W3 artifact manifest (21 entries) and W3 source manifest (42 entries) were checked against current bytes. This validates the copied baseline without rewriting W3. It does not claim a W4A result.
