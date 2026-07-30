# Gate 4.0 W2 Critical-Path Analysis

## Result

W2 dual selection is not the integrated core timing limiter. The conservative `boom_core_top` estimate remains 5.898 ns, equal to W1, and `CORE_CYCLE` remains unpipelined.

| Scope | State | Delay | Path |
|---|---:|---:|---|
| `boom_core_top` / `lsu_module` | 5 | 5.89 ns | Load-response extraction, including ROB metadata reads, shift/mask construction, and sign extension |
| `boom_core_top` / `execute_module` | 4 | 5.87 ns | Integer PRF reads and operand/result selection |
| `synth_issue_top` / `issue_module` | 12 | 4.57 ns | IQ readiness and uop classification followed by oldest MEM/INT selection |

The integrated limiter remains the pre-existing LSU load-response path. The standalone issue path has 1.32 ns of estimated margin relative to the 5.89 ns LSU state.

## Evidence

- Product summary: `reports/gate4_0/w2/csynth/boom_core_top/boom_core_top_csynth.rpt`.
- Issue summary: `reports/gate4_0/w2/csynth/synth_issue_top/synth_issue_top_csynth.rpt`.
- Core-step summary: `reports/gate4_0/w2/csynth/synth_core_step_top/synth_core_step_top_csynth.rpt`.
- Detailed LSU state: `boom_hls_gate4_0_w2_boom_core_top/solution_baseline/.autopilot/db/lsu_module.verbose.rpt`, State 5.
- Detailed execute state: `boom_hls_gate4_0_w2_boom_core_top/solution_baseline/.autopilot/db/execute_module.verbose.rpt`, State 4.
- Detailed issue state: `boom_hls_gate4_0_w2_synth_issue_top/solution_baseline/.autopilot/db/issue_module.verbose.rpt`, State 12.

No pipeline, DATAFLOW, complete-array partition, false-dependence override, or capacity reduction was introduced for W2.
