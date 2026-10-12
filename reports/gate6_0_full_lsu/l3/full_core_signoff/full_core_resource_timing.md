# Full-Core Resource And Timing

## PPA4 Compact Route From DCP Audit

| Metric | Full core `core_u` | Wrapper/adapter | Total system |
| --- | ---: | ---: | ---: |
| LUT | `48458` | `994` (`memory_u=931`, top glue=63) | `49452` |
| FF | `29567` | `2265` | `31832` |
| RAMB36 | `11` | `0` | `11` |
| RAMB18 | `34` | `0` | `34` |
| BRAM tile | not separately hierarchical | not separately hierarchical | `28` |
| DSP | `16` | `0` | `16` |
| Bonded IOB | internal core streams not package pins | exported status/debug | `265/360` |
| Clock buffers | not separately hierarchical | not separately hierarchical | `1 BUFGCE` |

Evidence: `/home/lab_726/opencode_tmp/g6_l3r5_ppa5_ppa4_netlist_audit_20261010/ppa4_hier_utilization_from_dcp.rpt` and `ppa4_utilization_from_dcp.rpt`.

## Timing

| Scope | WNS | TNS | Status | Evidence |
| --- | ---: | ---: | --- | --- |
| PPA4 compact system | `0.434 ns` | `0.000 ns` | PASS for user-specified `ap_clk` | `ppa4_timing_summary_from_dcp.rpt` |
| Core timing | not separately constrained/reported | not separately constrained/reported | INCLUDED_IN_SYSTEM_TIMING | Same routed DCP; no separate core clock group report. |
| Adapter timing | not separately constrained/reported | not separately constrained/reported | INCLUDED_IN_SYSTEM_TIMING | Same routed DCP; no separate adapter clock group report. |
| Full PPA5 board timing | not run | not run | BLOCKED | No board XDC or I/O delays. |

The compact system timing result must not be reused as a new PPA5 full-board timing result. It remains PPA4 compact package-level physical evidence only.

## Constraint Audit

| Check | Status | Evidence |
| --- | --- | --- |
| No-clock registers | PASS | `check_timing`: `0`. |
| Unconstrained internal endpoints | PASS | `check_timing`: `0`. |
| Input delay constraints | MISSING | `check_timing`: `1` input port with no input delay. |
| Output delay constraints | MISSING | `check_timing`: `261` ports with no output delay. |
| Multiple clocks | PASS | `check_timing`: `0`. |
| Generated clock issues | PASS | `check_timing`: `0`. |
| False paths / multicycle paths | NONE_REVIEWED | No board-level exception strategy present. |

## Status Fields

`L3_PPA5_FULL_CORE_LUT=48458_CORE_49452_TOTAL_COMPACT_SYSTEM`

`L3_PPA5_FULL_CORE_FF=29567_CORE_31832_TOTAL_COMPACT_SYSTEM`

`L3_PPA5_FULL_CORE_BRAM=28_TOTAL_BRAM_TILES_COMPACT_SYSTEM`

`L3_PPA5_FULL_CORE_DSP=16_CORE_16_TOTAL_COMPACT_SYSTEM`

`L3_PPA5_FULL_CORE_ROUTE=PASS_FOR_PPA4_COMPACT_SYSTEM_NOT_BOARD_SIGNOFF`

`L3_PPA5_FULL_CORE_WNS=0.434_COMPACT_SYSTEM_ONLY`

`L3_PPA5_FULL_CORE_TNS=0.000_COMPACT_SYSTEM_ONLY`
