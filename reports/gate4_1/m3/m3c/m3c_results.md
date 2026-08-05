# Gate 4.1 M3C Results

`GATE4_1_RV64M_VERIFIED=true`

## Acceptance Summary

1. Joint RV64M directed testing: 1458 checks, 0 failures.
2. Joint persistent random testing: 256 fixed seeds x 2048 cycles; all 13 operations accepted and completed with zero arithmetic mismatch, protocol mismatch, drop, duplicate, stale side effect, or starvation violation.
3. Native and Vitis csim full-core programs: 15/15 each.
4. Focused generated RTL: M3C 30/30, preserved M3B 26/26, preserved M2 10/10 plus 2/2 full-core.
5. Generated `boom_core_top` full-core RV64M programs: 15/15.
6. Gate 3.9 reset/backpressure matrix: 49/49.
7. W3 preservation: 400/400 software and 11/11 focused RTL.
8. W4 preservation: 95/95 directed, 128/128 persistent random, and 20/20 focused RTL.
9. Trace and architecture preservation: C++/csim 7/7, normalized checks 21/21, full-program diff 10/10, and partial order 7/7.
10. Canonical synthesis: 8/8. `synth_core_step_top` is 116249 LUT, 26408 FF, 16 BRAM, 3 DSP, 6.341 ns; `boom_core_top` is 123520 LUT, 26815 FF, 16 BRAM, 3 DSP, 6.341 ns.
11. PPA: `M3B_PPA_BLOCKER=false`; every canonical target has at most 3 DSP and 16 BRAM.
12. Topology remains 3 completion sources, 2 integer PRF writes, 3 wakeups, 3 bypasses, and 3 ROB completes.
13. `CORE_CYCLE` remains unpipelined and all canonical XML reports state `PipelineType=no`.
14. Independent read-only review: PASS with no acceptance blocker.

## Guardrails

`src/boom_all.cpp` is excluded. Raw `boom_core_step` is not used as a product synthesis top. Frozen expected artifacts are unchanged. No `CORE_CYCLE` pipeline, `DATAFLOW`, false-dependence, complete-array partition, core structural optimization, Frontend implementation, or Full LSU work was introduced.

`READY_FOR_FRONTEND_IMPLEMENTATION=false`

`READY_FOR_FULL_LSU_IMPLEMENTATION=false`

`READY_FOR_OFFICIAL_GATE_3=false`

`M009=PARTIALLY_VERIFIED`

`M014=VERIFIED`
