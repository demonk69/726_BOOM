# OOC Implementation Evidence

## Summary

`CONFIG_4_4` can synthesize, place, and route out of context on `xczu7ev-ffvc1156-2-e` when package I/O insertion is suppressed. This proves that the PPA1 raw-top placement failure is driven by top-level I/O pressure. It does not close the L3 physical timing gate, because the routed design meets 10.000 ns but fails when the routed checkpoint is re-evaluated at the 6.450 ns L3 period limit.

| Field | Value |
|---|---|
| Evidence commit | `236733e80a1fd60851436c5b9fe066ff81b09ea7` |
| Config | `CONFIG_4_4` |
| Vivado | `2021.2` |
| Part | `xczu7ev-ffvc1156-2-e` |
| Top | `boom_core_top` |
| Script | `reports/gate6_0_full_lsu/l3/physical_implementation/ooc_4_4_impl.tcl` |
| 6.450 ns retiming helper | `reports/gate6_0_full_lsu/l3/physical_implementation/ooc_retime_6p450.tcl` |
| OOC build root | `/home/lab_726/opencode_tmp/g6_l3r5_ppa2_ooc_4_4_rerun_20261009` |
| Product source modified | `false` |
| PPA gate status | `UNRESOLVED` |

## PPA2 Audit Fields

| Field | Value |
|---|---|
| `OOC_ROUTE_4_4` | `PASS` |
| `OOC_IOBUF_COUNT` | `0` |
| `TIMING_10NS_WNS` | `0.715` |
| `TIMING_10NS_TNS` | `0.000` |
| `TIMING_6P450NS_WNS` | `-2.835` |
| `TIMING_6P450NS_TNS` | `-2065.701` |
| `TIMING_6P450NS_FAILING_ENDPOINTS` | `3410` |
| 6.450 ns report source | Existing 10 ns routed checkpoint re-opened by `ooc_retime_6p450.tcl`, then `reset_timing` and `create_clock -period 6.450` |

## Artifact Provenance

| Artifact | SHA-256 | Path / Method |
|---|---|---|
| RTL manifest | `b50e2d8fe57cc9813468ff52ce07c0919cd0a64c33645e9121dad1cf3c18fdeb` | SHA-256 over sorted per-file SHA-256 records for `*.v` and `*.dat` under `/home/lab_726/opencode_tmp/g6_l3r2_r5_preflight_4_4_20261009_155117/hls_project/solution/syn/verilog` |
| Routed DCP | `1e758939d3aebf4bdf54f212320e874a8226d1667e953382ff13e154466cdb91` | `/home/lab_726/opencode_tmp/g6_l3r5_ppa2_ooc_4_4_rerun_20261009/ooc_post_route.dcp` |
| OOC clock XDC | `44454f3b63f252f61cf5cf536e77fb6cdfe8e5147ec993e8f18e68b3550b0507` | `/home/lab_726/opencode_tmp/g6_l3r5_ppa2_ooc_4_4_rerun_20261009/g6_l3_4_4_ooc_clock.xdc` |

Tool version:

| Tool | Version |
|---|---|
| Vivado | `v2021.2 (64-bit), SW Build 3367213, IP Build 3369179` |
| Target part | `xczu7ev-ffvc1156-2-e` |

Implementation command sequence in `ooc_4_4_impl.tcl`:

| Stage | Command |
|---|---|
| Project setup | `create_project -force g6_l3_4_4_ooc ... -part xczu7ev-ffvc1156-2-e`; `read_verilog`; `set_property top boom_core_top`; `read_xdc` |
| OOC synthesis | `synth_design -top boom_core_top -part xczu7ev-ffvc1156-2-e -mode out_of_context` |
| Optimization | `opt_design` |
| Placement | `place_design` |
| Routing | `route_design` |
| 6.450 ns re-analysis | `open_checkpoint ooc_post_route.dcp`; `reset_timing`; `create_clock -period 6.450 -name ap_clk [get_ports ap_clk]`; `report_timing_summary`; `report_timing` |

