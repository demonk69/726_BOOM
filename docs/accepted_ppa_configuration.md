# Accepted PPA Configuration

Accepted baseline: Gate 3.3 conservative `boom_core_top`.

| Setting | Value |
|---|---|
| Tool | Vitis HLS 2021.2 |
| Top | `boom_core_top` |
| `CORE_CYCLE` pipeline | Disabled by default |
| Estimated period | 5.898 ns |
| LUT | 83286 |
| FF | 16611 |
| BRAM_18K | 16 |
| DSP | 3 |

Gate 3.5 did not accept a new PPA optimization. Six single-variable branch recovery structure experiments completed; only `D4_LOCAL_KILL_BITMAP` reduced LUT, and it reduced full-core LUT by only 0.60%, below the 10% acceptance threshold. Gate 3.3 remains the accepted configuration.

| Gate 3.5 Best Observed Variant | LUT | FF | BRAM_18K | DSP | Period | Status |
|---|---:|---:|---:|---:|---:|---|
| `D4_LOCAL_KILL_BITMAP` | 82789 | 17041 | 16 | 3 | 5.898 ns | `CANDIDATE_NOT_ACCEPTED_BELOW_10_PERCENT` |

Gate 3.6 explains the prior 37936-LUT direct-diagnostic/product-top difference without accepting a product change. The same product top drops to 45602 LUT when only the required whole-state HLS reset is removed, proving that reset-triggered state/mux/helper elaboration is dominant. That diagnostic is `REJECTED_RESET_SEMANTICS`; T3 force-inlining increases LUT to 87388 and is `REJECTED_PPA`.

Gate 3.6 status: `TOP_LEVEL_DELTA_EXPLAINED_NO_ACCEPTED_OPTIMIZATION`.

The accepted configuration therefore still retains the whole-state reset, keeps `CORE_CYCLE` pipeline disabled, and uses the Gate 3.3 resource point above.

Gate 3.7 independently reruns the conservative top and reproduces the accepted result exactly in 71.40 seconds. `P1_PIPELINE_NO_II` times out after 900 seconds during Presyn 2 transformations before scheduling, achieved-II calculation, or resource reporting. P2-P6 are not run because the required P1 report gate is not met.

Gate 3.7 status: `CORE_CYCLE_PIPELINE_TRANSFORMATION_TIMEOUT_NO_SYNTHESIS_CANDIDATE`.

No pipelined configuration is accepted. `READY_FOR_ACCEPT_PIPELINED_CONFIG=false` and the table at the top of this document remains authoritative.

Gate 3.8 independently regenerates the unchanged conservative top and again reports 83286 LUT, 16611 FF, 16 BRAM_18K, 3 DSP, and 5.898 ns. This confirms the PPA point but does not validate the generated implementation for acceptance: targeted XSim testing finds `RTL_RESET_MISMATCH` because most architectural state is not reset by runtime `ap_rst_n`.

The whole-state source reset directive remains retained; Gate 3.8 shows that retaining the directive is necessary but not sufficient to produce coherent runtime-reset RTL. No PPA configuration change is accepted, and the reset mismatch must be closed before implementation acceptance.
