# Cycle Semantics

Status: `STRICT_CYCLE_EQUIVALENCE=false`.

Gate 3.1A corrected the old global event-order comparison by matching dynamic uops and checking only exposed partial-order constraints. That method found 8 legal reorder events and 0 real exposed partial-order violations.

Gate 3.1C extends architectural comparison through the retired store-to-`tohost`; it does not change the cycle-equivalence conclusion. BOOM exposes branch-resolution events that can interleave ahead of older commits, while the HLS trace is serialized around the current core-step/testbench observation points.

Generated artifacts:

- `reports/equivalence/provisional_gate3_1/intra_uop_latency.csv`
- `reports/equivalence/provisional_gate3_1/dependency_latency.csv`
- `reports/equivalence/provisional_gate3_1/concurrency_comparison.csv`

Conclusion: architectural behavior is validated for the current directed subset, including the minimal LSU store-to-`tohost` path, but strict cycle equivalence and RAW/WAR/WAW timing equivalence remain insufficient-signal.
