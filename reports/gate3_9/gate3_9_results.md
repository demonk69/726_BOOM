# Gate 3.9 Results

Date: 2026-07-29

## Verdict

Status: `RTL_RESET_VERIFIED`.

The F1 fine-grain reset architecture closes M014 for the supported HLS core. Generated RTL passes all 49 serial XSim scenarios, including the three Gate 3.8 counterexamples and four new reset-reentry stress cases. Of 66 observed reset releases, 60 complete and restart fetch/commit; six are intentionally interrupted by a scenario's runtime reset assertion and the following release completes normally.

## Implementation

`BoomCoreState` is no longer attached to a whole-state HLS reset pragma. A reset-only `ResetControllerState` drives a deterministic 145-step initialization sequence while normal core execution and output requests remain inactive. The sequence restores:

- frontend PC to `0x10040` and clears pending frontend state;
- speculative and committed maps to physical register zero;
- free/busy state to the accepted physical-register allocation contract;
- ROB, IQ, issued, branch-recovery, execute, and LSU validity/bookkeeping state;
- CSR/reset-visible state, physical register zero, and architectural outputs.

Payload storage that becomes unreachable after validity, map, queue, and busy state are reset is not bulk-cleared.

## RTL Matrix

| Group | Pass | Fail | Result |
|---|---:|---:|---|
| Reset R0-R7 | 8 | 0 | PASS |
| Commit-trace backpressure B0-B5 | 6 | 0 | PASS |
| IMEM I0-I6 | 7 | 0 | PASS |
| DMEM D0-D8 | 9 | 0 | PASS |
| Priority/interactions P0-P7 | 8 | 0 | PASS |
| Normal programs N0-N6 | 7 | 0 | PASS |
| Added reset stress R8-R11 | 4 | 0 | PASS |
| Total | 49 | 0 | PASS |

The added cases cover two runtime resets, reset during the initialization sequence, reset immediately after release, and reset/restart after a completed `tohost` transaction. `reset_latency.csv` records 60 completed releases with subsequent fetch and commit, six intentionally interrupted releases, and zero failures. First fetch after every completed reset is from `0x10040`; an ordinary completed reset has 688 RTL testbench cycles from release to first fetch.

## Trace and Regression Preservation

The seven normal generated-RTL traces are 7/7 architecturally identical to the frozen Gate 3.8 traces after excluding simulator cycle and provenance fields. Absolute generated-RTL commit cycles are 0/7 exact because the reset initialization and regenerated FSM schedule changed; no cycle-equivalence claim is made.

Accepted-source regressions pass:

- directed 25/25, Gate 1 13/13, LSU 14/14, branch directed 30/30, IQ compaction 10/10;
- branch random regression for all 21 configured seeds;
- reset architecture 14/14;
- frozen C++ and csim traces 10/10 byte-identical;
- full-program C++/csim architectural diff 10/10.

## Synthesis

| Metric | Gate 3.8 | Gate 3.9 F1 | Delta |
|---|---:|---:|---:|
| LUT | 83286 | 47999 | -35287 |
| FF | 16611 | 12134 | -4477 |
| BRAM_18K | 16 | 12 | -4 |
| DSP | 3 | 3 | 0 |
| Estimated period | 5.898 ns | 5.898 ns | 0 ns |
| `CORE_CYCLE` pipeline | disabled | disabled | unchanged |

The conservative synthesis report explicitly records `CORE_CYCLE` as `Pipelined=no`.

## Readiness

| Decision | Value |
|---|---|
| M014 generated-RTL runtime reset | VERIFIED |
| `READY_FOR_LOCAL_PIPELINE_OPTIMIZATION` | true for the supported local model |
| `READY_FOR_WIDE_ISSUE_IMPLEMENTATION` | false |
| `READY_FOR_OFFICIAL_GATE_3` | false |

Official Gate 3 remains independently blocked by the unavailable original Chipyard/FESVR/DRAMSim path and by the broader model limitations tracked in M012/M013. Gate 3.9 closes the local generated-RTL reset defect; it does not establish strict BOOM cycle equivalence.

## Primary Evidence

- `reports/gate3_9/rtl_test_matrix.csv`
- `reports/gate3_9/reset_latency.csv`
- `reports/gate3_9/normal_rtl_trace_comparison.csv`
- `reports/gate3_9/regression_after_artifacts/`
- `reports/gate3_9/variants/F1_FINE_GRAIN_RESET/boom_core_top_csynth.rpt`
- `reports/gate3_9/rtl_traces/`
- `reports/gate3_9/baseline_artifacts/`
