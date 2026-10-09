# PPA3 Fanout And Congestion Analysis

## Scope

This report correlates the failing 6.450 ns timing paths from the old PPA2 routed checkpoint with high-fanout and congestion reports, then compares the result with the fresh 6.450 ns implementation.

## Old Routed Checkpoint At 6.450 ns

The old routed checkpoint had no reported congestion windows above level 5, but still failed setup due to route-dominated high-fanout nets.

| Evidence | Value |
|---|---|
| Timing summary | `/home/lab_726/opencode_tmp/g6_l3r5_ppa3_timing_audit_20261009/report_timing_summary_6p450_100.rpt` |
| High-fanout report | `/home/lab_726/opencode_tmp/g6_l3r5_ppa3_timing_audit_20261009/report_high_fanout_nets_6p450.rpt` |
| Congestion report | `/home/lab_726/opencode_tmp/g6_l3r5_ppa3_timing_audit_20261009/report_congestion_6p450.rpt` |
| Design-analysis timing | `/home/lab_726/opencode_tmp/g6_l3r5_ppa3_timing_audit_20261009/report_design_analysis_timing_6p450.rpt` |

Worst setup path characteristics from `report_design_analysis_timing_6p450.rpt`:

| Metric | Value |
|---|---:|
| Requirement | `6.450 ns` |
| Path delay | `9.180 ns` |
| Logic delay | `0.079 ns` |
| Net delay | `9.101 ns` |
| Net delay share | `99%` |
| Logic levels | `0` |
| High fanout | `253` |
| Slack | `-2.835 ns` |

Relevant high-fanout entries include:

| Net | Fanout | Worst slack | Worst delay | Note |
|---|---:|---:|---:|---|
| `grp_boom_core_cycle_io_fu_6197/ap_CS_fsm_state3` | 3213 | `0.205 ns` | `5.481 ns` | Highest fanout, but not the worst failing path. |
| `grp_boom_core_cycle_io_fu_6197/grp_frontend_mode_module_true_s_fu_5628/grp_build_fetch_packet_fu_7015/misaligned_redirect_reg_2124_reg[0]_1[0]` | 1041 | `-1.132 ns` | `1.525 ns` | Large frontend fanout with negative slack. |
| `grp_boom_core_cycle_io_fu_6197/grp_completion_service_cycle_fu_2363/grp_service_pending_fu_4598/Q[12]` | 644 | `-1.380 ns` | `7.646 ns` | Route-heavy completion service fanout. |
| `pipe_4_fifo_U/mOutPtr_reg_n_125_[0]` | 391 | `-1.146 ns` | `7.077 ns` | Stream FIFO read pointer fanout. |
| `grp_boom_core_cycle_io_fu_6197/grp_rob_commit_module_fu_3904/ap_CS_fsm_state20` | 253 | `-2.835 ns` | `9.101 ns` | Worst path driver; ROB commit clock-enable/control fanout. |

Congestion summary from `report_congestion_6p450.rpt`:

| Section | Result |
|---|---|
| Placer final congestion | No congestion windows above level 5. |
| Router initial congestion | No effective congestion windows above level 5. |
| SLR crossing | Current part is not an SSI device. |

## Fresh 6.450 ns Implementation

The fresh implementation met timing while still showing a route-dominated worst path:

| Metric | Value |
|---|---:|
| Post-route WNS | `0.086 ns` |
| Post-route TNS | `0.000 ns` |
| Setup failing endpoints | `0` |
| Worst path data delay | `6.344 ns` |
| Worst path logic delay | `0.226 ns` |
| Worst path net delay | `6.118 ns` |
| Net delay share | `96.438%` |
| Logic levels | `1` |
| High fanout | `324` |

Fresh-route congestion also reported no congestion windows above level 5 and no effective router congestion windows above level 5.

## Conclusion

The old 6.450 ns failure was not caused by broad device congestion or excessive LUT/FF/BRAM/DSP utilization. It was a placement/routing outcome around high-fanout control and stream/FIFO nets. A fresh implementation from the synthesized checkpoint allowed the placer/router to find a legal 6.450 ns OOC solution, but the final system still needs a wrapper and board-level timing constraints before this can be treated as full-system timing closure.
