# Gate 5.2 R2 Critical Path Analysis

The final canonical `boom_core_top` XML reports an estimated period of **6.341 ns**, not an assumed or copied value. Its generated child `execute_module` also reports 6.341 ns; the full-core `frontend_module` reports 5.190 ns. The standalone `synth_execute_top` is 6.411 ns and standalone `synth_frontend_top` is 4.379 ns. RVC fetch therefore does not replace Execute as the product critical path.

This is Vitis HLS 2021.2 scheduling evidence for `xczu7ev-ffvc1156-2-e`, target 10.00 ns with 2.70 ns uncertainty, not post-route STA. Full-core timing is unchanged from the frozen Gate 5.1 baseline at 6.341 ns. Full-core LUT rises 2,481 (1.996%) and FF rises 979 (3.558%); BRAM and DSP remain 16 and 3. The R2 frontend cost is explicit in `stage_resource_delta.csv`, but the target period closes and no threshold failure is evidenced: `GATE5_2_R2_PPA_BLOCKER=false`.

`CORE_CYCLE` and every requested canonical top report `PipelineType=no`. No DATAFLOW, false-dependence directive, or complete-array partition is accepted or used. Exact XML paths and all ten top-level values are in `resource_summary.csv`.
