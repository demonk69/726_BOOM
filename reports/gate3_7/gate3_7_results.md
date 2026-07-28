# Gate 3.7 Results

Date: 2026-07-28

## Verdict

Status: `CORE_CYCLE_PIPELINE_TRANSFORMATION_TIMEOUT_NO_SYNTHESIS_CANDIDATE`.

Gate 3.7 confirms real loop-carried state and stream dependencies, reproduces the accepted conservative synthesis, and runs the required first pipeline experiment without an II target. P1 times out after 900 seconds during Presyn 2 transformations before scheduling or report generation. P2-P6 are not run because the required P1 report gate is not met. No pipeline source change or configuration is accepted.

## Baseline

| Metric | Accepted Baseline / P0 |
|---|---:|
| LUT | 83286 |
| FF | 16611 |
| BRAM_18K | 16 |
| DSP | 3 |
| Estimated period | 5.898 ns |
| P0 runtime | 71.40 s |
| P0 peak memory | 1520716 KB |
| `CORE_CYCLE` pipeline | disabled |
| Whole-state reset directive | retained |

## Pipeline Matrix

| Variant | Requested II | Status | Achieved II | Runtime | Last Stage | Resources |
|---|---:|---|---:|---:|---|---|
| P0_BASELINE | none | PASS | not pipelined | 71.40 s | csynth complete | 83286 LUT / 16611 FF / 16 BRAM / 3 DSP / 5.898 ns |
| P1_PIPELINE_NO_II | automatic | TIMEOUT | not reported | 900.00 s | Presyn 2 if-conversion | no report |
| P2_PIPELINE_II_16 | 16 | NOT_RUN_BLOCKED_BY_P1_TIMEOUT | not reported | - | not started | no report |
| P3_PIPELINE_II_8 | 8 | NOT_RUN_BLOCKED_BY_P1_TIMEOUT | not reported | - | not started | no report |
| P4_PIPELINE_II_4 | 4 | NOT_RUN_BLOCKED_BY_P1_TIMEOUT | not reported | - | not started | no report |
| P5_PIPELINE_II_2 | 2 | NOT_RUN_BLOCKED_BY_P1_TIMEOUT | not reported | - | not started | no report |
| P6_PIPELINE_II_1 | 1 | NOT_RUN_REQUIRED_P1_P5_REPORTS_ABSENT | not reported | - | not started | no report |

## P1 Transformation Evidence

| Metric | Result |
|---|---:|
| implied complete-unroll markings | 65 |
| completed loop-unroll records | 57 |
| functions marked unroll-all | 7 |
| incomplete variable-bound unrolls | 8 |
| inline records | 104 |
| automatic partition records | 0 |
| memory-promotion records | 0 |
| scheduler dependency messages | 0 |
| memory-port conflict messages | 0 |
| achieved-II messages | 0 |
| maximum reported HLS current allocation | 1354.752 MB |

P1's timeout-wrapper peak RSS of 3520 KB is not a valid child-process peak measurement. True P1 peak RSS is not captured.

P1 and the historical II=1 run both finish synthesizability and Presyn 1, then stop in Presyn 2 with final if-conversion activity. Removing the II target does not prevent outer-loop pipeline propagation into nested complete-unroll transformations.

## Required Answers

