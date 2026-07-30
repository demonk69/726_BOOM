# Clock Target Analysis

All eight P0/R1 requested target points complete csynth. Requested period is kept separate from HLS estimated period; no post-route achieved clock is available.

The smallest requested target, 4.5 ns, produces a 3.255 ns top/LSU estimate and a 3.230 ns execute estimate. P0 resources are 48041 LUT, 12945 FF, 12 BRAM, and 3 DSP. Seven normal XSim programs pass architecturally, but event cycles relative to first fetch are 0/7 exact versus Gate 3.9. The 4.5 ns point is therefore `SYNTHESIS_CHARACTERIZATION_REJECTED_NORMAL_CYCLE_CHANGE`, not an accepted clock configuration.

No clock sweep point is described as a physical achieved frequency because implementation STA was not run.
