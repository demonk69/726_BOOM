# Gate 3.9 Fine-Grain Reset

Gate 3.9 replaces the generated RTL's incomplete whole-state reset with a deterministic fine-grain initialization controller. Only the controller carries an HLS reset pragma; it reconstructs all reset-visible core state before normal execution is enabled.

## Result

Status: `RTL_RESET_VERIFIED`.

- Generated RTL: 49/49 XSim scenarios pass.
- Reset progress: 60 completed releases fetch again from `0x10040` and later commit; six releases are intentionally interrupted by scenario reset assertion and zero fail.
- Gate 3.8 failures `R2`, `R6`, and `P0`: all pass.
- New reset reentry/stress cases `R8` through `R11`: 4/4 pass.
- C++ reset architecture tests: 14/14 pass.
- Normal RTL architectural traces: 7/7 match the frozen baseline.
- Frozen C++/csim traces: 10/10 byte-identical.
- Conservative synthesis: 47999 LUT, 12134 FF, 12 BRAM_18K, 3 DSP, 5.898 ns.
- `CORE_CYCLE` pipeline: disabled.

Generated-RTL absolute commit cycles changed after reset initialization and resynthesis, so this result makes no strict cycle-equivalence claim. M014 is locally verified, while official Gate 3 remains blocked by the missing Chipyard/FESVR/DRAMSim environment and broader model scope.

Primary evidence is in `reports/gate3_9/gate3_9_results.md`, `reports/gate3_9/rtl_test_matrix.csv`, `reports/gate3_9/reset_latency.csv`, and `reports/gate3_9/normal_rtl_trace_comparison.csv`.
