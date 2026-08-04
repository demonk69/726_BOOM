# Gate 4.1 M1 Critical Path Check

Status: **PASS**.

| Top | W4 period | M1 period | Delta | Pipeline |
|---|---:|---:|---:|---|
| synth_decode_top | n/a | 1.526 ns | n/a | no |
| synth_core_step_top | 6.025 ns | 6.025 ns | 0.000 ns | no |
| boom_core_top | 6.025 ns | 6.025 ns | 0.000 ns | no |

The decode table increases product area but does not change the conservative estimated period. `boom_core_top_csynth.rpt` reports `CORE_CYCLE` with `Pipelined=no`, no achieved II, and an infinite trip count. No pipeline, DATAFLOW, false-dependence waiver, or complete array partition was added.
