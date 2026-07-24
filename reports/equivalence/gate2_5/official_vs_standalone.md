# Official Vs Standalone

Date: 2026-07-24

Status: BLOCKED_OFFICIAL_SIMULATOR_UNAVAILABLE

The official `simulator-chipyard-SmallBoomConfig` path did not build. The generated `VTestHarness.mk` target is `/root/chipyard/sims/verilator/simulator-chipyard-SmallBoomConfig`, but `/root/chipyard/sims/verilator` and the generated source inputs under `/root/chipyard/sims/verilator/generated-src/...` are unavailable.

First build blocker from `reports/equivalence/gate2_5/chipyard_build.log`:

```text
FIRST_BLOCKER: generated makefile target directory does not exist: /root/chipyard/sims/verilator
make: *** No rule to make target '/root/chipyard/sims/verilator/generated-src/chipyard.TestHarness.SmallBoomConfig/SimDRAM.cc', needed by 'SimDRAM.o'.  Stop.
```

No official-vs-standalone commit comparison was run. Standalone traces may be used only as loadmem-backed generated-model evidence until the official simulator path is restored.
