# L1 Artifact Manifest

Durable accepted evidence consists of canonical source, parameterized tests/runners, current-source PF3/PF5/PF6 matrices and traces, W4 logs, current-compatible RVC native/CSim/RTL evidence, and files in this directory. L1R2B focused RTL evidence includes the exact ordered 80-row acceptance CSV, the 10-row PATH_B stimulus/oracle CSV, per-case XSim logs, the PATH_B-only testbench/runner, bounded-retry resource logs, canonical-flow audit, and the generated RTL aggregate manifest.

Focused command-engine failure logs are retained under `focused_resume/`, `focused_resume_sliced/`, and `focused_resume_grouped/`. The exact compact input failed with exit 137 after creating a 305G `.autopilot` workspace; two narrower pilots were stopped by a 20GB watchdog. These are blocker provenance, not accepted RTL evidence.

`l1_behavior_equivalence.md`, `focused_rtl_matrix.csv`, and
`l1r_resume_outcome.md` are historical `17/80` checkpoint records. They retain
the validation-architecture blocker provenance and do not state the current
gate result. `l1_results.md` and
`focused_rtl_redesign/canonical_focused_rtl_acceptance.csv` are authoritative
for the final `80/80` status.

The successful one-time PATH_B retry generated 122 Verilog/data files totaling
18,049,415 bytes with aggregate hash
`c0165ccea43ccc41d1d2cc5514d8af17764b700b84a84af751c4b331be55fcbe`.
The generated project was ephemeral and is removed only after the durable
manifest, semantic logs, and hashes are recorded.
