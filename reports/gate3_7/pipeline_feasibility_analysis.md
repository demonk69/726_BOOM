# Gate 3.7 Pipeline Feasibility Analysis

## Pre-Synthesis Verdict

`CORE_CYCLE` has real loop-carried dependencies in every active subsystem. `II=1` is not proven semantically or structurally feasible. Gate 3.7 must ask Vitis for a legal schedule without false-dependence directives, then require RTL evidence before accepting any result.

## State Model

The top mutates one in-place `BoomCoreState`; it does not calculate a separate next-state object. The same iteration also reads/writes persistent `PipeSignals` FIFOs and can block on three AXIS outputs. Therefore iterations are not independent transactions.

The strongest explicit recurrence is:

`execute(k) -> branch/LSU(k+1) -> execute overwrite(k+1)`

Additional recurrences include frontend PC/request tracking, rename maps/free/busy state, branch snapshots/allocation lists, ROB pointers/entries, IQ entries, LSU pending transactions, CSR counters, RF data, and stream occupancy.

## Theoretical II=1

An HLS II of one could be legal only if the scheduler preserves every true dependence while initiating a new logical transition each clock. That would require each distance-1 recurrence path to have a dependence-constrained latency compatible with II=1 and enough memory/FIFO ports for overlapped iterations.

The current model makes this unlikely:

- one iteration calls a long ordered sequence of state-mutating helpers;
- dynamic ROB, IQ, map, free-list, busy, branch-list, LDQ/STQ, and RF accesses can alias;
- pipelining the outer loop implies complete unroll of many nested fixed loops;
- variable-bound loops cannot be fully unrolled;
- blocking AXIS outputs form an unbounded control/backpressure recurrence;
- the previous II=1 run did not reach scheduling after 15 minutes.

This does not prove that every II is impossible. It proves that II must be measured and that a small II cannot be assumed from the processor's architectural pipeline.

## Semantic Conditions for Overlap

| Condition | Required Result |
|---|---|
| State RAW | reader in `k+1` observes all required writes from `k` |
| State WAR/WAW | old readers finish before next overwrite; writer order remains source order |
| Branch redirect | execute result survives until next branch recovery and same-cycle frontend redirect |
| Memory protocol | transaction IDs and LSU-before-commit request ordering remain exact |
| Commit | ROB retirement, instret, trace token, and output acceptance retain ordering |
| Backpressure | no new state-changing transition advances past a blocked earlier output |
| Reset | generated RTL resets persistent architectural state and FIFOs during execution |
| Cycle count | one increment per accepted logical transition, not per internal pipeline stage |

## Tool Risks

The prior explicit II=1 pragma caused Vitis to mark 28 inner loops for complete unroll, complete 25 unrolls, and stop during Presyn 2 transformations before dependency scheduling. This means the first Gate 3.7 question is transformation closure, not achieved II.

P1 intentionally requests `PIPELINE` with no II. If P1 reaches a report, its achieved II establishes the first tool-derived bound. If P1 does not reach scheduling/report generation, P2-P6 remain blocked by the required experiment order rather than being guessed.

## Dependency Directive Decision

No false-dependence directive is justified. Dynamic-index storage and persistent control fields carry real values. Gate 3.7 will not use `DEPENDENCE false`, state replication, whole-state double buffering, `volatile`, reset removal, or stream-check removal.

## Cosim Requirement

Vitis/Vivado XSim 2021.2 is installed, but the existing cosim flow is not a valid top-level test:

- current C testbenches call `boom_core_step`, not `boom_core_top`;
- `boom_core_top` is `ap_ctrl_none` and never returns;
- no prior C/RTL cosim artifact exists;
- direct `ap_rst_n` and AXIS `TREADY` scenarios require a custom RTL testbench;
- accepted generated RTL currently provides incomplete evidence for mid-run reset of all persistent memories.

A finite wrapper may be built only for a synthesis candidate. It must call the existing cycle function and may not duplicate core logic. Without completed candidate RTL, cosim, mid-run reset, and stream-backpressure status remain `NOT_RUN_NO_SYNTHESIS_CANDIDATE` or `BLOCKED`.

## Initial Decision State

- `READY_FOR_ACCEPT_PIPELINED_CONFIG=false`
- `READY_FOR_WIDE_ISSUE_IMPLEMENTATION=false`
- `READY_FOR_LOCAL_PIPELINE_OPTIMIZATION=false` pending full-cycle characterization
- accepted Gate 3.3 conservative top unchanged

## P0/P1 Results

P0 reproduces the accepted baseline exactly: 83286 LUT, 16611 FF, 16 BRAM_18K, 3 DSP, 5.898 ns, 71.40 seconds, and 1520716 KB peak RSS.

P1 applies `PIPELINE` to `boom_core_top/CORE_CYCLE` without requesting an II. It times out at 900.00 seconds with no csynth report. The last completed named pass is `Checking Synthesizability`; internal artifacts show Presyn 1 completed and Presyn 2 started. The final visible operation is if-conversion of the 100-basic-block `older_store_in_rob` hyperblock.

| P1 Transformation Metric | Result |
|---|---:|
| implied complete-unroll markings | 65 |
| completed loop-unroll records | 57 |
| functions marked unroll-all | 7 |
| incomplete variable-bound unrolls | 8 |
| inline records | 104 |
| automatic partition records | 0 |
| memory-promotion records | 0 |
| achieved-II records | 0 |
| dependency/port-conflict scheduler records | 0 |
| maximum reported HLS current allocation | 1354.752 MB |

The `/usr/bin/time` timeout wrapper reports only 3520 KB peak RSS after killing the child and is not a valid Vitis peak-memory measurement. True P1 peak RSS is not captured.

P1 demonstrates that removing the explicit II=1 target does not avoid the transformation explosion. The outer-loop pipeline directive itself propagates complete-unroll requirements into large nested state-processing loops before scheduler recurrence analysis.

Because P1 produced no report, P2 through P5 are `NOT_RUN_BLOCKED_BY_P1_TIMEOUT`; P6 is `NOT_RUN_REQUIRED_P1_P5_REPORTS_ABSENT`. This preserves the required experiment order. No minimum synthesizable II can be reported.

Updated decision state:

- `CORE_CYCLE_PIPELINE_TRANSFORMATION_TIMEOUT_NO_SYNTHESIS_CANDIDATE`
- `MINIMUM_SYNTHESIZABLE_II=NOT_DETERMINED`
- `II1_THEORETICAL_STATUS=UNPROVEN_AND_STRONGLY_CONSTRAINED_BY_TRUE_RECURRENCES`
- `II1_TOOL_STATUS=NOT_REACHED`
