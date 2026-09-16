# Artifact Manifest

- Durable summaries and matrices: this directory.
- Byte-exact raw baseline, A1, and accepted A5 schedule, bind, csynth report, and csynth XML: `raw_evidence/`.
- Raw hashes and sizes: `durable_artifact_hashes.csv`.
- Baseline runner logs: `baseline_runner/`.
- Candidate logs: `A/`, `A2/`, `A3/`, `A4/`, `B/`, and `C/`.
- Accepted A5 canonical timing/PPA and final preservation: `A5/canonical_full_core/` and `A5/final_preservation/`.
- `A5/full_core/` is a noncanonical diagnostic run without FTQ/predictor LUTRAM CFLAGS; it is excluded from acceptance. `A5/canonical_full_core/` is the acceptance source.
- Ephemeral HLS projects: `/home/lab_726/opencode_tmp/g6_t0r_baseline/` and `/home/lab_726/opencode_tmp/g6_t0r/`.
- Ephemeral projects are rebuildable from the recorded tool, part, clock, flags, source hashes, and tops. Final conclusions do not depend on those paths because required raw timing evidence is repository-resident.
