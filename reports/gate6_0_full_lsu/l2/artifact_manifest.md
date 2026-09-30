# Gate 6.0 L2 Artifact Manifest

- Product source aggregate and per-file source evidence:
  `preservation/pf6/current/source_freshness_manifest.csv`.
- Functional and preservation status: `preservation_matrix.csv`.
- Parameter synthesis/PPA matrix: `synthesis/parameter_csynth.csv`.
- BRAM object and repair audit: `l2r_bram_mapping_audit.md` and
  `l2r_bram_object_matrix.csv`.
- PF6 accepted matrix: `preservation/pf6/current/full_core_rtl_matrix.csv`.
- PF6 source provenance: `preservation/pf6/current/generation_provenance.md`.
- PF6 progress diagnosis and repair evidence: the committed Markdown and CSV
  summaries under `pf6_progress_triage/`.
- Final outcome: `l2_results.md` and `l2r_outcome.md`.
- Durable SHA-256 inventory: `durable_artifact_hashes.csv`.
- Workspace evidence: `workspace_hygiene_check.md`.

Temporary HLS/Vivado workspaces remain outside the repository under
`/home/lab_726/opencode_tmp/g6_l2/`.
Raw logs and traces referenced by acceptance summaries remain workspace-local
supporting evidence; the release commit intentionally retains only the durable
matrices, provenance, hashes, and outcome summaries.