## Stage Status

`/home/lab_726/opencode_tmp/g6_l3r5_ppa2_ooc_4_4_rerun_20261009/ooc_status.csv` records:

| Stage | Status |
|---|---|
| setup | PASS |
| synth_design_ooc | PASS |
| opt_design | PASS |
| place_design | PASS |
| route_design | PASS |
| implementation | COMPLETE |

Route status from `ooc_route_status.rpt`:

| Metric | Value |
|---|---:|
| Logical nets | 116859 |
| Routable nets | 78016 |
| Fully routed nets | 78016 |
| Nets with routing errors | 0 |

## I/O Isolation

OOC synthesis and implementation used no package I/O buffers:

| Stage | IBUF | INBUF | OBUF | IBUFCTRL |
|---|---:|---:|---:|---:|
| OOC synth | 0 | 0 | 0 | 0 |
| OOC place | 0 | 0 | 0 | 0 |
| OOC route | 0 | 0 | 0 | 0 |

The routed utilization report also reports `Bonded IOB = 0 / 360`.

## Routed Utilization

From `ooc_route_utilization.rpt`:

| Resource | Used | Available | Utilization |
|---|---:|---:|---:|
| CLB LUTs | 52730 | 230400 | 22.89% |
| CLB Registers | 35417 | 460800 | 7.69% |
| Block RAM Tile | 30 | 312 | 9.62% |
| RAMB36/FIFO | 13 | 312 | 4.17% |
| RAMB18 | 34 | 624 | 5.45% |
| DSPs | 16 | 1728 | 0.93% |
| Bonded IOB | 0 | 360 | 0.00% |

These are post-implementation Vivado resource numbers. They are not directly comparable to HLS C-synthesis estimates in `ppa_feasibility/five_config_csynth_matrix.csv`.

## Timing

At the generated 10.000 ns clock constraint, the routed OOC checkpoint meets setup and hold:

| Constraint | WNS | TNS | Setup failing endpoints | WHS | THS | Hold failing endpoints |
|---:|---:|---:|---:|---:|---:|---:|
| 10.000 ns | 0.715 ns | 0.000 ns | 0 | 0.026 ns | 0.000 ns | 0 |

The same routed checkpoint re-analyzed at the L3 6.450 ns period limit fails setup:

| Constraint | WNS | TNS | Setup failing endpoints | WHS | THS | Hold failing endpoints |
|---:|---:|---:|---:|---:|---:|---:|
| 6.450 ns | -2.835 ns | -2065.701 ns | 3410 | 0.026 ns | 0.000 ns | 0 |

The 6.450 ns critical path is in `grp_rob_commit_module_fu_3904`, from `ap_CS_fsm_reg[19]/C` to `ce_memory_data_reg_6312_reg[20]/CE`. The path has 9.180 ns data-path delay with 9.101 ns route delay, 99.139% route.

## DRC And Congestion

`ooc_route_drc.rpt` reports 45 warnings and no errors:

| Rule | Severity | Count | Meaning |
|---|---|---:|---|
| DPIP-2 | Warning | 24 | DSP input pipelining |
| DPOP-3 | Warning | 10 | DSP PREG output pipelining |
| DPOP-4 | Warning | 10 | DSP MREG output pipelining |
| RTSTAT-10 | Warning | 1 | No routable loads |

The RTSTAT-10 warning lists unconnected top-level outputs in OOC context, including `io_cycle[63:0]`, `io_instret[63:0]`, `io_success`, `io_trap`, and portions of the stream payloads. This is expected for an OOC run without a wrapper consumer.

`ooc_route_congestion.rpt` reports no congestion windows above level 5 and no effective router congestion windows above level 5.

## Conclusion

OOC routing is physically feasible after removing package I/O pressure. The remaining blockers are a real physical wrapper and timing closure against the 6.450 ns L3 period limit.
