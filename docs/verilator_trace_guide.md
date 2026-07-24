# Verilator Trace Guide

Status: corrected standalone generated-model BOOM traces are available and checked in loadmem mode; full original Chipyard emulator trace generation remains blocked in this workspace.

## Paths

- Generated BOOM artifacts: `/home/lab_726/boom/chipyard.TestHarness.SmallBoomConfig`
- Trace schema: `reference/verilator_trace/trace_schema.json`
- Trace patch: `patches/boom_equivalence_trace.patch`
- Trace output directory: `reference/boom_traces/`
- Gate 2 logs: `reports/equivalence/gate2/`

## Standalone Commands

```sh
cd /home/lab_726/boom/hls_boom
scripts/build_reference_programs.sh
scripts/build_boom_trace.sh
scripts/run_boom_trace.sh --loadmem tb/programs/boom_reference/build/independent_alu.hex --output reference/boom_traces/independent_alu_commit.jsonl --max-cycles 3000
scripts/check_boom_trace.sh reference/boom_traces/independent_alu_commit.jsonl
```

Repeat `scripts/run_boom_trace.sh` and `scripts/check_boom_trace.sh` for `raw_chain.hex`, `branch_taken.hex`, `branch_not_taken.hex`, and `nested_branch.hex` to reproduce the current Gate 2 trace set.

## Current Standalone Evidence

| Trace | Status |
|---|---|
| `reference/boom_traces/independent_alu_commit.jsonl` | PASS_LOADMEM: 109 events, 40 commits, `tohost` termination |
| `reference/boom_traces/raw_chain_cycle.jsonl` | PASS_LOADMEM: 109 events, 40 commits, `tohost` termination |
| `reference/boom_traces/branch_taken_cycle.jsonl` | PASS_LOADMEM: 114 events, 41 commits, `tohost` termination |
| `reference/boom_traces/branch_not_taken_cycle.jsonl` | PASS_LOADMEM: 114 events, 42 commits, `tohost` termination |
| `reference/boom_traces/nested_branch_cycle.jsonl` | PASS_LOADMEM: 120 events, 42 commits, `tohost` termination |

## Required Environment

- Original Chipyard/BOOM source tree or a generated tree with reusable local paths.
- RISC-V toolchain providing `riscv64-unknown-elf-gcc` and `riscv64-unknown-elf-objdump`.
- FESVR install with `lib/libfesvr.a`; set `RISCV` to its install prefix.
- DRAMSim2 build with `libdramsim.a`; set `DRAMSIM_HOME` to its build directory.
- Final simulator build must be possible without hardcoded missing `/root/chipyard` paths.

## Full Official Emulator Blockers

- No final BOOM `simulator-chipyard-SmallBoomConfig` binary exists.
- Generated `VTestHarness.mk` references `/root/chipyard`, which is absent.
- `libfesvr`, `libdramsim`, and the RISC-V cross compiler are absent.
- Commit instruction and commit rd value are not exposed as stable public signals in the current generated model.

## Equivalence Rule

Standalone traces may be used only as loadmem-backed evidence from the generated BOOM `VTestHarness` model. They may support provisional architectural commit comparison and exploratory event-order comparison. Do not claim strict Cycle Equivalence or official BOOM Cycle Equivalence until the HLS model has been compared against checked BOOM traces and the remaining official-emulator/noninterference limitations are resolved.
