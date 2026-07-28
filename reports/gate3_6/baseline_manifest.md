# Gate 3.6 Baseline Manifest

Date: 2026-07-27

Baseline commit: `326c96f Gate 3.5: complete branch recovery PPA experiments`.

Accepted PPA configuration: Gate 3.3 conservative `boom_core_top`.

| Metric | Value |
|---|---:|
| LUT | 83286 |
| FF | 16611 |
| BRAM_18K | 16 |
| DSP | 3 |
| Estimated period | 5.898 ns |
| CORE_CYCLE pipeline | disabled |

Frozen evidence:

| Artifact | Path |
|---|---|
| Git status before Gate 3.6 | `reports/gate3_6/git_status_before.txt` |
| Baseline commit | `reports/gate3_6/git_commit_before.txt` |
| Source hashes | `reports/gate3_6/source_hashes_before.txt` |
| Pre-existing source diff | `reports/gate3_6/worktree_diff_before.patch` |
| Gate 3.4 module baseline | `reports/gate3_6/baseline_artifacts/gate3_4_module_baseline.csv` |
| Gate 3.5 variant summary | `reports/gate3_6/baseline_artifacts/gate3_5_variant_summary.csv` |
| Raw direct-step csynth report | `reports/gate3_6/baseline_artifacts/synth_core_step_top_csynth.rpt` |
| Raw product-top csynth report | `reports/gate3_6/baseline_artifacts/boom_core_top_csynth.rpt` |
| Raw cycle-wrapper csynth report | `reports/gate3_6/baseline_artifacts/boom_core_cycle_io_csynth.rpt` |
| Raw finite wrapper csynth report | `reports/gate3_6/baseline_artifacts/boom_core_step_top_csynth.rpt` |
| Accepted HLS traces | `reports/gate3_6/baseline_artifacts/accepted_hls_traces/` |
| BOOM reference traces | `reports/gate3_6/baseline_artifacts/boom_reference_traces/` |
| Baseline regression summary | `reports/gate3_6/regression_before.md` |

The existing unstaged generated files listed in `git_status_before.txt` are not part of the accepted source baseline and are not modified by Gate 3.6 source experiments.
