# Gate 5.4 PR0 Fetch Buffer Preservation Gap Audit

## Verdict

The gap is explained. Gate 5.3 accepted a packet-aware B3I random harness, and that exact runner reproduces its recorded 256 x 4096 all-zero result at the accepted checkpoint. It also passes current baseline-equivalent and PF1 source. The `425388/174170` result comes from selecting the unchanged B2 scalar-producer test after B3I legally introduced atomic two-lane packets.

This is `RUNNER_DRIFT`, expressed in the initial taxonomy as `D. EXPECTATION_DRIFT`; it is not PF1 causation and not a product regression.

## Evidence Summary

| Evidence | Result |
|---|---|
| Accepted checkpoint + accepted test | PASS, all error counters zero |
| Accepted checkpoint + current-selected B2 test | FAIL, `425388/174170` |
| Current baseline + accepted test | PASS, all error counters zero |
| Current baseline + current-selected B2 test | FAIL, `425388/174170` |
| PF1 source + accepted test | PASS, all error counters zero |
| First divergence | seed 0, cycle 7, omitted second packet lane in old oracle |

## Final Flags

```text
GATE5_4_PR0_PRESERVATION_GAP_REVIEWED=true
ACCEPTED_GATE5_3_RANDOM_EXPECTED_ZERO=true

CURRENT_RANDOM_DROP=425388
CURRENT_RANDOM_ORDERING_ERROR=174170

PF1_EQUIVALENT_DROP=425388
PF1_EQUIVALENT_ORDERING_ERROR=174170

PF1_CAUSED_REGRESSION=false

ACCEPTED_CHECKPOINT_REPRODUCED_PASS=true

PRESERVATION_GAP_ROOT_CAUSE=RUNNER_DRIFT

FIRST_FAIL_SEED=0
FIRST_FAIL_CYCLE=7

PRESERVATION_GAP_EXPLAINED=true

PRODUCT_FIX_REQUIRED=false
TEST_HARNESS_FIX_REQUIRED=true

READY_FOR_PR1_FETCH_BUFFER_GAP_REPAIR=false

READY_FOR_GATE5_4_PF2_PRODUCT_PREDICTOR_FRONTEND=true
```

`TEST_HARNESS_FIX_REQUIRED=true` means restoration/versioning of the preservation invocation, not weakening or changing either test's expected behavior. No product source, test source, `src/boom_all.cpp`, Predictor, FTQ, or PF2 implementation was modified.
