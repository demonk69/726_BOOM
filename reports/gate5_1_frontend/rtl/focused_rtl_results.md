# Gate 5.1 Focused RTL Results

The canonical generated `synth_frontend_top` was compiled and simulated with XSim 2021.2. The observable subset passes 20 checks: initial request packing, all requested ID/epoch/address mismatch combinations, exact triple match, duplicate/no-owner drain, delayed response uniqueness, request backpressure, 32 sequential `PC+4` responses, monotonic IDs, and control-reset characterization.

The matrix contains 33 checks: 20 PASS and 13 BLOCKED. Redirect priority/redirect-over-response, architectural ownership, redirect epoch, Decode hold/stale drain under Decode stall, fault propagation/hold, target misalignment, and runtime reset cannot be driven or observed through this top. `ap_rst` resets generated control/FIFOs but does not perform the architectural runtime Frontend reset.

The focused top also optimizes away instruction, exception, cause, held-entry, ownership, and redirect state not affecting PC. Passing its exposed projection cannot prove the complete Gate 5.1 contract.

- `GATE5_1_FOCUSED_RTL_VERIFIED=false`
- `GATE5_1_THROUGHPUT_BLOCKER=true`
- `GATE5_1_PPA_BLOCKER=false`

Evidence: `rtl_test_matrix.csv`, `request_cycle_trace.csv`, `logs/xsim.log`, and `throughput_analysis.md`.
