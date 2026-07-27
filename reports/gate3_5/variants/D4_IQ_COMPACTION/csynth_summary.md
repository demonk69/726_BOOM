# D4_IQ_COMPACTION Csynth Summary

| Module | Status | Period ns | LUT | FF | BRAM_18K | DSP | Runtime s |
|---|---|---:|---:|---:|---:|---:|---:|
| synth_free_list_rollback_top | PASS | 2.203 | 11707 | 6284 | 1 | 0 | 23.85 |
| synth_core_step_top | PASS | 5.898 | 52835 | 14086 | 12 | 3 | 62.27 |
| boom_core_top | PASS | 5.898 | 90802 | 18590 | 16 | 3 | 73.40 |

Verdict: `REJECTED_PPA`. Full-core LUT increased by 7516 versus the accepted 83286 LUT baseline.

The survivor-based IQ compaction passed the dedicated 10-test IQ suite and produced a small reduction in `synth_free_list_rollback_top`, but it substantially increased `synth_core_step_top` and `boom_core_top` resources.
