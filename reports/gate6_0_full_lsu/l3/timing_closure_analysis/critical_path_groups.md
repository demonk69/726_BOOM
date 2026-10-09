# PPA3 Critical Path Groups

## Summary

The PPA3 audit extracted the top 100 setup paths from the PPA2 routed checkpoint after applying the 6.450 ns target clock. The old routed checkpoint fails timing with `WNS=-2.835 ns`, `TNS=-2065.701 ns`, and `3410` setup failing endpoints.

| Field | Value |
|---|---|
| Input checkpoint | `/home/lab_726/opencode_tmp/g6_l3r5_ppa2_ooc_4_4_rerun_20261009/ooc_post_route.dcp` |
| Extracted CSV | `/home/lab_726/opencode_tmp/g6_l3r5_ppa3_timing_audit_20261009/critical_paths_100.csv` |
| Copied report CSV | `reports/gate6_0_full_lsu/l3/timing_closure_analysis/critical_paths.csv` |
| Clock | `ap_clk`, `6.450 ns` |
| Path count | 100 setup paths |

## Group Counts

| Path class | Count in top 100 | Worst rank | Worst slack | Notes |
|---|---:|---:|---:|---|
| `ROB_COMMIT` | 68 | 1 | `-2.835 ns` | Dominates the worst failing region. The top 66 paths are all ROB commit clock-enable/control fanout paths. |
| `OTHER` | 32 | 67 | `-1.641 ns` | Mostly `state_brupdate_jalr_target_reg[...]` enable/data paths sourced from `state_rob_entries_uop_imm_packed_4_U/q0_reg[0]/C`. |

## Worst Path

| Metric | Value |
|---|---|
| Startpoint | `grp_boom_core_cycle_io_fu_6197/grp_rob_commit_module_fu_3904/ap_CS_fsm_reg[19]/C` |
| Endpoint | `grp_boom_core_cycle_io_fu_6197/grp_rob_commit_module_fu_3904/ce_memory_data_reg_6312_reg[20]/CE` |
| Module hierarchy | `grp_boom_core_cycle_io_fu_6197/grp_rob_commit_module_fu_3904` |
| Path class | `ROB_COMMIT` |
| Path group | `ap_clk` |
| Requirement | `6.450 ns` |
| Slack | `-2.835 ns` |
| Data path delay | `9.180 ns` |
| Logic delay | `0.079 ns` |
| Net delay | `9.101 ns` |
| Net-delay share | `99.139%` |
| Logic levels on worst path | `0` |
| Maximum logic levels in top 100 | `6` |
| Launch fanout | `253` |

## Interpretation

The old 10 ns route failed the 6.450 ns target because of route-dominated high-fanout control/clock-enable paths, not deep combinational logic. The single worst path is a ROB commit FSM state path driving clock-enable loads. The top 100 also contain branch-update target paths with up to 6 logic levels, but those are less severe than the ROB commit control fanout.

| Field | Value |
|---|---|
| `L3_PPA3_CRITICAL_PATH_MODULE` | `grp_boom_core_cycle_io_fu_6197/grp_rob_commit_module_fu_3904` |
| `L3_PPA3_CRITICAL_PATH_CLASS` | `ROB_COMMIT` |
| `L3_PPA3_MAX_LOGIC_LEVELS` | `6` |
| `L3_PPA3_ROUTING_DELAY_DOMINANT` | `true` |
