# Provisional Gate 3.1 Results

Date: 2026-07-24

Current executed scope: Gate 3.1A plus Gate 3.1C minimal LSU/store-to-`tohost` validation.

Gate 3.1A status: PARTIAL_MATCH with LEGAL_REORDER classification.

Gate 3.1C status: FULL_LOADED_PROGRAM_ARCH_PASS for the minimal LSU/tohost subset.

OLD_EVENT_ORDER_FAIL=VALIDATION_METHOD_FALSE_POSITIVE

READY_FOR_GATE_3=false

STRICT_CYCLE_EQUIVALENCE=false

## Gate 3.1A Summary

| Check | Result | Evidence |
|---|---|---|
| Dynamic uop map | GENERATED | `reports/equivalence/provisional_gate3_1/dynamic_uop_map.csv` |
| First old event-order failure classification | LEGAL_REORDER | `reports/equivalence/provisional_gate3_1/first_event_order_failure.md` |
| Partial-order comparison | PARTIAL_MATCH | `reports/equivalence/provisional_gate3_1/partial_order_diff.csv` |
| Real partial-order violations | 0 | `reports/equivalence/provisional_gate3_1/partial_order_diff.md` |
| Legal reorder events | 8 | `reports/equivalence/provisional_gate3_1/partial_order_diff.md` |
| Cycle-semantics conclusion | STRICT_CYCLE_EQUIVALENCE=false | `reports/equivalence/provisional_gate3_1/cycle_semantics.md` |
| Commit order | MATCH | all five prefix programs |
| RAW/WAR/WAW timing | INSUFFICIENT_SIGNAL | issue, wakeup, rename-source, and complete events are not exposed |
| Branch squash architectural effect | MATCH for committed prefix | no committed wrong-path PC appears in BOOM/HLS matched prefix |

## Gate 3.1C Summary

| Check | Result | Evidence |
|---|---|---|
| Minimal LSU regressions | 14/14 PASS | `reports/equivalence/provisional_gate3_1/lsu_test_results.md` |
| Existing directed regressions | 25/25 PASS | `reports/equivalence/provisional_gate3_1/lsu_test_results.md` |
| Existing Gate 1 regressions | 13/13 PASS | `reports/equivalence/provisional_gate3_1/lsu_test_results.md` |
| HLS C++ complete traces through `tohost` | 5/5 PASS | `reference/hls_traces/*_hls_cpp_full.jsonl` |
| Vitis HLS csim complete traces through `tohost` | 5/5 PASS | `reference/hls_traces/*_hls_csim_full.jsonl` |
| BOOM vs HLS full loaded-program architectural diff | 10/10 PASS | `reports/equivalence/provisional_gate3_1/full_program_architectural_diff.csv` |
| Vitis HLS baseline csynth | TIMEOUT | `reports/equivalence/provisional_gate3_1/hls_csynth.log` |

## Interpretation

The old BOOM-vs-HLS event-order failure compared unrelated stream positions. For example, it compared a BOOM branch-resolution event for a younger branch uop against a HLS commit event for an older integer uop. That is not a valid out-of-order equivalence check.

The corrected partial-order method matches dynamic uops by committed program-order identity and then checks only same-uop order, commit order, and available architectural branch/commit constraints. Under the exposed signal set, no real functional partial-order violation is detected.

This does not upgrade microarchitectural equivalence to verified. The traces still lack enough signal coverage to prove RAW wakeup/issue timing, WAR rename-source capture, WAW physical map transitions, IQ grant uniqueness, and full wrong-path squash identity. Gate 3.1C also does not implement a full BOOM LSU/cache/MMU; it only validates the conservative integer LSU path needed for these directed load/store tests and retired store-to-`tohost` completion.

## Gate 3.1C Decision

Gate 3.1A confirms that the old event-order failure is a validator false positive, not an architectural or exposed partial-order hardware error. The next allowed step is Gate 3.1C minimal LSU/store-to-`tohost` work, with the explicit limitation that it cannot be claimed as a full BOOM LSU implementation.

## Current Readiness

| Flag | Value | Reason |
|---|---|---|
| READY_FOR_PPA_OPTIMIZATION | false | full-program architectural comparison passes, but baseline csynth after LSU timed out before a report was produced |
| READY_FOR_OFFICIAL_GATE_3 | false | official Chipyard/FESVR simulator remains unavailable |
