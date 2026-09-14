# Gate 5.4 PF5 Results

The product path now preserves resolved conditional direction in the ROB,
qualifies the live FTQ/P2 identity at architectural Commit, and submits one
canonical update through the existing Frontend predictor step before FTQ
retire/reclaim. The implementation, canonical PPA, and complete current-source
preservation matrix pass, so PF5 is accepted.

## Verified Current-Source Evidence

- Directed product path: 6,205/6,205, including 64 continuous commits.
- Small-state exhaustive: 128 combinations, 256 checks, zero errors.
- Persistent integrated random: 256 x 8,192 cycles, zero errors.
- Integrated long run: 1,000,000 steps, zero errors.
- Focused generated RTL: exactly 160/160 PASS.
- Product native and CSim: 12/12 each with exact counter probes and zero
  dropped, duplicate, or stale-accepted updates.
- PF4 random/long-run, PF4 directed, PF3 directed 481322, and PF2 directed
  2239 pass on current source.
- PF4 focused RTL 140/140, PF3 product RTL 12/12, PF2 focused RTL 116/116,
  and PF1 focused/full-core RTL 64/64 and 8/8 pass on current source.
- Canonical `boom_core_top`: 213436 LUT, 47337 FF, 16 BRAM, 3 DSP, 6.341 ns.
- Canonical synthesis: 9/9 tops PASS.
- Current-source full-core RTL: 12/12 programs and 2/2 mandatory fault cases.
- Twelve independently named PF5 assembly sources pass native, CSim, and RTL.
- PF1 through PF4 current-source preservation remains passing.
- PF2 full-core RTL passes 11/11. Its historical shadow-only steering condition
  is classified as expected later-gate architectural advance, not a regression.
- W3 software passes 400/400 and focused generated RTL passes 11/11.
- W4 exact accepted multi-writeback semantics pass 13/13.
- M3C passes 1,458 directed checks, 256 x 2,048 random cycles, and 15/15
  native full-core programs.
- R2 current-compatible native, CSim, and full-core RTL each pass 11/11. The
  older lifecycle-mismatched focused harness remains non-verdict diagnostic.
- B3I packet-aware random passes 256 x 4,096 with all integrity counters zero;
  native and fresh source-hash-keyed RTL each pass 6/6.

## Preservation Closure

- The complete required preservation matrix is current and passing.
- Canonical source, canonical synthesis summary, and historical `boom_all.cpp`
  hashes are unchanged from the PF5P freeze.
- PF5 canonical PPA is reused by unchanged source hash; no product source was
  modified during PF5P.
- Detailed provenance and hashes are in `pf5p_preservation_provenance.md` and
  `preservation_artifact_hashes.txt`.
- The final directed count is exactly 6,205: 61 finite directed checks plus
  2,048 stress iterations with three distinct invariants each. The earlier
  6,166 count was an intermediate checkpoint and is not a final gate value.
- PF2 and W3 generated-RTL rows in `rtl_test_matrix.csv` are synchronized to
  their current-source preservation evidence.

`PF5_FINAL_DIRECTED_EXPECTED_CHECKS=6205`

`PF5_DIRECTED_COUNT_PROVENANCE_VERIFIED=true`

`GATE5_4_PF5_COMMIT_BIM_TRAINING_VERIFIED=true`

`PRODUCT_COMMIT_BIM_TRAINING_INTEGRATED=true`

`PF5P_PRODUCT_BUG_FOUND=false`

`PF5P_PPA_REUSED_BY_SOURCE_HASH=true`

`PF5_EPHEMERAL_RTL_CLASSIFICATION_FIXED=true`

`PF5_TMP_REQUIRED_DURABLE_ARTIFACT_COUNT=0`

`READY_FOR_GATE5_4_PF6_FULL_RTL_PPA_ACCEPTANCE=true`

`NEXT_REQUIRED_ACTION=BEGIN_PF6_FULL_RTL_PPA_ACCEPTANCE`
