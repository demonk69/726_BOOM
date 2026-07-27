# D4_LOCAL_KILL_BITMAP Csynth Summary

| Module | Status | Period ns | LUT | FF | BRAM_18K | DSP | Runtime s |
|---|---|---:|---:|---:|---:|---:|---:|
| synth_free_list_rollback_top | PASS | 3.272 | 12725 | 6685 | 1 | 0 | 24.29 |
| synth_core_step_top | PASS | 5.898 | 45889 | 12541 | 12 | 3 | 59.22 |
| boom_core_top | PASS | 5.898 | 82789 | 17041 | 16 | 3 | 70.72 |

Verdict: `CANDIDATE_NOT_ACCEPTED_BELOW_10_PERCENT`. Full-core LUT decreased by 497 versus the accepted 83286 LUT baseline, but this is only a 0.60% reduction and does not meet the 10% Gate 3.5 acceptance threshold. FF increased by 430.

The result permits the independent D4-IQ compaction experiment, but it does not replace the Gate 3.3 accepted PPA configuration.
