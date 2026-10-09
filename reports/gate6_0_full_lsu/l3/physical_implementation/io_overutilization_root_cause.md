# I/O Overutilization Root Cause

## Summary

The raw `boom_core_top` full implementation failed at placement because Vitis HLS exported wide AXIS payloads and scalar debug/status outputs as package-level pins. This is a top-level interface packaging problem, not evidence that the core logic exceeds LUT/FF/BRAM/DSP capacity.

Required fields:

| Field | Value |
|---|---:|
| `L3_PPA1_EVIDENCE_COMMIT` | `236733e80a1fd60851436c5b9fe066ff81b09ea7` |
| `L3_PHYS_IO_PORTS_REQUIRED` | 1940 |
| `L3_PHYS_IO_PORTS_AVAILABLE` | 719 |
| `L3_PRODUCT_CODE_MODIFIED` | `false` |
| `PPA_GATE_STATUS` | `UNRESOLVED` |

## Failure Evidence

The PPA1 raw-top implementation stopped in `place_design`:

| Evidence | Value |
|---|---|
| Status file | `reports/gate6_0_full_lsu/l3/ppa_feasibility/logs/4_4_impl.status` |
| Status | `place_design,FAIL,ERROR: [Common 17-69] Command failed: Placer could not place all instances` |
| Log file | `reports/gate6_0_full_lsu/l3/ppa_feasibility/logs/4_4_impl.log` |
| Vivado placer error | `ERROR: [Place 30-415] IO Placement failed due to overutilization. This design contains 1940 I/O ports` |
| Package/user-I/O line | `contains only 719 available user I/O` |

The synthesized raw-top utilization report also shows the same package-pin pressure before placement:

| Site Type | Used | Available | Utilization |
|---|---:|---:|---:|
| Bonded IOB | 1940 | 360 | 538.89% |

Vivado reports 719 available user I/O in the placer log and 360 bonded IOB sites in the synthesized utilization table. Both numbers show the same conclusion: the raw generated top cannot be mapped directly to package pins.

## Inventory

The package-pin relevant count is the synthesized I/O-buffer inventory exported from `/home/lab_726/opencode_tmp/g6_l3r5_ppa2_io_query_20261009/synth_iob_cells.csv`. It counts package-level `IBUF`/`INBUF` and `OBUF` cells and excludes internal logical port accounting artifacts.

| Category | Bits |
|---|---:|
| Clock/reset | 2 |
| Control/status scalar pins | 6 |
| IMEM AXIS pins | 421 |
| DMEM AXIS pins | 613 |
| Commit trace AXIS pins | 770 |
| Debug counters | 128 |
| Internal state exported as pins | 0 |
| Other | 0 |
| Total | 1940 |

The detailed row-level inventory is in `io_port_inventory.csv`.

## Interpretation

The root cause is the raw HLS top being treated as the device top. Its streams are structurally correct for HLS RTL integration, but when implemented directly Vivado must bind every payload bit and handshake signal to a package I/O resource.

The largest contributors are `commit_trace_out_TDATA` at 768 bits, `dmem_req_out_TDATA` at 320 bits, `dmem_resp_in_TDATA` at 289 bits, and `imem_resp_in_TDATA` at 225 bits. Those four payload buses alone account for 1602 package pins before handshake, control, clock/reset, and debug pins.

## Consequence

A physical wrapper or platform integration layer is required before full-system implementation can close. The wrapper must connect the generated core to internal fabric interfaces, DMA/NoC/AXI infrastructure, BRAM/URAM, or a narrower external packet interface instead of exporting the raw HLS stream payloads to package pins.
