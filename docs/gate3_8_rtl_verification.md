# Gate 3.8 Generated RTL Verification

Gate 3.8 verifies the accepted conservative `boom_core_top` at generated-RTL pins with XSim. It adds no processor feature, capacity change, pipeline, or accepted PPA optimization.

## Result

Status: `RTL_RESET_MISMATCH`.

The serial 45-scenario matrix passes 42 cases. All tested commit-trace, IMEM, and DMEM AXIS backpressure behavior passes, as do seven normal programs. Three required reset interactions fail:

- `R2_RESET_ROB_NONEMPTY`: no commit after reset; timeout at 40000 cycles.
- `R6_RESET_BRANCH_RECOVERY`: first post-reset fetch is `0x80000000`, not `0x10040`.
- `P0_RESET_AND_BRANCH_MISPREDICT`: same first-fetch mismatch.

## Reset Finding

The generated implementation resets control FSMs and transport occupancy but leaves most processor state at its pre-reset value. The frontend PC, ROB, IQ, issued state, rename maps/free/busy state, branch snapshots/allocation lists, RF, execute results, and LSU queue/bookkeeping storage are `RESET_INITIAL_ONLY`. Generated RAM modules accept a reset input but do not use it in their sequential storage logic.

This means runtime `ap_rst_n` is a partial control restart, not a coherent processor reset. Passing valid-based cases cannot close the requirement because R2, R6, and P0 are concrete counterexamples.

## Passing Evidence

- XSim build and pin inventory cover `ap_rst_n` plus three AXIS channels.
- Commit trace backpressure: 6/6 PASS.
- IMEM request/response, delay, stale response, and reset interaction: 7/7 PASS.
- DMEM request/response, delay, stale response, flush, reset, and `tohost`: 9/9 PASS.
- Normal generated-RTL programs: 7/7 PASS.
- C++ versus Vitis csim versus RTL architectural traces: 7/7 PASS.
- Conservative synthesis is unchanged at 83286 LUT, 16611 FF, 16 BRAM_18K, 3 DSP, and 5.898 ns.

## Decision

`READY_FOR_LOCAL_PIPELINE_OPTIMIZATION=false`, `READY_FOR_WIDE_ISSUE_IMPLEMENTATION=false`, and `READY_FOR_OFFICIAL_GATE_3=false`.

Primary evidence is in `reports/gate3_8/gate3_8_results.md`, `reports/gate3_8/rtl_test_matrix.csv`, `reports/gate3_8/rtl_reset_state_inventory.csv`, and `reports/gate3_8/reset_semantics_analysis.md`.
