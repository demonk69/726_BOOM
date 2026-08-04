# Gate 4.0 W4C Multi Wakeup And Bypass

Historical W4C checkpoint result: **PASS**. At that checkpoint W4D multiwrite was not implemented; this is not the current final product claim.

## Semantics

- The fixed HLS interfaces are three `WakeupEvent` ports and three `BypassEvent` ports, matching `NUM_INT_WAKEUP_PORTS=3` and `NUM_INT_BYPASS_PORTS=3`.
- At the start of a core cycle, completion captures execute/load results, validates live ROB allocation ownership, and publishes a stable cycle snapshot before issue. Issue latches matching data into `prs1`, `prs2`, and `prs3`; execute checks x0, then the validated bypass snapshot, then the PRF.
- A retained result may wake and supply consumers while its global busy bit remains set. Only actual serialized PRF writeback clears that bit, so later rename/dispatch cannot treat stale PRF contents as ready.
- Identical same-`pdst`/value events de-duplicate. Differing values for one `pdst` are counted and both results are rejected from wakeup, bypass, completion, and PRF writeback.
- Correct branch resolution clears masks in retained and transient state. Mispredict recovery removes younger pending completion, wakeup, and bypass state before issue. Reset reconstructs `CompletionPendingState`, including ports and wakeup-sent state.
- W4B branch/exception fencing, allocation IDs, commit width one, and serial PRF writeback remain unchanged.

## Verification

- W4C directed: multi-wakeup **13/13 PASS**; bypass **11/11 PASS**.
- Cumulative W4A interface: **19/19 PASS**; cumulative W4B completion: **17/17 PASS**.
- Product software suites: **15/15 PASS**, **400/400 checks** across 197 runs.
- W3 persistent random: **100/100 seeds**, 6,400 cycles, 0 drops and 0 duplicates. W2 random: **64/64 seeds**, 2,048 cycles.
- C++ versus Vitis HLS csim trace pairs: **7/7 PASS**. Architecture/event/cycle checks: **21/21 PASS**. Full program: **10/10 PASS**. Partial order: **7/7 PASS**.
- Directed measured peaks: wakeups **3**, bypasses **2**, PRF writes **1**. Three retained writers drain in at most **3 cycles**.
- Random measured peaks: wakeups **2**, PRF writes **1**; 2,200 wakeups and 2,124 PRF writes demonstrate that wakeup is no longer equated with writeback.
- All seven loaded-program cycle counts are unchanged from W4B; exact values are in `regression/w4c/w4c_cycle_deltas.csv`.

Metric evidence is in `regression/w4c/w4c_metrics.csv`, `regression/w4c/product_full/random_metrics.csv`, and `regression/w4c/w4c_event_deltas.csv`.

## Synthesis

Vitis HLS 2021.2, `xczu7ev-ffvc1156-2-e`, 10.00 ns target: **7/7 PASS**.

| Top | Estimated ns | Margin ns | LUT | FF | BRAM_18K | DSP |
|---|---:|---:|---:|---:|---:|---:|
| `synth_issue_top` | 4.445 | 5.555 | 17,301 | 4,821 | 0 | 0 |
| `synth_execute_top` | 5.009 | 4.991 | 1,816 | 384 | 4 | 3 |
| `synth_completion_top` | 6.443 | 3.557 | 9,079 | 1,986 | 4 | 0 |
| `synth_rob_top` | 3.385 | 6.615 | 3,273 | 632 | 1 | 0 |
| `synth_lsu_top` | 5.600 | 4.400 | 13,769 | 3,526 | 3 | 0 |
| `synth_core_step_top` | 6.832 | 3.168 | 96,847 | 21,426 | 12 | 3 |
| `boom_core_top` | 6.832 | 3.168 | 102,647 | 21,518 | 12 | 3 |

Exact XML paths, runtime, memory, and resources are in `w4c_resource_summary.csv`. Bind audit is in `w4c_bind_audit.csv`; every target has zero completion multiply, address-multiply, and reciprocal operations. All reports state `PipelineType=no`; `boom_core_top` retains unpipelined `CORE_CYCLE`.

## Blockers

No W4C implementation blocker remains. The pre-existing HLS 200-805 default stream-depth warnings remain: two for `synth_lsu_top`, five for `synth_core_step_top`, and five for `boom_core_top`. Official external Chipyard/FESVR equivalence remains unavailable, as in W4B.
