# Gate 4.1 M2A Baseline Manifest

- Frozen commit: `e8c1447a2cb23bb7a1212c55538f15003d21a076`
- Branch: `gate3.8-rtl-verification`
- Local and remote HEAD matched at freeze.
- M1 status: `M1_RV64M_DECODE_VERIFIED`.
- M2 readiness: `READY_FOR_M2_MUL_FAMILY=true`.
- Tool: Vitis HLS 2021.2.
- `CORE_CYCLE`: `Pipelined=no`.

The M1 source hashes, resource rows, and frozen trace manifest are copied into this directory. M2A does not overwrite M0 or M1 evidence.

## Pre-M2A Execute Gap

At the frozen commit, `src/execute.cpp:73` handles only uopc 16 and performs signed 32x32 multiplication. Uopcs 17-20 have no multiply arithmetic implementation. M2A records this defect but must not modify `src/execute.cpp`.

## Dirty Workspace Exclusions

The complete task-start status is recorded in `git_status_before.txt`. Pre-existing tracked logs, `src/boom_all.cpp`, `vitis_hls.log`, `build/`, HLS/XSim projects, and timestamped backup logs are excluded from M2A evidence and future staging.
