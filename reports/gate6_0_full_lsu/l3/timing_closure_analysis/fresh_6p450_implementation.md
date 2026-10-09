# Fresh 6.450 ns OOC Implementation

## Summary

PPA3 reran implementation from the PPA2 OOC post-synthesis checkpoint with the 6.450 ns L3 clock applied before optimization, placement, physical optimization, and routing. Unlike the PPA2 re-analysis of an existing 10 ns route, this fresh 6.450 ns implementation met timing.

| Field | Value |
|---|---|
| PPA2 evidence commit | `cf4d4ab59f3150896ecd8fd2b6a2af2f87c732bc` |
| Script | `reports/gate6_0_full_lsu/l3/timing_closure_analysis/ppa3_fresh_6p450_impl.tcl` |
| Input checkpoint | `/home/lab_726/opencode_tmp/g6_l3r5_ppa2_ooc_4_4_rerun_20261009/ooc_post_synth.dcp` |
| Output root | `/home/lab_726/opencode_tmp/g6_l3r5_ppa3_fresh_6p450_4_4_20261009` |
| Routed checkpoint | `/home/lab_726/opencode_tmp/g6_l3r5_ppa3_fresh_6p450_4_4_20261009/fresh_post_route.dcp` |
| RTL manifest SHA-256 | `b50e2d8fe57cc9813468ff52ce07c0919cd0a64c33645e9121dad1cf3c18fdeb` |
| Post-synth netlist DCP SHA-256 | `94c62ad0095256822985f92e2ec03acd6ff9ce47ffb2b67e69e695b3578ae5b2` |
| OOC clock XDC SHA-256 | `44454f3b63f252f61cf5cf536e77fb6cdfe8e5147ec993e8f18e68b3550b0507` |
| Routed checkpoint SHA-256 | `7adb1fb683743830c0e1e97c6c98fd5ba6d92c33fe1735c6f36f6d0667d73377` |
| Post-route timing summary SHA-256 | `5c848ceaf2c07264ab1f20b18db564735030bf33264ea84898158f1c218f8eb5` |
| Clock | `ap_clk`, `6.450 ns`, `{0.000 3.225}` |
| Tool | Vivado `2021.2`, SW Build `3367213`, IP Build `3369179` |
| Part | `xczu7ev-ffvc1156-2-e` |
| Top | `boom_core_top` |
| Product source modified | `false` |

## Implementation Commands

`ppa3_fresh_6p450_impl.tcl` performs the following command sequence:

| Stage | Commands |
|---|---|
| Open and constrain | `open_checkpoint $G6_L3_PPA3_SYNTH_DCP`; `reset_timing`; `create_clock -period 6.450 -name ap_clk [get_ports ap_clk]`; `report_clocks`; `check_timing` |
| Optimize | `opt_design`; `report_utilization`; `report_timing_summary`; `write_checkpoint -force fresh_post_opt.dcp` |
| Place | `place_design`; `report_utilization`; `report_timing_summary`; `report_design_analysis -congestion`; `report_drc`; `write_checkpoint -force fresh_post_place.dcp` |
| Physical optimize | `phys_opt_design`; `report_utilization`; `report_timing_summary`; `report_design_analysis -congestion`; `report_drc`; `write_checkpoint -force fresh_post_phys_opt.dcp` |
| Route | `route_design`; `report_route_status`; `report_utilization`; `report_timing_summary -max_paths 100`; `report_timing -max_paths 20 -sort_by group`; `report_design_analysis -congestion`; `report_design_analysis -timing -setup`; `report_drc`; `write_checkpoint -force fresh_post_route.dcp` |

## Stage Status

From `fresh_6p450_status.csv`:

| Stage | Status |
|---|---|
| Open synth checkpoint | PASS |
| `opt_design` | PASS |
| `place_design` | PASS |
| `phys_opt_design` | PASS |
| `route_design` | PASS |
| Implementation | COMPLETE |

Route status from `fresh_route_status.rpt`:

