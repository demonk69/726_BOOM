# Gate 3.5 Baseline Manifest

Date: 2026-07-26

Baseline commit: `d9ee017 Gate 3.4: attribute branch recovery resource cost`.

Accepted PPA configuration remains Gate 3.3 conservative `boom_core_top` with Gate 3.4 attribution-only framework:

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
| Git status before variants | `reports/gate3_5/git_status_before.txt` |
| Source hashes before variants | `reports/gate3_5/source_hashes_before.txt` |
| Worktree diff before variants | `reports/gate3_5/worktree_diff_before.patch` |
| Baseline resources | `reports/gate3_5/baseline_resources.csv` |
| Baseline regression | `reports/gate3_5/baseline_regression.md` |
| Gate 3.4 attribution | `reports/gate3_5/baseline_artifacts/resource_attribution.md` |
| Gate 3.4 module baseline | `reports/gate3_5/baseline_artifacts/module_baseline.csv` |
| Accepted HLS traces | `reports/gate3_5/baseline_artifacts/baseline_traces/` |

The original Vitis project directories containing raw `*_csynth.rpt` files are not present in this workspace. Gate 3.5 therefore freezes the parsed Gate 3.4 csynth CSVs and logs already committed under `reports/gate3_4` as the accepted PPA evidence.

Existing unstaged generated artifacts are intentionally not modified by the baseline freeze.
