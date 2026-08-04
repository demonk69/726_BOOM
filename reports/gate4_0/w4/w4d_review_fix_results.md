# W4D Review Blocker Fixes

Result: **PASS for cumulative W4D review fixes**. This is not a W4E final claim; random-128 and the final RTL matrix were not run.

## Precise Validation Fault

Pending control is resolved before writer validation. Mispredict recovery first removes younger writers; a correct branch preserves cross-boundary writers for validation. Same-`pdst`/different-value surviving results no longer remain permanently pending. The oldest source by ROB age and source priority records sticky fault identity and cause `0x100`; all participants become nonbusy exception entries and their pending events are consumed. No PRF write, wakeup, or bypass occurs in the fault cycle. A ROB-wide exception fence prevents younger completion side effects on later cycles. At the head, commit emits exactly one exception trace and asserts `io_trap`; reset clears fault/trap state. Directed cross-boundary conflict, unrelated-younger suppression, mispredict-killed non-conflict, multi-cycle fault, no-repeat trace, and reset recovery pass.

Measured clean-run counters: dropped completions **0**, dropped writebacks **0**, duplicate physical writes **0**, conflicts **1**, validation faults **1**, fault events consumed **2**, and same-value de-duplications **1**. A directed duplicate-held-source invariant probe increments the completion-drop counter exactly once and removes the duplicate execute copy.

## Wakeup Fence

Branch resolution, surviving-writer validation, and publication are ordered explicitly. A correct branch with a younger writer shows no younger wakeup/write in the branch cycle; the retained writer publishes and writes on the next cycle. Publication and writeback service share one allocation-valid oldest-exception scan over pending events and live ROB entries. A validation-fault cycle publishes nothing, retained younger events remain suppressed on second and subsequent cycles until kill/reset, and events older than the exception may progress.

## Status Ports

Root cause was Vitis 2021.2 dead-store elimination of nonvolatile scalar reference/pointer status writes in the non-returning `boom_core_top` loop. The public HLS top now uses write-only volatile pointers; internal helper references are unchanged. Generated directions are:

| Port | Direction | Width |
|---|---|---:|
| `io_success` | output | 1 |
| `io_halted` | output | 1 |
| `io_trap` | output | 1 |
| `io_cycle_valid` | output | 1 |
| `io_cycle` | output | 64 |
| `io_instret` | output | 64 |

Both XSim harnesses now connect these outputs to wires. Product RTL compile/elaboration passes, and `independent_alu/R0_POWER_ON_RESET` passes in 21,096 RTL clocks with 12 commits.

## Verification

- Cumulative product: **400/400**, 15/15 suites, 197 runs.
- W4A **19/19**, W4B **17/17**, wakeup/fence **14/14**, bypass **14/14**, writeback/fault **13/13**, completion **6/6**, oracle C++ **10/10**.
- W2 random **64/64**; W3 random **100/100**, 6,400 cycles; drops and duplicates **0**.
- C++/csim pairs **7/7**; architecture/event/cycle **21/21**; full program **10/10**; partial order **7/7**; cycle endpoints unchanged.
- Independent oracle synthesis **1/1** and generated RTL **10/10**, including three-cycle persistent exception-fence publication suppression.
- Canonical csynth **7/7**; two PRF write enables and six output status directions are script-enforced.
- Final `boom_core_top`: estimated period **5.976 ns**, **107,693 LUT**, **24,918 FF**, **16 BRAM_18K**, **3 DSP**. Bind audit is zero across all seven targets.

Exact resources are in `w4d_resource_summary.csv`; metrics in `regression/w4d/w4d_metrics.csv`; the exact 10-case oracle matrix is in `w4d_rtl_oracle_results.csv`; PRF and status directions are in `prf_after.csv` and `status_ports_after.csv`.

## Remaining

Twelve inherited HLS 200-805 stream-depth warnings and 42 HLS 207-5523 deprecated-resource warnings remain non-blocking. External Chipyard/FESVR equivalence remains unavailable. W4E campaigns remain deferred.
