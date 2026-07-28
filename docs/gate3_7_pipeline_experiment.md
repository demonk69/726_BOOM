# Gate 3.7 CORE_CYCLE Pipeline Experiment

## Status

`CORE_CYCLE_PIPELINE_TRANSFORMATION_TIMEOUT_NO_SYNTHESIS_CANDIDATE`

Gate 3.7 preserves the Gate 3.3/Gate 3.6 accepted configuration and characterizes the first legal pipeline step. It does not add a pipeline pragma to accepted source.

## Main Finding

`CORE_CYCLE` iterations are not independent. The current single-state model has real distance-1 and longer recurrences through execute/branch/LSU, frontend request state, rename state, ROB, IQ, branch recovery, CSR, RF, and streams. External output backpressure also forms a control boundary between logical processor cycles.

P1 asks Vitis to pipeline the loop without an II target. It still triggers nested complete-unroll propagation and times out after 900 seconds before scheduling:

| Evidence | P1 |
|---|---:|
| implied complete-unroll loops | 65 |
| completed unrolls | 57 |
| function-wide unrolls | 7 |
| variable-bound unroll failures | 8 |
| inline records | 104 |
| automatic partitions | 0 |
| achieved II | not reported |
| resource/timing report | none |

The last completed named pass is `Checking Synthesizability`; Presyn 1 completes and Presyn 2 stops during if-conversion. This matches the stage of the historical Gate 3.2 II=1 timeout and shows that the transformation problem is caused by requesting the outer-loop pipeline, not solely by an II=1 target.

## Matrix Decision

| Variant | Decision |
|---|---|
| P0 baseline | PASS and exact accepted-resource reproduction |
| P1 no II | TIMEOUT_NO_REPORT |
| P2 II=16 | not run; P1 report gate unmet |
| P3 II=8 | not run; P1 report gate unmet |
| P4 II=4 | not run; P1 report gate unmet |
| P5 II=2 | not run; P1 report gate unmet |
| P6 II=1 | not run; required P1-P5 reports absent |

Minimum synthesizable II is `NOT_DETERMINED`. No scheduler dependence or memory-port report exists because Vitis does not reach scheduling.

## Verification Decision

XSim 2021.2 is available, but no pipeline RTL candidate exists. Existing C testbenches call `boom_core_step` rather than the selected product top, and the infinite `ap_ctrl_none` top cannot provide bounded automatic-cosim transactions.

C/RTL cosim, pin-level AXIS backpressure, and mid-run `ap_rst_n` are not run. No candidate can be accepted from C++/csim evidence.

## Accepted Configuration

- top: `boom_core_top`
- pipeline: disabled
- whole-state source reset directive: retained
- LUT: 83286
- FF: 16611
- BRAM_18K: 16
- DSP: 3
- estimated period: 5.898 ns

## Readiness

| Flag | Value |
|---|---|
| `READY_FOR_ACCEPT_PIPELINED_CONFIG` | false |
| `READY_FOR_WIDE_ISSUE_IMPLEMENTATION` | false |
| `READY_FOR_LOCAL_PIPELINE_OPTIMIZATION` | false |
| `READY_FOR_OFFICIAL_GATE_3` | false |

See `reports/gate3_7/gate3_7_results.md` for complete results.
