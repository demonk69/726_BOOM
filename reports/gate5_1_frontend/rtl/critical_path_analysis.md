# Gate 5.1 Critical Path Analysis

All nine canonical Vitis HLS 2021.2 targets synthesized. `boom_core_top` and `synth_core_step_top` remain at 6.341 ns, below the 6.5 ns threshold. Their longest reported child schedule is `execute_module` at 6.341 ns. The standalone execute wrapper is 6.411 ns. Frontend is 5.569 ns and is not the full-core critical path.

Resources remain 16 BRAM and 3 DSP for both full-core targets. Divider remains iterative; no combinational DIV/REM was introduced. The shared multiply mapping remains three DSP. Every canonical top reports `PipelineType=no`; `CORE_CYCLE` was not enabled.

`GATE5_1_PPA_BLOCKER=false`.

These are HLS estimates, not post-route STA.
