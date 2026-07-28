# Gate 3.7 C/RTL Cosim Feasibility

## Tool Availability

| Tool | Status |
|---|---|
| Vitis HLS | 2021.2 build 3367213 installed |
| Vivado XSim | 2021.2 installed and executable |
| `xelab` / `xvlog` | installed |
| Icarus Verilog | installed |
| Verilator | installed |

RTL simulator absence is not the blocker.

## Existing Flow Audit

The existing `scripts/run_cosim.tcl` selects `boom_core_top`, but `tb/tb_boom_core.cpp` calls `boom_core_step` on a test-local state and never invokes the selected top. The prefix trace testbench has the same property. Existing CSim passes therefore verify the step C model, not the free-running top RTL.

No previous `*_cosim.rpt`, automatic HLS RTL testbench, or completed C/RTL cosim project exists in the workspace.

Automatic cosim of `boom_core_top` is not practical as written:

- it is `ap_ctrl_none`;
- it contains an infinite loop and never returns;
- there is no C transaction boundary for reset or bounded checking;
- scalar product outputs are reported as dangling/reversed in accepted generated RTL;
- pin-level AXIS `TREADY` sequences are not expressible in the current queue-based C testbench.

## Required Finite Wrapper

A pipeline synthesis candidate would require a finite `ap_ctrl_hs` wrapper that calls the existing `boom_core_cycle_io` once per invocation, retains one static resettable `BoomCoreState`, uses the product AXIS payloads, and exposes valid-qualified scalar outputs. It may not copy the core implementation or introduce a second state inside one selected design.

No wrapper was added in Gate 3.7 because P1 produced no synthesized pipeline candidate. Building a verification wrapper without candidate RTL would not validate pipeline iteration overlap.

## Runtime Reset

Native tests reset C++ state by assignment; they do not toggle generated `ap_rst_n`. Existing generated RTL uses power-on initialization for many state fields. Direct RTL inspection shows only selected scalars with explicit reset branches, and generated state RAM modules expose but do not behaviorally use reset ports.

Consequently:

- conservative source reset directive: retained;
- pipeline source reset directive: retained in attempted synthesis;
- conservative RTL mid-run whole-state reset: `NOT_VERIFIED_AND_POTENTIALLY_INCOMPLETE`;
- pipeline RTL mid-run reset: `NOT_RUN_NO_PIPELINE_RTL`.

This is an acceptance blocker independent of pipeline PPA.

## Stream Backpressure

Current C tests exercise delayed responses, not pin-level output `TREADY=0`. Product output writes are blocking and ordered IMEM request, DMEM request, then commit trace. A custom XSim testbench must independently stall each output and verify that no later state transition, request, commit, cycle counter, or instret update passes a blocked earlier iteration.

Pipeline RTL backpressure status: `NOT_RUN_NO_PIPELINE_RTL`.

## Candidate Status

- `SYNTHESIS_CANDIDATE`: none
- `FUNCTIONALLY_VERIFIED_CANDIDATE`: none
- `C/RTL_COSIM_STATUS`: `NOT_RUN_NO_PIPELINE_RTL`
- `MID_RUN_RESET_STATUS`: `NOT_VERIFIED`
- `STREAM_BACKPRESSURE_STATUS`: `NOT_VERIFIED_AT_RTL`
- `READY_FOR_ACCEPT_PIPELINED_CONFIG=false`
