# PPA4 System Top Architecture

Scope: Gate 6.0 L3R5-PPA4 experimental package-level integration for `CONFIG_4_4` on `xczu7ev-ffvc1156-2-e`.

PPA4 does not modify canonical product source. The wrapper and reports under this directory are experimental integration evidence.

## Inputs

| Item | Value |
| --- | --- |
| PPA3 evidence commit | `5a2ce0bc6b4f245747b9c0b4b13a1a2bfb6b5db6` |
| Generated RTL source | `/home/lab_726/opencode_tmp/g6_l3r2_r5_preflight_4_4_20261009_155117/hls_project/solution/syn/verilog` |
| HLS core top | `boom_core_top` |
| PPA4 system top | `boom_l3_ppa4_system_top` |
| PPA4 memory/trace adapter | `boom_l3_ppa4_memory_system` |
| Clock target | `6.450 ns` |
| Tool | Vivado 2021.2 build 3367213 |

## Architecture

`boom_l3_ppa4_system_top` instantiates the generated HLS `boom_core_top` and connects all raw HLS AXIS-style streams to an on-chip adapter. The raw stream payloads are not exposed as package pins.

| Raw HLS stream | PPA4 binding |
| --- | --- |
| `imem_req_out` | Consumed by on-chip IMEM responder. |
| `imem_resp_in` | Driven by on-chip IMEM responder with deterministic NOP instruction data. |
| `dmem_req_out` | Consumed by on-chip DMEM target with pending-response ownership tracking. |
| `dmem_resp_in` | Driven by on-chip DMEM response scheduler preserving request transaction IDs. |
| `commit_trace_out` | Consumed by on-chip trace receiver with bounded FIFO/backpressure. |

The adapter was compacted after the first synthesis attempt showed that byte-addressed synthetic arrays caused LUT overutilization. The current adapter uses a small store-forwarding buffer and trace-summary FIFO while retaining the protocol checks that matter for this package-level proof.

## External Interface

The wrapper exposes a narrow status/debug interface rather than the raw HLS streams:

| Port group | Direction | Width |
| --- | --- | ---: |
| `ap_clk` | input | 1 |
| `ap_rst_n` | input | 1 |
| core status flags | output | 4 |
| `io_cycle` | output | 64 |
| `io_instret` | output | 64 |
| adapter counters | output | 128 |
| adapter fault flags | output | 3 |
| Total package-level logical I/O | mixed | 265 |

## Provenance

| Artifact | SHA-256 |
| --- | --- |
| `rtl/boom_l3_ppa4_memory_system.v` | `e93bc70ecf0cc393fbe9be003ff3102acb93d5b04af2a1163a2e38eef0de037a` |
| `rtl/boom_l3_ppa4_system_top.v` | `75034bacd56202737e5868c35aaefcff699377e2d22d32019499381246dd0c21` |
| `ppa4_system_elab.tcl` | `7cc8d9735d59f20eddc90cc12e57cf063b9605571421a4c568d66e37fcfae940` |
| `ppa4_system_synth.tcl` | `ec9984a1b342b16cc154dc4b13458149d38352973d7d3ac8c91496e1742a6dcd` |
| `ppa4_system_impl.tcl` | `6a266785ecc3c6e2c986fa79bcbd1835a0bbcd5348a34853eb71cb053ace8b7b` |
| Adapter scoreboard TB | `c368a78d8db543bdcaa4b89671cc42f89e755a769b00b00de06fea52f30484d9` |

## Status

`L3_PPA4_SYSTEM_TOP_ELABORATION=PASS`

`L3_PPA4_SYSTEM_TOP_FUNCTIONAL_SIM=ADAPTER_SCOREBOARD_PASS_FULL_CORE_WORKLOAD_PENDING`

`L3_PPA4_SYSTEM_IMPLEMENTATION=ROUTE_PASS_BOARD_XDC_PENDING`

`PPA_GATE_STATUS=UNRESOLVED_BOARD_XDC_AND_FULL_SYSTEM_FUNCTIONAL_SIGNOFF_PENDING`
