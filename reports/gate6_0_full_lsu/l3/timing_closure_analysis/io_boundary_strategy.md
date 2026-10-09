# PPA3 I/O Boundary Strategy

## Summary

The raw HLS `boom_core_top` cannot be used directly as the package top. Vivado placement failed previously because the raw top exports `1940` package-level I/O ports while the target package reports `719` available user I/O and `360` bonded IOB sites in utilization. PPA3 did not change product source and did not implement a wrapper; this report defines the required wrapper strategy for the next closure step.

| Field | Value |
|---|---|
| Raw product top | `boom_core_top`, `src/boom_core_top.cpp:99` |
| Step/test top | `boom_core_step_top`, `src/boom_core_top.cpp:220` |
| Test-only PF top | `boom_core_pf4_rtl_top`, `src/boom_core_top.cpp:156` |
| Raw I/O required | 1940 package pins |
| Available user I/O | 719 package pins |
| Product source modified | `false` |

## Raw Interface Drivers

`boom_core_top` exposes five HLS AXIS streams and six scalar status/debug outputs:

| Interface | Direction | Source | Payload |
|---|---|---|---|
| `imem_req_out` | Output | `src/boom_core_top.cpp:99` | `ImemRequest` |
| `imem_resp_in` | Input | `src/boom_core_top.cpp:100` | `ImemResponse` |
| `dmem_req_out` | Output | `src/boom_core_top.cpp:101` | `DmemRequest` |
| `dmem_resp_in` | Input | `src/boom_core_top.cpp:102` | `DmemResponse` |
| `commit_trace_out` | Output | `src/boom_core_top.cpp:103` | `CommitEntry` |
| `io_success`, `io_halted`, `io_trap`, `io_cycle_valid`, `io_cycle`, `io_instret` | Outputs | `src/boom_core_top.cpp:104` through `src/boom_core_top.cpp:109` | scalar status/debug |

The AXIS pragmas are at `src/boom_core_top.cpp:111` through `src/boom_core_top.cpp:115`. The scalar `ap_none` pragmas are at `src/boom_core_top.cpp:116` through `src/boom_core_top.cpp:121`.

The largest payload sources are defined in `include/boom_types.hpp`:

| Payload | Source | Relevant fields |
|---|---|---|
| `CommitEntry` | `include/boom_types.hpp:239` | PC, instruction, rd, privilege, rd value, exception, memory/store fields. |
| `ImemRequest` | `include/boom_types.hpp:252` | address, fetch ID, epoch, kill. |
| `ImemResponse` | `include/boom_types.hpp:260` | address, fetch ID, epoch, instruction, exception/cause. |
| `DmemRequest` | `include/boom_types.hpp:270` | transaction ID, ROB index, command, size, masks, epoch, address/data. |
| `DmemResponse` | `include/boom_types.hpp:280` | transaction ID, data/read data, exception/cause. |

The raw I/O inventory from PPA2 classified the `1940` package pins as:

| Category | Bits |
|---|---:|
| Clock/reset | 2 |
| Control/status scalar pins | 6 |
| IMEM AXIS pins | 421 |
| DMEM AXIS pins | 613 |
| Commit trace AXIS pins | 770 |
| Debug counters | 128 |
| Total | 1940 |

## Required Wrapper Semantics

A deployable top must internalize the wide HLS streams instead of binding every payload bit to package pins. The wrapper must preserve these semantics:

1. Preserve reset sequencing for `static BoomCoreState state` and `static ResetControllerState reset_ctrl` in `src/boom_core_top.cpp:123` through `src/boom_core_top.cpp:130`.
2. Preserve ready/valid backpressure on IMEM, DMEM, and commit trace streams; `boom_core_cycle_io` reads responses only when input streams are non-empty and writes requests/traces only through streams at `src/boom_core_top.cpp:51` through `src/boom_core_top.cpp:63`.
3. Preserve `DmemRequest.transaction_id` and `DmemResponse.transaction_id` end to end. Loads allocate transaction IDs at `src/lsu.cpp:158` through `src/lsu.cpp:169`, record ownership at `src/lsu.cpp:201` through `src/lsu.cpp:225`, and validate responses at `src/completion.cpp:81` through `src/completion.cpp:137` and `src/completion.cpp:167` through `src/completion.cpp:203`.
4. Preserve commit-trace ordering or define a product build mode that intentionally removes/internalizes trace output without changing commit behavior. Commit code checks `pipe.commit_trace.full()` before retiring trace-producing commits at `src/commit.cpp:200` and `src/commit.cpp:219`, then writes traces at `src/commit.cpp:208` and `src/commit.cpp:283`.
5. Do not constant-tie valid request paths, drop responses, rewrite transaction ownership, or remove LSU functionality to reduce pins.

## Recommended Physical Boundary

Use a real platform wrapper around the generated core with internal fabric connections:

| Boundary | Strategy |
|---|---|
| IMEM request/response | Bridge HLS streams to an internal AXI4, AXI4-Stream, BRAM, cache, NoC, or PS-PL fabric endpoint. Keep request/response queues inside PL fabric. |
| DMEM request/response | Bridge to an internal memory subsystem that carries transaction IDs unchanged and supports backpressure. |
| Commit trace | Route to an internal trace FIFO, BRAM/URAM ring buffer, DMA stream, or a debug-only build option. Do not export the 768-bit raw payload to package pins. |
| Status/debug counters | Expose through a narrow AXI-Lite/status register file or integrated debug bus. Do not export 128 bits of counters as raw pins unless the board budget explicitly allows it. |
| External package I/O | Limit to clock/reset and a board-appropriate narrow host/memory/debug interface with timing constraints. |

## Non-Strategies

The following approaches would not be valid PPA closure:

| Approach | Reason |
|---|---|
| Use raw `boom_core_top` as package top | Reproduces the `1940` vs `719` I/O overutilization. |
| Use `boom_core_step_top` as package top | It has the same raw stream/status interface and does not solve package I/O pressure. |
| Use `boom_core_pf4_rtl_top` as product top | It is explicitly test-only and adds a predictor seed interface. |
| Tie `TREADY` high and ignore real consumers | Can break backpressure, trace ordering, and memory correctness. |
| Drop `DmemResponse` or rewrite transaction IDs | Violates outstanding-load ownership and completion validation. |

## Conclusion

The feasible system I/O strategy is a wrapper/platform integration layer that keeps the BOOM core streams inside FPGA fabric and exposes only a board-valid narrow host/memory/debug interface. PPA3 provides OOC timing evidence for the core fabric, but a full-system pass requires implementing and routing that wrapper.

| Field | Value |
|---|---|
| `L3_PPA3_SYSTEM_IO_STRATEGY` | `REQUIRES_PACKAGE_VALID_WRAPPER_WITH_INTERNAL_AXI_OR_MEMORY_FABRIC` |
| `L3_PPA3_PRODUCT_CODE_MODIFIED` | `false` |
