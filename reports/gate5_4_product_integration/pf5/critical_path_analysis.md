# PF5 Critical Path Analysis

- Target: 10.00 ns.
- Required gate: `boom_core_top <= 6.5 ns`.
- PF4 `boom_core_top`: 6.341 ns.
- PF5 `boom_core_top`: 6.341 ns, PASS.
- `synth_rob_top`: 6.548 ns, diagnostic only.
- `synth_predictor_foundation_top`: 2.989 ns.
- The commit-to-predictor transfer is registered by
  `predictor_update_pending`; FTQ reclaim follows snapshot generation.

Values are Vitis HLS estimates and do not represent post-route STA.
