# Physical Wrapper Feasibility

## Feasibility Statement

A physical wrapper is feasible and required. The routed OOC run proves the `CONFIG_4_4` core fabric can place and route when the raw HLS streams are not bound to package pins. The raw generated HLS top itself is not a package-level top because it requires 1940 package-level I/O pins on a part with 719 available user I/O pins.

This report does not claim final PPA acceptance. Timing at the L3 6.450 ns period remains open.

## Evidence

| Evidence | Result |
|---|---|
| Raw top placement | FAIL, I/O overutilization: 1940 required vs 719 available user I/O |
| OOC synthesis | PASS |
| OOC placement | PASS |
| OOC routing | PASS |
| OOC routed I/O buffers | 0 `IBUF`, 0 `INBUF`, 0 `OBUF`, 0 `IBUFCTRL` |
| OOC routed 10 ns timing | PASS, WNS 0.715 ns |
| OOC routed 6.450 ns timing | FAIL, WNS -2.835 ns, TNS -2065.701 ns |
| OOC route congestion | No windows above level 5 |
| Product source modified | `false` |

## Recommended Wrapper Direction

Use `boom_core_top` as an internal module, not as the FPGA package top. The package-level top should provide a narrow board/platform interface and instantiate or integrate the generated core behind fabric-resident adapters.

Recommended integration boundaries:

| Raw HLS interface | Wrapper direction |
|---|---|
| `imem_req_out` / `imem_resp_in` | Connect to internal instruction-memory adapter, cache model, AXI master bridge, or NoC endpoint |
| `dmem_req_out` / `dmem_resp_in` | Connect to internal data-memory adapter that preserves `transaction_id` and ready/valid backpressure |
| `commit_trace_out` | Route to an internal trace FIFO, BRAM capture buffer, debug bus, or optional synthesis-time product/debug partition |
| `io_success`, `io_halted`, `io_trap` | Compress into a small status register or management bus |
| `io_cycle`, `io_instret` | Expose through a status/register bus or debug snapshot path, not 128 dedicated pins |

## Interface Compression Impact

The raw package-pin inventory is dominated by streams and debug payloads:

| Group | Raw pins |
|---|---:|
| Commit trace | 770 |
| DMEM | 613 |
| IMEM | 421 |
| Debug counters | 128 |
| Clock/reset/control | 8 |
| Total | 1940 |

Internalizing the three AXIS stream groups and moving debug counters behind a register/debug bus would remove 1932 direct package pins, leaving only board-level clock/reset and a narrow platform interface chosen by the wrapper.

## Correctness Constraints

The wrapper must not change the core's internal LSU ownership protocol:

| Constraint | Rationale |
|---|---|
| Preserve AXIS ready/valid | `boom_core_cycle_io` checks stream emptiness/fullness each cycle before moving responses and requests |
| Preserve `DmemRequest.transaction_id` | Load responses match active load-queue owners by transaction ID |
| Allow out-of-order DMEM responses | R4/R2 traces demonstrate out-of-order response ownership and completion |
| Preserve reset isolation | The top suppresses normal operation until `reset_ctrl.completed` |
| Handle commit-trace backpressure | `rob_commit_module` returns when `pipe.commit_trace.full()` |

## Remaining Engineering Work

The minimal next implementation step is to add a non-product-source wrapper or integration top that consumes the generated core RTL internally and exposes a board-valid interface. After that, run full implementation with real I/O constraints and re-run timing at the accepted L3 clock period.

Timing work is also required. The OOC critical path at 6.450 ns is route-dominated in the ROB commit module. A wrapper alone may not fix that path; additional floorplanning, constraints, retiming, or microarchitectural changes may be needed after the physical top exists.

## Status

`PPA_GATE_STATUS=UNRESOLVED`

`L3_PRODUCT_CODE_MODIFIED=false`
