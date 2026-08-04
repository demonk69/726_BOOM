# Gate 4.0 W4B Multi ROB Complete

Historical W4B checkpoint result: **PASS**. At that checkpoint W4C multi-wakeup, bypass, and multi-PRF-write were not enabled; this is not the current final product claim.

## Review Fixes

- Completion arbitration first identifies the oldest valid retained event. If the one PRF slot is already used and that oldest event is a writing branch/JAL/JALR or precise exception, service stops for the cycle. No younger ROB, LSU, PRF, wakeup, or branch side effect can cross the fence.
- Branch mispredict recovery remains before exception marking. Recovery kills younger ROB, LSU, execute, and retained completion state. Reset clears all pending events and counters before normal service.
- Correct branch resolution clears the resolved bit in every valid retained load, MEM, and INT completion. Directed branch-tag reuse confirms that a retained completion is not falsely killed.
- Load-address/AGU and store events set `writes_prf=false`. `apply_completion` now uses that canonical classification, so an AGU cannot write the PRF, wake a destination, or consume/increment the serial writeback slot.
- W4B is macro-free by default. `BOOM_HLS_W4A_COMPLETION_DIAGNOSTIC` exists only for explicit W4A delta measurement.
- The fixed four-entry logical HLS completion interface is operative inside product service on every iteration. Named slots map LSU load response, MEM execute/LSU sideband, and INT execute; the fourth slot is invalid because the remaining generated topology sources are excluded FP-domain producers. No aggregate event is dynamically indexed.

## Directed And Random

- W4B directed checks: **17/17 PASS**.
- Fencing cases pass for older writing JAL, writing JALR, and precise exception with a younger store.
- Correct-resolution retention, branch-tag reuse, no false kill, AGU nonwrite behavior, duplicate/stale/killed rejection, reset, wrap order, trace backpressure, load plus two execute sources, and lane-2 exclusion all pass.
- Directed product counters: peak ROB complete **3**, peak PRF writes **1**, peak wakeups **1**; totals are ROB complete **28**, PRF writes **13**, wakeups **13**.
- Default-product random campaign: **100/100 seeds**, 6,400 cycles, 0 dropped and 0 duplicate tokens. Observed peak ROB complete **3**, PRF writes **1**, wakeups **1**; totals are ROB complete **3,782**, PRF writes **2,124**, wakeups **2,124**.
- Metrics come from `CompletionPendingState` instrumentation and are emitted by the tests. The regression script parses and validates the emitted values; no literal metric result is accepted.

Canonical metric evidence is `regression/w4b/w4b_metrics.csv` and `regression/w4b/product_full/random_metrics.csv`.

## Product Regression

- Software suites: **15/15 PASS**, **400/400 checks**, 0 failed across 197 runs.
- W4A diagnostic interface suite: **19/19 PASS**.
- C++ versus Vitis HLS csim normalized trace pairs: **7/7 PASS**.
- Architecture/event/cycle comparisons within the W4B product: **21/21 PASS**.
- Partial order: **7/7 PASS**.
- Full-program architectural comparison: **10/10 PASS**.
- Merged generation/compile, synthesis-top compile, and core-top compile: **4/4 PASS**.

W4A diagnostic to W4B product random-campaign deltas are recorded rather than claiming strict W3 cycle preservation:

| Event | W4A | W4B | Delta |
|---|---:|---:|---:|
| Issue | 4,253 | 4,315 | +62 |
| Execute | 4,253 | 4,315 | +62 |
| Completion | 4,757 | 4,859 | +102 |
| PRF writeback | 2,130 | 2,124 | -6 |
| ROB complete | 3,734 | 3,782 | +48 |
| Commit | 2,815 | 2,788 | -27 |
| Total random cycles | 6,400 | 6,400 | 0 |

Canonical delta evidence is `regression/w4b/w4b_event_deltas.csv`.

## Vitis HLS 2021.2

Part: `xczu7ev-ffvc1156-2-e`. Target: 10.00 ns. Seven of seven default-product targets passed.

| Top | Estimated ns | Margin ns | LUT | FF | BRAM_18K | DSP | Pipeline |
|---|---:|---:|---:|---:|---:|---:|---|
| `synth_issue_top` | 4.570 | 5.430 | 16,806 | 4,734 | 0 | 0 | no |
| `synth_execute_top` | 5.081 | 4.919 | 1,633 | 877 | 4 | 3 | no |
| `synth_completion_top` | 6.362 | 3.638 | 5,738 | 1,054 | 4 | 0 | no |
| `synth_rob_top` | 3.385 | 6.615 | 1,395 | 221 | 1 | 0 | no |
| `synth_lsu_top` | 5.428 | 4.572 | 9,418 | 2,631 | 3 | 0 | no |
| `synth_core_step_top` | 6.832 | 3.168 | 87,299 | 18,233 | 14 | 3 | no |
| `boom_core_top` | 6.832 | 3.168 | 92,567 | 18,325 | 14 | 3 | no |

Canonical values and XML paths are in `w4b_resource_summary.csv`; raw reports are under `csynth/w4b/`.

## Audits

- All seven XML reports state `PipelineType=no`; `boom_core_top` has `CORE_CYCLE` and no `PipelineII`.
- `build_rob_complete_ports` is synthesized and inlined into `service_pending`; generated bind evidence includes all three retained integer-subset sources.
- Completion bind reports contain 0 multiply, 0 completion address-multiply, and 0 reciprocal operations. Completion and ROB targets use 0 DSP; the full core retains only the expected 3 execute DSPs.
- Widths and architectural capacities remain unchanged. Commit remains ordered and width 1.
- No multi-wakeup, bypass, second PRF write, or wider commit path exists.

## Warnings And Blockers

No W4B implementation blocker remains.

Vitis HLS retains the existing HLS 200-805 default-depth stream warnings: 2 in `synth_lsu_top`, 5 in `synth_core_step_top`, and 5 in `boom_core_top`; the other targets have none. No completion stream was added.

Official external Chipyard/FESVR equivalence remains outside this local gate, as in W4A. W4C was not started.
