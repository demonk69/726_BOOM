# Gate 3.2 Csynth Timeout Analysis

Input log: `reports/gate3_2/logs/hls_csynth_before.log`

Automatic partition records: 427
Records caused by `next_state` temporaries: 395
Branch snapshot partition records: 65
Risk counts: HIGH=94, MEDIUM=333, LOW=0

Last completed HLS pass before timeout:

| Pass | Elapsed | Memory |
|---|---:|---:|
| Checking Synthesizability | 19.87 seconds | 703.633 MB. |

Root cause:

The post-LSU csynth timeout is dominated by Vitis HLS transformation/auto-partition expansion of the full `BoomCoreState next_state = state` copy in `boom_core_step.cpp`. The copy turns every persistent state array into a field-level `next_state.*` temporary. With the existing top-level loop pipeline, HLS then tries to flatten/partition these temporaries, including branch snapshots, ROB/IQ-related fields, and lane arrays. The earlier hardcoded `#pragma HLS PIPELINE II=1` on `CORE_CYCLE` made this behavior part of the nominal baseline rather than a performance-only experiment.

Required fix direction:

- Remove hardcoded baseline core-loop pipelining.
- Stop copying the whole `BoomCoreState` every cycle.
- Keep large persistent arrays as module state with explicit write updates.
- Keep branch snapshots allocated for future BOOM equivalence, but do not expose them as per-cycle copy temporaries.

See `reports/gate3_2/automatic_partition_inventory.csv` for the full inventory.

## After Gate 3.2 Refactor

| Check | Result |
|---|---:|
| `BoomCoreState next_state = state` in `boom_core_step` | Removed |
| Hardcoded `#pragma HLS PIPELINE II=1` in baseline top | Removed; now gated by `BOOM_HLS_ENABLE_CORE_PIPELINE` |
| Baseline `boom_core_top` csynth | PASS |
| Baseline runtime | 45.56 seconds |
| Baseline peak memory | 1521072 KB |
| Baseline automatic partition records parsed from log | 0 |
| Baseline report | `boom_hls_gate3_2_baseline/solution_baseline/syn/report/boom_core_top_csynth.rpt` |

The finite `boom_core_step_top` also synthesizes successfully. The separate performance-mode top with `BOOM_HLS_ENABLE_CORE_PIPELINE=1` still timed out after 15 minutes in HLS transformations, so pipelining remains a later PPA/debug task and is not part of the accepted baseline.

After-refactor partition inventory: `reports/gate3_2/automatic_partition_inventory_after.csv`.

Top-mode summary: `reports/gate3_2/top_csynth_modes.csv`.
