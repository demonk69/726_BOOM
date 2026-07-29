# Gate 3.8 Final Regression Summary

Date: 2026-07-28

The unchanged accepted core source preserves all prior functional evidence.

| Check | Result |
|---|---|
| Directed tests | 25/25 PASS |
| Gate 1 regressions | 13/13 PASS |
| Minimal LSU tests | 14/14 PASS |
| Branch snapshot directed | 30/30 PASS |
| Branch snapshot random | 42/42 PASS |
| IQ compaction | 10/10 PASS |
| Frozen HLS C++ traces | 5/5 byte-identical |
| Frozen Vitis csim traces | 5/5 byte-identical |
| BOOM architectural diff | 10/10 PASS |
| Merged compilation unit | PASS |

The Gate 3.8 trace testbench additionally generates `load_store` and `tohost` traces. All seven normal programs pass C++ versus Vitis csim versus generated RTL architectural commit and `tohost` comparison.

| Program | C++ / csim / RTL |
|---|---|
| independent_alu | PASS |
| raw_chain | PASS |
| branch_taken | PASS |
| branch_not_taken | PASS |
| nested_branch | PASS |
| load_store | PASS |
| tohost | PASS |

Primary evidence is under `reports/gate3_8/regression_after_artifacts/` and `reports/gate3_8/traces/`.
