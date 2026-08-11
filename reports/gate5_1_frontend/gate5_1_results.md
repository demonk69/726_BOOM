# Gate 5.1 Final Results

Gate 5.1 does not pass final acceptance.

1. Focused RTL: 20 observable checks PASS, 13 mandatory checks BLOCKED; not verified.
2. Triple response match: PASS for fetch ID, 32-bit epoch, and expected address, including six mismatch combinations.
3. Redirect over response and priority: BLOCKED by focused top interface.
4. Architectural ownership: BLOCKED by focused top interface.
5. Stale drain: PASS for exposed unstalled stale responses; Decode-stall drain is BLOCKED.
6. Holding/backpressure: request backpressure PASS; held Decode instruction/fault stability is BLOCKED.
7. Fault propagation and misaligned targets: native PASS, focused RTL BLOCKED.
8. Runtime reset: focused RTL BLOCKED; `ap_rst` is control-only for algorithmic static state.
9. Steady request and instruction interval: 6 `ap_clk` cycles in the 32-instruction always-ready run.
10. HLS interval 3: top transaction initiation interval, not request interval; two transactions per instruction produce interval 6.
11. Throughput blocker: true.
12. Full-core regression: M3C directed/random/native 15/15 PASS, but W3 is 399/400 and remaining current-source suites are not accepted/reached.
13. Full-core RTL: not accepted; W4 focused rerun timed out and later suites were not run after mandatory failures.
14. Canonical csynth: 9/9 PASS.
15. `boom_core_top`: 124079 LUT, 27513 FF, 16 BRAM, 3 DSP, 6.341 ns.
16. `synth_core_step_top`: 116597 LUT, 27106 FF, 16 BRAM, 3 DSP, 6.341 ns.
17. Critical path: `execute_module`; Frontend is 5.569 ns and is not critical.
18. `GATE5_1_PPA_BLOCKER=false`.
19. `GATE5_1_FRONTEND_FOUNDATION_VERIFIED=false`.
20. `READY_FOR_GATE5_2_RVC=false`.
21. `READY_FOR_FRONTEND_IMPLEMENTATION=false`.
22. `READY_FOR_FULL_LSU_IMPLEMENTATION=false`.
23. `M009=PARTIALLY_VERIFIED`.
24. `M014=VERIFIED` based on the accepted Gate 3.9 baseline; no new contradictory reset result is claimed.
25. `READY_FOR_OFFICIAL_GATE_3=false`.

Gate 5.2 RVC was not started. The minimum next action is a verification-capable generated focused top or equivalent canonical observability that does not alter architectural behavior, followed by a minimal legal scheduling fix for the six-clock throughput and diagnosis of the W3 ROB-full regression.