| Metric | Value |
|---|---:|
| Logical nets | 116858 |
| Routable nets | 77984 |
| Fully routed nets | 77984 |
| Nets with routing errors | 0 |

## Timing

Post-route timing from `fresh_post_route_timing_summary.rpt`:

| Constraint | WNS | TNS | Setup failing endpoints | WHS | THS | Hold failing endpoints |
|---:|---:|---:|---:|---:|---:|---:|
| 6.450 ns | `0.086 ns` | `0.000 ns` | 0 | `0.025 ns` | `0.000 ns` | 0 |

Stage timing progression:

| Stage | WNS | TNS | Setup failing endpoints |
|---|---:|---:|---:|
| Post-place | `0.509 ns` | `0.000 ns` | 0 |
| Post-phys-opt | `0.509 ns` | `0.000 ns` | 0 |
| Post-route | `0.086 ns` | `0.000 ns` | 0 |

Vivado reported: `All user specified timing constraints are met.`

## Fresh Critical Path

Worst post-route setup path from `fresh_post_route_critical_path.rpt`:

| Metric | Value |
|---|---|
| Slack | `0.086 ns` |
| Source | `pipe_3_fifo_U/mOutPtr_reg[1]/C` |
| Destination | `grp_boom_core_cycle_io_fu_6197/grp_completion_service_cycle_fu_2363/response_read_data_reg_12971_reg[8]/D` |
| Path group | `ap_clk` |
| Requirement | `6.450 ns` |
| Data path delay | `6.344 ns` |
| Logic delay | `0.226 ns` |
| Route delay | `6.118 ns` |
| Route-delay share | `96.438%` |
| Logic levels | `1` |
| Launch net fanout | `324` |

The critical path changed from the PPA2 re-analysis ROB commit CE path to a completion-service response FIFO read-data path. It is still routing-dominated, but it meets the target period in the fresh 6.450 ns route.

## Utilization

From `fresh_post_route_utilization.rpt`:

| Resource | Used | Available | Utilization |
|---|---:|---:|---:|
| CLB LUTs | 52806 | 230400 | 22.92% |
| CLB Registers | 35419 | 460800 | 7.69% |
| Block RAM Tile | 30 | 312 | 9.62% |
| RAMB36/FIFO | 13 | 312 | 4.17% |
| RAMB18 | 34 | 624 | 5.45% |
| DSPs | 16 | 1728 | 0.93% |
| Bonded IOB | 0 | 360 | 0.00% |

The zero IOB count confirms this is still an OOC result with package I/O insertion suppressed.

## DRC And Congestion

`fresh_post_route_drc.rpt` reports `45` warnings and no errors:

| Rule | Severity | Count | Meaning |
|---|---|---:|---|
| `DPIP-2` | Warning | 24 | DSP input pipelining. |
| `DPOP-3` | Warning | 10 | DSP PREG output pipelining. |
| `DPOP-4` | Warning | 10 | DSP MREG output pipelining. |
| `RTSTAT-10` | Warning | 1 | No routable loads in OOC context. |

`fresh_post_route_congestion.rpt` reports no congestion windows above level 5 and no effective router congestion windows above level 5.

## Conclusion

The fresh 6.450 ns OOC implementation closes timing for `CONFIG_4_4`. This resolves the specific PPA2 question of whether a fresh implementation attempt can meet the L3 target in OOC mode. It does not resolve full-system PPA because no package-valid wrapper/top has been implemented or routed.

| Field | Value |
|---|---|
| `L3_PPA3_FRESH_6P450_PLACE` | `PASS` |
| `L3_PPA3_FRESH_6P450_ROUTE` | `PASS` |
| `L3_PPA3_FRESH_6P450_WNS` | `0.086` |
| `L3_PPA3_FRESH_6P450_TNS` | `0.000` |
| `L3_PPA3_FRESH_6P450_FAILING_ENDPOINTS` | `0` |
| `L3_PPA3_PRODUCT_CODE_MODIFIED` | `false` |
