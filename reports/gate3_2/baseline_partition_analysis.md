# Gate 3.2 Baseline Partition Analysis

Input log: `reports/gate3_2/baseline_csynth.log`

Automatic partition records: 0
Records caused by `next_state` temporaries: 0
Branch snapshot partition records: 0
Risk counts: HIGH=0, MEDIUM=0, LOW=0

Last completed HLS pass:

| Pass | Elapsed | Memory |
|---|---:|---:|
| Command csynth_design CPU user time | 43.63 seconds | -491.785 MB. |

Original timeout root cause:

The post-LSU csynth timeout is dominated by Vitis HLS transformation/auto-partition expansion of the full `BoomCoreState next_state = state` copy in `boom_core_step.cpp`. The copy turns every persistent state array into a field-level `next_state.*` temporary. With the existing top-level loop pipeline, HLS then tries to flatten/partition these temporaries, including branch snapshots, ROB/IQ-related fields, and lane arrays. The earlier hardcoded `#pragma HLS PIPELINE II=1` on `CORE_CYCLE` made this behavior part of the nominal baseline rather than a performance-only experiment.

Required fix direction:

- Remove hardcoded baseline core-loop pipelining.
- Stop copying the whole `BoomCoreState` every cycle.
- Keep large persistent arrays as module state with explicit write updates.
- Keep branch snapshots allocated for future BOOM equivalence, but do not expose them as per-cycle copy temporaries.

See `reports/gate3_2/automatic_partition_inventory_after.csv` for the after-refactor inventory.
