# Gate 3.5 Regression Summary

Date: 2026-07-26

| Variant | Functional Regression | Trace Preservation | BOOM Diff | Partial Order | Csynth |
|---|---|---|---|---|---|
| B1_PACKED_ALLOC_BITMAP | PASS | 10/10 byte-identical | 10/10 PASS | 8 legal, 0 real | PASS |
| B4_SET_ROLLBACK | PASS | 10/10 byte-identical | 10/10 PASS | 8 legal, 0 real | PASS |
| C1_RECOVERY_ENABLE | PASS | 10/10 byte-identical | 10/10 PASS | 8 legal, 0 real | PASS |
| D1_SHARED_BRANCH_MASKS | PASS | 10/10 byte-identical | 10/10 PASS | 8 legal, 0 real | PASS |
| D4_LOCAL_KILL_BITMAP | PASS | 10/10 byte-identical | 10/10 PASS | 8 legal, 0 real | PASS |
| D4_IQ_COMPACTION | PASS | 10/10 byte-identical | 10/10 PASS | 8 legal, 0 real | PASS |

Each variant ran:

| Gate | Result |
|---|---:|
| g++ merged compile | PASS |
| Directed tests | 25/25 PASS |
| Gate 1 regressions | 13/13 PASS |
| Minimal LSU tests | 14/14 PASS |
| Branch snapshot directed tests | 30/30 PASS |
| Branch snapshot random tests | 42/42 PASS |
| HLS C++ complete traces | 5/5 byte-identical |
| Vitis csim complete traces | 5/5 byte-identical |
| BOOM vs HLS full-program diff | 10/10 PASS |
| Partial-order comparison | 8 legal reorders, 0 real exposed violations |

D4-IQ additionally ran `tb/differential/iq_compaction_tests.cpp`: 10/10 PASS.

No variant was accepted as the Gate 3.5 PPA configuration because none met the minimum 10% full-core LUT reduction threshold.
