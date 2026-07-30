# Gate 3.10 Results

Status: `LOCAL_PIPELINE_CHARACTERIZED_NO_ACCEPTED_CANDIDATE`.

## Critical Path

Gate 3.9 Vitis HLS verbose schedules identify the longest state-local path as `lsu_module` load-response extraction at 5.90 ns: ROB address read, variable shift/mask/sign extension, selection, and ROB data write. Execute multiply is second at 5.87 ns. These are HLS scheduling estimates, not physical STA paths.

## Legality

- Legal experiment: reset-only ROB valid/busy sweep.
- `L1_FREE_LIST_SELECT`: real head/count/first-match recurrence.
- `L2_ISSUE_SELECT`: oldest-ready/grant/compaction recurrence.
- `L3_BRANCH_MASK_REDUCTION`: same-cycle branch recovery.
- `L4_ROB_COMMIT`: ordered commit plus trace/store handshake.
- `L5_LSU_SELECT`: pending-transaction and oldest-request recurrence.

No normal-path named loop is legal to pipeline under the required exact-cycle contract.

## R1 Result

`R1_RESET_INIT_PIPELINE` pipelines only `RESET_ROB_INIT`, requested II=1, achieved II=1, loop latency 32 cycles. C++ reset calls fall from 145 to 114. Resources are 48019 LUT, 12191 FF, 12 BRAM, 3 DSP at 5.898 ns; `CORE_CYCLE` remains unpipelined.

All regressions and XSim 49/49 pass, but RTL reset-to-first-fetch worsens from 688 to 886 cycles. Normal event cycles relative to first fetch are 0/7 exact and `load_store` changes external event ordering. R1 is rejected.

## Clock Sweep

All requested targets synthesize. The best HLS estimate is 3.255 ns at a requested 4.5 ns for both P0 and R1. This is not an achieved physical clock. P0/4.5 normal RTL remains architecturally 7/7 but event cycles are 0/7 exact, so it is characterization-only and the full 49-case matrix is not promoted.

## Final Decisions

| Question | Answer |
|---|---|
| Best local pipeline candidate | R1 reset-only ROB sweep, rejected |
| Best clock target result | requested 4.5 ns, estimated 3.255 ns, cycle-inexact |
| Accepted Gate 3.10 candidate | none |
| Final accepted configuration | Gate 3.9 commit `557bdf5` |
| M014 | `VERIFIED` |
| M009 | `PARTIALLY_VERIFIED` |
| `READY_FOR_WIDE_ISSUE_IMPLEMENTATION` | true for a separately gated local implementation experiment |
| `READY_FOR_LOCAL_PIPELINE_OPTIMIZATION` | false; named legal space characterized with no accepted candidate |
| `READY_FOR_OFFICIAL_GATE_3` | false |

Strict BOOM cycle equivalence remains `INSUFFICIENT_EVIDENCE`. Official Gate 3 remains blocked by the missing Chipyard/FESVR/DRAMSim environment.
