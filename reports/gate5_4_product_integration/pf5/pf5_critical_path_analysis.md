# PF5 Critical Path Analysis

- Target clock: 10.000 ns.
- Gate limit for `boom_core_top`: 6.500 ns.
- PF4 baseline: 6.341 ns.
- PF5 current: 6.341 ns, PASS with zero period delta.
- `synth_rob_top` is 6.548 ns and is diagnostic, not the product gate top.
- Commit writes a one-entry pending handoff; predictor mutation remains in the
  canonical Frontend predictor step and is not a direct Commit combinational
  write path.

These are Vitis HLS estimates, not post-route timing results.
