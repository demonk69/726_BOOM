# BOOM Verilator Trace Build Manifest

Date: 2026-07-24

## Confirmed Artifacts

| Item | Path / Value |
|---|---|
| Generated artifact root | `/home/lab_726/boom/chipyard.TestHarness.SmallBoomConfig` |
| Config name | `SmallBoomConfig` / `chipyard.TestHarness.SmallBoomConfig` |
| Generated Verilog | `/home/lab_726/boom/chipyard.TestHarness.SmallBoomConfig/chipyard.TestHarness.SmallBoomConfig.top.v` |
| Harness Verilog | `/home/lab_726/boom/chipyard.TestHarness.SmallBoomConfig/chipyard.TestHarness.SmallBoomConfig.harness.v` |
| FIRRTL | `/home/lab_726/boom/chipyard.TestHarness.SmallBoomConfig/chipyard.TestHarness.SmallBoomConfig.fir` |
| Top FIRRTL | `/home/lab_726/boom/chipyard.TestHarness.SmallBoomConfig/chipyard.TestHarness.SmallBoomConfig.top.fir` |
| Verilator model dir | `/home/lab_726/boom/chipyard.TestHarness.SmallBoomConfig/chipyard.TestHarness.SmallBoomConfig` |
| Verilator top header | `/home/lab_726/boom/chipyard.TestHarness.SmallBoomConfig/chipyard.TestHarness.SmallBoomConfig/VTestHarness.h` |
| Verilator archive | `/home/lab_726/boom/chipyard.TestHarness.SmallBoomConfig/chipyard.TestHarness.SmallBoomConfig/VTestHarness__ALL.a` |
| Emulator source | `/home/lab_726/boom/chipyard.TestHarness.SmallBoomConfig/emulator.cc` |
| Generated Makefile | `/home/lab_726/boom/chipyard.TestHarness.SmallBoomConfig/chipyard.TestHarness.SmallBoomConfig/VTestHarness.mk` |
| Reset vector | `0x10040` from `chipyard.TestHarness.SmallBoomConfig.dromajo_params.h` |
| Boot ROM images | `bootrom.rv32.img`, `bootrom.rv64.img` |
| TestHarness ports | `clock`, `reset`, `io_success` |

## Version Commands

| Command | Result |
|---|---|
| `git -C /home/lab_726/boom rev-parse --show-toplevel` | not a git repository |
| `git -C /home/lab_726/boom/chipyard.TestHarness.SmallBoomConfig rev-parse --show-toplevel` | not a git repository |
| `git -C /home/lab_726/boom/hls_boom rev-parse --show-toplevel` | not a git repository |
| `verilator --version` | `Verilator 5.030 2024-10-27 rev v5.030` |

## Commit Metadata

| Repository | Commit |
|---|---|
| BOOM | unavailable: original git checkout is absent |
| Chipyard | unavailable: original git checkout is absent |
| Rocket Chip | unavailable: original git checkout is absent |

## Runtime / Loading

- `emulator.cc` requires a positional `BINARY`; otherwise it exits with `No binary specified for emulator`.
- Standard Chipyard loading goes through FESVR/TSI, with optional `+loadmem=` support in `SimDRAM.cc`.
- No final `simulator-chipyard-SmallBoomConfig` binary exists under `/home/lab_726/boom`.
- No RISC-V ELF, `.riscv`, `.hex`, or proxy kernel was found in the workspace.

## Build Blockers

- Generated `VTestHarness.mk` hardcodes `/root/chipyard/...` paths; `/root/chipyard` is absent.
- `libfesvr` is not present under `/home/lab_726` or `/usr`.
- `libdramsim` is not present under `/home/lab_726` or `/usr`.
- `riscv64-unknown-elf-gcc` is not present under `/home/lab_726` or `/usr`.
- The original Chisel/Scala source tree is absent, so stable Chisel-level trace instrumentation cannot be rebuilt here.