| Question | Answer |
|---|---|
| 1. Does CORE_CYCLE have real loop-carried RAW dependencies? | Yes. Execute results, frontend request state, rename maps/free/busy state, branch snapshots/lists, ROB/IQ/LSU state, CSR counters, RF data, and FIFO occupancy all carry real values across iterations. |
| 2. Which state limits II? | Strongest feedback includes execute(k) to branch/LSU(k+1), PC/request tracking, dynamic ROB/IQ/map/free/busy/RF accesses, pending load transactions, branch tags/snapshots, cycle/instret recurrences, and blocking output streams. No scheduler-derived numerical limit was reached. |
| 3. Where did the previous pipeline attempt time out? | Gate 3.2 II=1 timed out after 15 minutes in Presyn 2 transformations/if-conversion, after synthesizability and Presyn 1 but before scheduling. |
| 4. P1 automatic achieved II | Not reported. P1 timed out before scheduling/report generation. |
| 5. P2-P6 status | P2-P5 not run because P1 produced no valid report. P6 additionally lacked the required complete P1-P5 report set. |
| 6. Minimum synthesizable II | `NOT_DETERMINED`; no pipelined report exists. |
| 7. Is II=1 theoretically/tool feasible? | Theoretically unproven and strongly constrained by true recurrences/backpressure. Tool feasibility was not reached; both prior II=1 and current no-II attempts time out before scheduling. |
| 8. Does pipeline change state-transition semantics? | It permits iteration overlap and can change semantics unless all RAW/WAR/WAW/control/FIFO/backpressure ordering is preserved. No pipeline RTL exists to prove equivalence. |
| 9. Any false-dependence directive used? | No. Count is zero for all variants. |
| 10. Variant resources | P0 only: 83286 LUT, 16611 FF, 16 BRAM, 3 DSP. P1-P6 have no resource report. |
| 11. Variant clocks | P0: 5.898 ns. P1-P6: not reported. |
| 12. Variant synthesis time | P0: 71.40 s. P1: timeout at 900.00 s. P2-P6: not started. |
| 13. Is cosim available? | XSim 2021.2 is installed, but no pipeline RTL candidate exists and the current infinite top/testbenches are not a valid automatic cosim setup. |
| 14. Was mid-run reset verified? | No. Source reset is retained; native reset-by-assignment is not RTL `ap_rst_n`. Existing conservative RTL also requires a full reset audit. |
| 15. Was stream backpressure verified? | Native delayed-response tests pass, but pin-level AXIS `TREADY` backpressure was not run without candidate RTL. |
| 16. FUNCTIONALLY_VERIFIED_CANDIDATE | None. There is no synthesis candidate. |
| 17. Final accepted configuration | Gate 3.3 conservative `boom_core_top`: 83286 LUT, 16611 FF, 16 BRAM, 3 DSP, 5.898 ns, pipeline disabled, reset directive retained. |
| 18. READY_FOR_WIDE_ISSUE_IMPLEMENTATION | false. Pipeline characterization did not close and this gate did not evaluate wide issue. |
| 19. READY_FOR_LOCAL_PIPELINE_OPTIMIZATION | false. No full-cycle schedule/dependency report or trace-invariant local region was established. |
| 20. READY_FOR_OFFICIAL_GATE_3 | false. Chipyard/FESVR/DRAMSim environment remains unavailable. |

## Local Pipeline Decision

L1-L4 are not run. The prerequisite full-cycle scheduling/dependency report was not produced, and no local loop was proven both feedback-free and same-cycle trace invariant. Branch kill and busy rebuild remain same-cycle recovery operations whose latency cannot be changed without RTL evidence.

## Cosim and Reset

XSim, `xelab`, and `xvlog` are available. Existing C testbenches bypass the selected top, while `boom_core_top` is infinite `ap_ctrl_none`; no prior C/RTL cosim evidence exists. A finite wrapper would be required for a synthesized candidate, followed by a custom RTL testbench for `ap_rst_n` and AXIS backpressure.

No such wrapper is added because no pipeline candidate RTL exists. `READY_FOR_ACCEPT_PIPELINED_CONFIG=false`.

## Regression Preservation

Final accepted source passes directed 25/25, Gate 1 13/13, LSU 14/14, branch directed 30/30, branch random 42/42, IQ 10/10, trace 10/10 byte-identical, BOOM architectural diff 10/10, and partial-order 8 legal/0 real.

Strict BOOM cycle equivalence remains `INSUFFICIENT_EVIDENCE`; `M009` remains `PARTIALLY_VERIFIED`.

## Primary Evidence

- `reports/gate3_7/loop_carried_dependency_inventory.csv`
- `reports/gate3_7/pipeline_feasibility_analysis.md`
- `reports/gate3_7/previous_pipeline_timeout_analysis.md`
- `reports/gate3_7/pipeline_variant_summary.csv`
- `reports/gate3_7/cosim_feasibility.md`
- `reports/gate3_7/regression_after.md`
- `docs/core_cycle_pipeline_semantics.md`
- `docs/gate3_7_pipeline_experiment.md`
