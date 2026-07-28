# Gate 3.6 Regression Summary

Date: 2026-07-27

| Configuration | Functional Regression | Trace Preservation | BOOM Diff | Partial Order | Csynth/Decision |
|---|---|---|---|---|---|
| T3_FUNCTION_BOUNDARIES | PASS | 10/10 byte-identical | 10/10 PASS | 8 legal, 0 real | PASS; `REJECTED_PPA` |
| T4_RESET_CONSOLIDATION | PASS_CSIM_ONLY | 10/10 byte-identical | 10/10 PASS | 8 legal, 0 real | PASS; `REJECTED_RESET_SEMANTICS` |
| FINAL_ACCEPTED_RESTORED | PASS | 10/10 byte-identical | 10/10 PASS | 8 legal, 0 real | accepted baseline restored |

Each executed source variant ran:

| Gate | Result |
|---|---:|
| g++ merged compile | PASS |
| Directed tests including reset/backpressure | 25/25 PASS |
| Gate 1 regressions | 13/13 PASS |
| Minimal LSU including stale response/reset | 14/14 PASS |
| Branch snapshot directed tests | 30/30 PASS |
| Branch snapshot random tests | 42/42 PASS |
| IQ compaction tests | 10/10 PASS |
| HLS C++ and Vitis csim complete traces | 10/10 byte-identical |
| BOOM vs HLS full-program diff | 10/10 PASS |
| Partial-order comparison | 8 legal reorders, 0 real exposed violations |

T4 trace results do not validate synthesized reset behavior: HLS reset pragmas do not change native C++ semantics and these traces do not assert the generated RTL reset in the middle of execution. T4 is therefore rejected despite functional and csim trace preservation.

T1, T2, and T5 did not change source because direct source/report evidence showed no corresponding defect or meaningful contribution to the target delta.
