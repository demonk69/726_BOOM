# C1_RECOVERY_ENABLE Csynth Summary

| Module | Status | Period ns | LUT | FF | BRAM_18K | DSP | Runtime s |
|---|---|---:|---:|---:|---:|---:|---:|
| synth_free_list_rollback_top | PASS | 2.203 | 11895 | 6295 | 1 | 0 | 23.98 |
| synth_core_step_top | PASS | 5.898 | 45968 | 12205 | 12 | 3 | 58.63 |
| boom_core_top | PASS | 5.898 | 83903 | 16705 | 16 | 3 | 69.71 |

Verdict: `REJECTED_PPA`. Full-core LUT increased by 617 versus the accepted 83286 LUT baseline.

The explicit recovery-enable boundary preserved traces but did not reduce duplicated logic in Vitis HLS 2021.2; the generated top-level cone grew while period, BRAM, and DSP stayed unchanged.
