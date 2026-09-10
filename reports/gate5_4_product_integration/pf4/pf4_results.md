# Gate 5.4 PF4 Results

PF4 integrates conditional prediction steering and precise prediction
resolution/recovery for the supported product subset.

## Verdict

```text
GATE5_4_PF4_BRANCH_PREDICTION_RECOVERY_VERIFIED=true
PRODUCT_COMMIT_BIM_TRAINING_INTEGRATED=false
PPA_REVIEW_REQUIRED=false
GATE5_4_PF4_PPA_BLOCKER=false
READY_FOR_GATE5_4_PF5_COMMIT_BIM_TRAINING=true
CORE_CYCLE_PIPELINED=false
```

## Functional Evidence

- Directed native: 12,949 checks, zero failures.
- Random: 256 seeds x 8,192 cycles, all classification, direction, target,
  stale-reference, recovery, FTQ, RVC, JAL/JALR, rename, training, and ordering
  errors zero.
- Long-run: 1,000,000 steps, zero errors.
- Focused generated RTL: 140/140.
- Product programs: native 12/12 and Vitis CSim 12/12.
- Current-source Product-FTQ full-core RTL: 12/12 architectural signatures.
- Predicted-T younger fault: actual-T mask and actual-NT correction/refetch/
  precise take both pass.
- Exact seeded BIM counters are unchanged after full-core RTL execution.

## Recovery Contract

Completion resolves the oldest pending branch, validates FTQ entry validity,
generation, live lane, and CFI identity, then compares actual direction and
target with retained prediction metadata. Correct conditional/JAL outcomes do
no recovery work. Conditional mispredicts and unpredicted JALR publish a
Frontend-owned redirect and squash only younger backend/FTQ state. Invalid or
stale metadata falls back to the actual architectural next PC.

## PPA

Canonical `boom_core_top` is 210,914 LUT, 46,551 FF, 16 BRAM, 3 DSP, and
6.341 ns. Relative to accepted PF3 this is +12,033 LUT (+6.050%), +1,108 FF,
0 BRAM, 0 DSP, and 0 ns. The review/blocker thresholds are not crossed.

All requested canonical diagnostic tops passed. See
`pf4_canonical_synthesis.csv`, `pf4_preservation_matrix.csv`, and
`full_core_rtl/generation_provenance.md`.
