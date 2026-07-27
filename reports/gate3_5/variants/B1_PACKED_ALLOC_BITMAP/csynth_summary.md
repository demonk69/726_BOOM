# B1_PACKED_ALLOC_BITMAP Csynth Summary

| Module | Status | Period ns | LUT | FF | BRAM_18K | DSP | Runtime s |
|---|---|---:|---:|---:|---:|---:|---:|
| synth_free_list_rollback_top | PASS | 2.203 | 12155 | 6475 | 1 | 0 | 30.07 |
| synth_core_step_top | PASS | 5.898 | 45473 | 12254 | 12 | 3 | 57.56 |
| boom_core_top | PASS | 5.898 | 83408 | 16754 | 16 | 3 | 67.31 |

Verdict: `REJECTED_PPA`. Full-core LUT increased by 122 versus the accepted 83286 LUT baseline.

RTL storage observation: `branch_state.br_alloc_bitmap` was inferred as `RAM_AUTO_1R1W` with `DataWidth=52`, `AddressRange=8`, not flattened registers. This changed the storage shape from 416x1 to 8x52 but did not reduce the full-core recovery/control cone.
