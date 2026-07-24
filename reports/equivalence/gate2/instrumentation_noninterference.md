# Instrumentation Noninterference

Date: 2026-07-24

Status: OFFICIAL_BLOCKED_STANDALONE_ADAPTER_ONLY

## Official Check

The required official comparison is still blocked. The same BOOM program has not been run through both an uninstrumented and instrumented `simulator-chipyard-SmallBoomConfig` because the official simulator path does not build in this workspace.

First official build blocker:

```text
FIRST_BLOCKER: generated makefile target directory does not exist: /root/chipyard/sims/verilator
make: *** No rule to make target '/root/chipyard/sims/verilator/generated-src/chipyard.TestHarness.SmallBoomConfig/SimDRAM.cc', needed by 'SimDRAM.o'.  Stop.
```

## Standalone Adapter Check

Gate 2.5 verifies limited standalone-adapter noninterference only. The runner links the existing generated `VTestHarness__ALL.a`, samples generated fields once per full simulated cycle after reset, and keeps trace metadata, sequence IDs, architectural shadow state, and retired-store `tohost` detection in local C++ state.

This does not prove official noninterference because the standalone environment still supplies local DPI memory/serial/JTAG/UART stubs, wakes hart 0, disables assertion-stop handling, and does not use the original FESVR/DRAMSim emulator.

## Evidence Logs

- `reports/equivalence/gate2/build_boom_trace.log`
- `reports/equivalence/gate2/build_reference_programs.log`
- `reports/equivalence/gate2/check_boom_traces.log`
- `reports/equivalence/gate2_5/chipyard_build.log`
- `docs/verilator_commit_signal_proof.md`

No full official-emulator noninterference claim is made.
