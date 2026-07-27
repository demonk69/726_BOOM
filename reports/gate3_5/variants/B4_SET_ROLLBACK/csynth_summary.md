# B4_SET_ROLLBACK Csynth Summary

| Module | Status | Period ns | LUT | FF | BRAM_18K | DSP | Runtime s |
|---|---|---:|---:|---:|---:|---:|---:|
| synth_free_list_rollback_top | PASS | 2.625 | 13368 | 6644 | 1 | 0 | 24.71 |
| synth_core_step_top | PASS | 5.898 | 47060 | 12443 | 12 | 3 | 59.30 |
| boom_core_top | PASS | 5.898 | 84984 | 16943 | 16 | 3 | 69.91 |

Verdict: `REJECTED_PPA`. Full-core LUT increased by 1698 versus the accepted 83286 LUT baseline.

The once-built release set removed duplicate free-list/map scans in C++ structure, but Vitis synthesized the local set construction as additional logic rather than reducing the recovery cone.
