# PF5CR Commit Evidence Consistency Repair

PF5CR changes evidence and provenance only. Canonical product source, program
semantics, RTL DUTs, directives, accepted tests, and PPA are unchanged.

## Directed Count

The accepted native runner requires the exact sentinel
`PF5_COMMIT_BIM_TRAINING_PASS checks=6205 failures=0 continuous_commits=64`.
The test contributes 61 finite directed checks plus 2,048 stress iterations
that independently check update emission, exact FTQ/BIM index identity, and
pending-update consumption (`61 + 2048 x 3 = 6205`). These are distinct
invariants, not duplicate or diagnostic accounting. The earlier 6,166 value
was an intermediate checkpoint and is not used by current final evidence.

## Matrix Repair

- PF2 full-core current-source RTL: `11/11 PASS`, evidenced by
  `preservation/pf2_full_core/r2_full_core_rtl_matrix.csv`.
- W3 focused current-source RTL: `11/11 PASS`, evidenced by
  `preservation/w3/focused/rtl_test_matrix.csv`.
- W3 software remains `400/400 PASS` in
  `preservation/w3/software/regression_after.md`.

## Artifact Classification

The generated PF4/PF5 full-core RTL entity formerly listed by its
`/tmp/boom_hls` path is `EPHEMERAL_BUILD_PROVENANCE`, not a required-at-commit
durable artifact. The durable policy retains source hashes, selected RTL hash,
generation/test provenance, and result matrices without requiring that build
workspace to exist. `ephemeral_build_provenance.csv` retains the generated RTL
hash and original path with `required_at_commit=false`.
`durable_artifact_hashes.csv` contains only repository paths and records both
SHA-256 and byte size.

`PF5_FINAL_DIRECTED_EXPECTED_CHECKS=6205`

`PF5_DIRECTED_COUNT_PROVENANCE_VERIFIED=true`

`PF2_FULL_CORE_RTL_STATUS=PASS_11_OF_11_CURRENT_SOURCE`

`W3_SOFTWARE_STATUS=PASS_400_OF_400`

`W3_FOCUSED_RTL_STATUS=PASS_11_OF_11`

`PF5_EPHEMERAL_RTL_CLASSIFICATION_FIXED=true`

`PF5_TMP_REQUIRED_DURABLE_ARTIFACT_COUNT=0`

The durable artifact count is owned by the subsequent PF5CW normalized
manifest and is not frozen at the pre-normalization value of 12.

`PF5CR_PRODUCT_SOURCE_CHANGED=false`
