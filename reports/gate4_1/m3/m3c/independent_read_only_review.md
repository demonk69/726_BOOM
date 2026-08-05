# Independent Read-Only Review: Gate 4.1 M3C

## Findings

No acceptance blocker was identified.

- M3C software passes: directed 1458 checks, random 256 seeds x 2048 cycles with zero arithmetic/protocol/integrity failures, and native full-core 15/15.
- Vitis csim full-core passes 15/15 with zero csim errors.
- Focused generated RTL passes 30/30.
- Generated `boom_core_top` full-core RTL passes 15/15, including dependency, branch, load/store, reset replay, ROB wrap, and tohost stress.
- M1/M2/M3A/M3B preservation passes: M1 decode 42/42; M2 random and focused/full-core RTL; M3A 104 directed plus 65,536 arithmetic and 65,536 protocol-random cases; M3B 167 directed, 128 x 1024 random, focused RTL 26/26, and full-core 10/10.
- W3 preservation passes 400/400 software and 11/11 focused RTL.
- W4 preservation passes 95/95 directed, 128/128 random, and 20/20 focused RTL.
- Gate 3.9 preservation passes 49/49 generated-RTL scenarios.
- All eight canonical csynth targets completed. Both full-core targets remain at 6.341 ns, below the 6.5 ns threshold, with 3 DSP and 16 BRAM; results exactly match the M3B baseline. All eight XML reports state `PipelineType=no`.
- W4 topology remains three completion sources, two PRF writes, three wakeups, three bypasses, and three ROB-complete sources. Random evidence reaches all required peaks.
- `CORE_CYCLE` remains unpipelined.
- `src/boom_all.cpp` is pre-existing dirty legacy state and is excluded from M3C compilation, synthesis, evidence, and acceptance.
- Product RTL uses `boom_core_top`; no raw `boom_core_step.v` product top exists.
- Git reports no modification under tracked frozen `reference/` traces or schemas.
- Frontend and Full LSU readiness remain explicitly false; M3C claims neither.

## Evidence

- M3C software: `logs/rv64m_full_tests.log`, `logs/rv64m_full_random_tests.log`, `logs/rv64m_full_core_tests.log`.
- Csim: `csim/vitis_csim.log`.
- Focused RTL: `rtl/rtl_test_matrix.csv` and `rtl/logs/xsim.log`.
- Full-core RTL: `full_core_rtl/full_core_rtl_matrix.csv`.
- Earlier milestones: `regression/m2b/`, `regression/m3b_recheck2/`, `rtl/m2/`, and `rtl/m3b_focused/`.
- W3/W4: `regression/w4e/regression_after.md` and `rtl/w4/rtl_focused_summary.json`.
- Gate 3.9: `reset_rtl_49/rtl_test_matrix.csv`.
- Csynth/PPA: `resource_summary.csv`, `m3b_resource_baseline.csv`, and `csynth/*/*_csynth.xml`.

## Residual Risks

- HLS timing and area are estimates, not post-route measurements.
- Strict BOOM cycle equivalence, Frontend readiness, and Full LSU readiness remain outside Gate 4.1 scope.

## Recommendation

**PASS.** No technical blocker prevents setting `GATE4_1_RV64M_VERIFIED=true`.
