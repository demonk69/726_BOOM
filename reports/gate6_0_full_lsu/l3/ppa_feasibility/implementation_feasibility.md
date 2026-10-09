# Implementation Feasibility

## Attempted Flow

The `CONFIG_4_4` generated RTL from the committed R5 evidence was used for a Vivado implementation preflight.

| Item | Value |
|---|---|
| Tool | Vivado 2021.2 |
| Part | `xczu7ev-ffvc1156-2-e` |
| Top | `boom_core_top` |
| Work root | `/home/lab_726/opencode_tmp/g6_l3r5_impl_4_4_20261009_ppa1` |
| Tcl | `reports/gate6_0_full_lsu/l3/ppa_feasibility/config_4_4_impl.tcl` |
| Clock XDC | `/home/lab_726/opencode_tmp/g6_l3r5_impl_4_4_20261009_ppa1/g6_l3_4_4_clock.xdc` |

## Stage Status

| Stage | Status | Evidence |
|---|---|---|
| setup | PASS | `logs/4_4_impl.status` |
| synth_design | PASS | `logs/4_4_impl.status` |
| opt_design | PASS | `logs/4_4_impl.status` |
| place_design | FAIL | `ERROR: [Common 17-69] Command failed: Placer could not place all instances` |
| route_design | NOT RUN | placement failed |
| post-route timing | NOT AVAILABLE | placement failed |

## Synthesized Utilization

| Resource | Used | Available | Utilization |
|---|---:|---:|---:|
| CLB LUTs | 55482 | 230400 | 24.08% |
| CLB Registers | 35493 | 460800 | 7.70% |
| Block RAM Tile | 30 | 312 | 9.62% |
| RAMB36/FIFO | 13 | 312 | 4.17% |
| RAMB18 | 34 | 624 | 5.45% |
| DSP | 16 | 1728 | 0.93% |
| Bonded IOB | 1940 | 360 | 538.89% |
| Global clock buffers | 1 | 544 | 0.18% |

## Placement Failure

Vivado fails during IO placement:

```text
ERROR: [Place 30-415] IO Placement failed due to overutilization. This design contains 1940 I/O ports
 while the target  device: xczu7ev package: ffvc1156, contains only 719 available user I/O.
```

This is a raw HLS top-level integration issue, not a core LUT/FF capacity failure. The generated `boom_core_top` exposes wide streams, debug/commit trace, memory interfaces, and control ports directly as package I/O. A realistic implementation needs an integration wrapper, AXI/register slicing, pin reduction, or internal tie-off/packing strategy before physical placement and route timing can be assessed.

## Feasibility Conclusion

The 4_4 RTL is synthesizable and opt-design completes, but raw top-level implementation is not placeable on `xczu7ev-ffvc1156-2-e` because of I/O overutilization. Therefore implementation feasibility is `BLOCKED`, and post-route timing is unavailable.
