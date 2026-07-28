# Gate 3.7 Baseline Regression

Source: Gate 3.6 `FINAL_ACCEPTED_RESTORED` evidence at commit `94b4ce1`.

| Gate | Result |
|---|---:|
| g++ merged compile | PASS |
| Directed tests including reset/backpressure | 25/25 PASS |
| Gate 1 regressions | 13/13 PASS |
| Minimal LSU including stale response/reset | 14/14 PASS |
| Branch snapshot directed | 30/30 PASS |
| Branch snapshot random | 42/42 PASS |
| IQ compaction | 10/10 PASS |
| HLS C++ and Vitis csim traces | 10/10 byte-identical |
| BOOM vs HLS full-program diff | 10/10 PASS |
| Partial order | 8 legal reorders, 0 real violations |

This is C/native and Vitis csim evidence. It is not C/RTL cosim or strict BOOM cycle-equivalence evidence.
