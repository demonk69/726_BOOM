# Gate 3.7 Baseline Manifest

Date: 2026-07-28

Baseline commit: `94b4ce12a0c1673f0eeb035d2ba9edef5f0034e9 Gate 3.6: explain top-level resource delta`.

Accepted PPA configuration: Gate 3.3 conservative `boom_core_top`, preserved through Gate 3.6.

| Metric | Value |
|---|---:|
| LUT | 83286 |
| FF | 16611 |
| BRAM_18K | 16 |
| DSP | 3 |
| Estimated period | 5.898 ns |
| `CORE_CYCLE` pipeline | disabled |
| Whole-state reset | required and retained |

Frozen evidence:

| Artifact | Path |
|---|---|
| Git status | `reports/gate3_7/git_status_before.txt` |
| Baseline commit | `reports/gate3_7/git_commit_before.txt` |
| Source hashes | `reports/gate3_7/source_hashes_before.txt` |
| Gate 3.6 source hashes | `reports/gate3_7/baseline_artifacts/gate3_6_source_hashes_after.txt` |
| Accepted csynth report | `reports/gate3_7/baseline_artifacts/boom_core_top_csynth.rpt` |
| Accepted HLS traces | `reports/gate3_7/baseline_artifacts/accepted_hls_traces/` |
| BOOM reference traces | `reports/gate3_7/baseline_artifacts/boom_reference_traces/` |
| Full-program diff | `reports/gate3_7/baseline_artifacts/full_program_architectural_diff.md` |
| Partial-order result | `reports/gate3_7/baseline_artifacts/partial_order.log` |
| Regression summary | `reports/gate3_7/regression_before.md` |

Pre-existing unstaged generated artifacts are recorded but are not part of the accepted source baseline. Gate 3.7 does not modify or clean them.
