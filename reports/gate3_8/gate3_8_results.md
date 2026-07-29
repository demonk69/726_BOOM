# Gate 3.8 Results

Date: 2026-07-28

## Verdict

Status: `RTL_RESET_MISMATCH`.

The accepted conservative generated RTL passes normal execution, architectural trace comparison, and all tested AXIS backpressure cases, but fails required mid-run reset behavior in 3 of 45 XSim scenarios. A passing aggregate is not claimed because runtime reset is a mandatory semantic requirement.

## RTL Matrix

| Group | Pass | Fail | Result |
|---|---:|---:|---|
| Reset R0-R7 | 6 | 2 | FAIL |
| Commit-trace backpressure B0-B5 | 6 | 0 | PASS |
| IMEM I0-I6 | 7 | 0 | PASS |
| DMEM D0-D8 | 9 | 0 | PASS |
| Priority/interactions P0-P7 | 7 | 1 | FAIL |
| Normal programs N0-N6 | 7 | 0 | PASS |
| Total | 42 | 3 | FAIL |

Failures:

| Test | Observation | Classification |
|---|---|---|
| `R2_RESET_ROB_NONEMPTY` | reset with a nonempty ROB; 26 fetches, 0 commits, timeout at 40000 cycles | retained in-flight architectural state after control restart |
| `R6_RESET_BRANCH_RECOVERY` | first post-reset fetch is `0x80000000`, expected `0x10040` | frontend PC is initial-only, not runtime-reset |
| `P0_RESET_AND_BRANCH_MISPREDICT` | first post-reset fetch is `0x80000000`, expected `0x10040` | same reset/redirect priority defect |

The complete matrix is `reports/gate3_8/rtl_test_matrix.csv`.

## Architectural Trace Comparison

Seven normal programs pass C++ versus Vitis csim versus RTL comparison with no first commit or `tohost` difference: `independent_alu`, `raw_chain`, `branch_taken`, `branch_not_taken`, `nested_branch`, `load_store`, and `tohost`.

This closes generated-RTL evidence for normal operation in the supported subset. It does not override the reset failures and does not establish strict BOOM cycle equivalence.

## AXIS and Interaction Coverage

Commit trace stalls, request stalls, delayed/random IMEM and DMEM responses, stale responses after redirect/flush, response activity during reset, store acceptance, branch redirect interactions, and `tohost` backpressure pass. No protocol error is reported in the matrix.

## Reset Audit

The generated RTL has mixed reset behavior:

- HLS FSMs, helper start flags, stream FIFO occupancy, AXIS register-slice control, selected frontend-private state, branch-update valid flags, and CSR cycle state are explicitly reset.
- FIFO and register-slice payload is retained but safely hidden by reset validity/occupancy.
- Frontend PC/validity, ROB, IQ, issued uops, rename maps/free/busy state, branch snapshots/allocation lists, RF, execute results, and LSU queues/bookkeeping are initialized only at elaboration and retained across runtime reset.
- Generated architectural RAM modules expose a reset input but do not use it in their clocked storage process.

See `reports/gate3_8/rtl_reset_state_inventory.csv` and `reports/gate3_8/reset_semantics_analysis.md`.

## Synthesis and Regression Preservation

The final unchanged source reproduces the accepted conservative result:

| Metric | Gate 3.8 result |
|---|---:|
| LUT | 83286 |
| FF | 16611 |
| BRAM_18K | 16 |
| DSP | 3 |
| Estimated period | 5.898 ns |
| `CORE_CYCLE` pipeline | disabled |

Final regressions remain directed 25/25, Gate 1 13/13, LSU 14/14, branch directed/random 30/30 and 42/42, IQ 10/10, frozen HLS trace 10/10 byte-identical, and BOOM architectural diff 10/10. The two added normal programs also pass C++/csim/RTL comparison.

## Readiness

| Decision | Value |
|---|---|
| `READY_FOR_LOCAL_PIPELINE_OPTIMIZATION` | false |
| `READY_FOR_WIDE_ISSUE_IMPLEMENTATION` | false |
| `READY_FOR_OFFICIAL_GATE_3` | false |
| Runtime reset verified | false |
| Pin-level AXIS backpressure verified for tested scenarios | true |

Official Gate 3 remains independently blocked by the unavailable original Chipyard/FESVR/DRAMSim environment. Gate 3.8 additionally blocks acceptance on the demonstrated generated-RTL runtime reset mismatch.

## Primary Evidence

- `reports/gate3_8/rtl_test_matrix.csv`
- `reports/gate3_8/rtl_run_status.csv`
- `reports/gate3_8/rtl_reset_state_inventory.csv`
- `reports/gate3_8/reset_semantics_analysis.md`
- `reports/gate3_8/traces/`
- `reports/gate3_8/regression_after.md`
- `reports/gate3_8/artifact_integrity.md`
- `reports/gate3_8/fixed_synthesis/boom_core_top_csynth.rpt`
