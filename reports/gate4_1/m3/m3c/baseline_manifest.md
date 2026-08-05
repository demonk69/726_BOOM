# Gate 4.1 M3C Baseline Manifest

- Task-start Git HEAD: `ad20cf54734f2c8409df20ab0d25405a4985635e`
- Branch: `gate3.8-rtl-verification`, three commits ahead of its upstream at task start.
- Tool: Vitis HLS 2021.2, part `xczu7ev-ffvc1156-2-e`, target clock 10 ns.
- `M1_RV64M_DECODE_VERIFIED=true`
- `M2_MUL_FAMILY_VERIFIED=true`
- `M3A_STANDALONE_DIVIDER_VERIFIED=true`
- `M3B_INT_DIVIDER_INTEGRATION_VERIFIED=true`
- `READY_FOR_M3C_RV64M_FINAL=true`
- `GATE4_1_RV64M_VERIFIED=false`
- `M3B_PPA_BLOCKER=false`
- Canonical core cycle: `Pipelined=no`.
- M1 evidence: `reports/gate4_1/m1/`.
- M2A/M2B/M2C evidence: `reports/gate4_1/m2/`.
- M3A evidence: `reports/gate4_1/m3/m3a/`.
- M3B evidence: `reports/gate4_1/m3/m3b/`.
- M3B generated RTL: `reports/gate4_1/m3/m3b/full_core_rtl/` and `reports/gate4_1/m3/m3b/rtl/`.
- M3B full-core traces: `reports/gate4_1/m3/m3b/full_core_rtl/traces/`.
- M3B csynth reports: `reports/gate4_1/m3/m3b/csynth/`.
- Frozen W4 topology: 3 completion sources, 2 integer PRF writes, 3 wakeups, 3 bypasses, and 3 ROB completes.

All earlier evidence trees are read-only inputs to M3C and must not be overwritten.
