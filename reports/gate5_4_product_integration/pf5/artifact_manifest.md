# PF5 Artifact Manifest

Durable evidence is stored under this directory. Functional matrices and logs
are source-bound by `source_hashes_after.txt`; canonical synthesis is recorded
in `../../gate5_4_pf5_canonical_final/module_csynth_summary.csv`.

- `directed_test_matrix.csv`: directed and preservation assertion totals.
- `small_state_exhaustive.csv`: complete reduced-state transition space.
- `random_test_summary.csv`: 256 x 8,192 reference test.
- `long_run_summary.csv`: one-million-step reference test.
- `program_matrix.csv`: twelve standalone PF5 native/CSim/RTL verdicts.
- `pf5_product_coverage.csv`: required coverage totals.
- `rtl_test_matrix.csv`: focused and full-core generated RTL verdicts. Its PF2
  and W3 current-source rows bind to the exact evidence paths recorded in
  `pf5p_preservation_provenance.md`.
- `resource_summary.csv` and `stage_resource_delta.csv`: canonical PPA and
  PF4-to-PF5 comparison.
- `preservation_matrix.csv`: complete current-source prior-gate preservation.
- `pf5p_preservation_provenance.md` and `preservation_artifact_hashes.txt`:
  accepted semantics, source/PPA freeze, and durable preservation evidence.
- `ephemeral_build_provenance.csv`: non-required generated-workspace identity
  and selected RTL hash retained as durable provenance only.
- `raw_evidence_provenance.csv`: byte-exact identity for immutable tool output;
  only entries with `required_at_commit=true` belong to the commit manifest.
- `pf5_commit_whitespace_inventory.csv` and
  `pf5cw_whitespace_policy_repair.md`: PF5CW classification and normalization
  provenance.
- `full_core_rtl/`: current-source RTL traces and matrix.
- `durable_artifact_hashes.csv`: required-at-commit native, CSim, RTL-matrix,
  csynth, preservation, and PF5CR evidence with SHA-256 and byte size.

Generated RTL workspaces under `/tmp/boom_hls` are
`EPHEMERAL_BUILD_PROVENANCE`. Their source hash, generation provenance, test
matrix, and selected RTL hash remain durable, but the generated workspace path
and RTL entity are not required at commit and are not listed in
`durable_artifact_hashes.csv`.

Normal text commit artifacts use LF endings and the text whitespace gate. Raw
tool output is never trimmed: required raw evidence uses `RAW_BYTE_EXACT_GATE`,
while non-required raw output is `EPHEMERAL_OR_EXTERNAL_RAW_EVIDENCE` and is
excluded from the commit manifest.

The durable evidence and complete preservation matrix are current. PF5 is
accepted and ready for the separately scoped PF6 full-RTL/PPA acceptance work.
