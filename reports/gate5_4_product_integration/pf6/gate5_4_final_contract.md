# Gate 5.4 Final Contract

Product scope is frozen at static target predecode plus a 256-entry, 2-bit,
LUTRAM, lazy-valid BIM with `(pc >> 1) & 255` indexing, update-forward-new-value
same-index behavior, 32-entry LUTRAM/control-only-reset FTQ, prediction steering,
resolution comparison, recovery, and exactly-once architectural Commit training.

The FTQ entry is 211 bits. Its reference is 40 bits: 5 index, 32 generation,
lane and halfword identity. One nonempty final packet atomically enqueues once and
allocates exactly one FTQ entry. Reclaim may remove at most one zero-live head and
allow same-step allocation. Stale lookup and stale training have no side effect.

Implemented and frozen: P1 predecode, BIM predictor, FTQ, conditional steering,
predicted-vs-actual comparison, branch recovery, FTQ prediction lookup, and
Commit BIM training. Out of scope: BTB, RAS, GHR, TAGE, ICache, full LSU, FPU,
pipeline widening, and any new architecture feature.

`PF6_PRODUCT_SOURCE_CHANGED=false` and `TEST_SEMANTICS_CHANGED=false`. Runner
plumbing only adds output/workspace overrides, skips an already byte-verified
merged-source rewrite, repairs legacy source lists, and adds PF6-only checks.
