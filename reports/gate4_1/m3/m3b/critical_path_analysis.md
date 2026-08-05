# M3B Critical Path Analysis

All eight canonical Vitis HLS 2021.2 synthesis targets completed with `PipelineType=no` at a 10 ns target and 2.7 ns uncertainty.

- `synth_divider_top`: 5.734 ns. The longest state is the 5.73 ns `divider_accept` call. The restoring iteration state is 5.23 ns and consists of input-bit select, 64-bit compare, conditional subtract, sign/result selects, and state write.
- `synth_execute_top`: 6.411 ns. This is the largest M3B module estimate and is below 6.5 ns. The integrated Divider raises the M2C execute estimate by 0.981 ns.
- `synth_core_step_top`: 6.341 ns, unchanged from M2C.
- `boom_core_top`: 6.341 ns, unchanged from M2C. Its report attributes the scheduled product call to `boom_core_cycle_io`; the Divider does not replace the accepted full-core critical path.

The product period is below 6.5 ns, so `M3B_PPA_BLOCKER=false`. No M3C directive, `CORE_CYCLE` pipeline, DATAFLOW, false-dependence, or complete-array-partition experiment was applied.
