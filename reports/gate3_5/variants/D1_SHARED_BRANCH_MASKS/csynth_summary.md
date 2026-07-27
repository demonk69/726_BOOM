# D1_SHARED_BRANCH_MASKS Csynth Summary

| Module | Status | Period ns | LUT | FF | BRAM_18K | DSP | Runtime s |
|---|---|---:|---:|---:|---:|---:|---:|
| synth_free_list_rollback_top | PASS | 2.203 | 11903 | 6295 | 1 | 0 | 24.14 |
| synth_core_step_top | PASS | 5.898 | 45365 | 12119 | 12 | 3 | 59.10 |
| boom_core_top | PASS | 5.898 | 83301 | 16619 | 16 | 3 | 69.00 |

Verdict: `REJECTED_PPA`. Full-core LUT increased by 15 versus the accepted 83286 LUT baseline.

The shared mask bundle preserved behavior and reduced one warning, but it did not produce a meaningful area reduction in the generated branch helper hierarchy.
