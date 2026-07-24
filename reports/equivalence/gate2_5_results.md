# Gate 2.5 Results

Date: 2026-07-24

Gate 2.5: PARTIAL PASS - loadmem-backed standalone generated-model traces

READY_FOR_PROVISIONAL_GATE_3=true

READY_FOR_GATE_3=false

## Verdict

The previous standalone traces were semantically invalid as complete program traces: the hand-encoded fallback images ended in `jal x0,0`, the runner had no working termination detector, and every trace stopped only at fixed `--max-cycles 3000`. That caused the suspicious ~1,800 commits.

Gate 2.5 fixed the fallback images and standalone runner. The five regenerated traces now terminate by a real retired store to configured `tohost` address `0x80000080`, return `exit_code=0`, and do not reach max-cycles. They are suitable for provisional architectural commit comparison and exploratory event-order comparison against the generated BOOM model.

They are not sufficient for strict Gate 3, strict Cycle Equivalence, or official BOOM Cycle Equivalence because ELF/binutils, the full Chipyard/FESVR/DRAMSim simulator, and official noninterference remain blocked.

## Root Cause Of ~1800 Commits

- Previous fallback images executed a finite prefix and then retired the same self-loop `jal x0,0` until `--max-cycles 3000`.
- The old traces were fixed-cycle truncated traces: `TRUNCATED_TRACE=true` for the old evidence.
- The commit signal itself was not a repeated-eval duplicate; it was repeatedly observing real BOOM retirement of the self-loop instruction.
- The new traces contain no fixed-length final commit loop and all have `max_cycles_reached=false`.

## Program And Trace Status

| Program | Old commits | New commits | Events | Termination | Tohost | Max-cycle truncated | Status |
|---|---:|---:|---:|---|---|---|---|
| `independent_alu` | 1,862 | 40 | 109 | `tohost` via `retired_store` | `0x1` | false | PASS_LOADMEM_ELF_BLOCKED |
| `raw_chain` | 1,862 | 40 | 109 | `tohost` via `retired_store` | `0x1` | false | PASS_LOADMEM_ELF_BLOCKED |
| `branch_taken` | 1,850 | 41 | 114 | `tohost` via `retired_store` | `0x1` | false | PASS_LOADMEM_ELF_BLOCKED |
| `branch_not_taken` | 1,862 | 42 | 114 | `tohost` via `retired_store` | `0x1` | false | PASS_LOADMEM_ELF_BLOCKED |
| `nested_branch` | 1,838 | 42 | 120 | `tohost` via `retired_store` | `0x1` | false | PASS_LOADMEM_ELF_BLOCKED |

See `reports/equivalence/gate2_5/program_manifest.csv` and `reports/equivalence/gate2_5/trace_statistics.csv`.

## ELF And Loadmem Result

- ELF files are unavailable: no `*.elf` exists under `/home/lab_726/boom`.
- Required commands were invoked and failed because the tools are missing: `riscv64-unknown-elf-readelf`, `riscv64-unknown-elf-objdump`, and `riscv64-unknown-elf-nm`. See `reports/equivalence/gate2_5/riscv_toolchain_commands.log`.
- Current evidence is loadmem-backed, not ELF-backed.
- `scripts/validate_trace_against_elf.py` passed in `--loadmem` mode for all five traces and wrote per-program reports under `reports/equivalence/gate2_5/*_elf_check.md`.

## Trace Semantic Checks

- Correct program load: PASS for fallback loadmem images at `0x80000000`.
- ELF correspondence: BLOCKED because ELF/binutils are absent.
- First loaded commits match the hand-encoded instruction map.
- Branch taken/not-taken behavior differs as expected: `branch_taken` skips the `0x8000000c` instruction and records a taken mispredict at `0x80000008`; `branch_not_taken` commits `0x8000000c` and records not-taken at `0x80000008`.
- `nested_branch` records two taken branch resolves at loaded-program PCs and then reaches the expected pass store.
- RAW chain records dependent ALU results `x1=5`, `x2=7`, `x3=11` before the pass store.
- No cycle rollback was detected.
- No final repeated commit loop remains.
- Boot ROM commits are still present before the jump to `0x80000000`; boot instruction encodings remain `null` because they are not in the loadmem image.
- Trap loop was not observed in the fixed traces.

See `reports/equivalence/gate2_5/trace_semantic_analysis.md`.

## Commit Signal Proof

Conclusion: `commit` records are sourced from BOOM ROB lane-0 retirement for the generated `VTestHarness` model.

The runner records only when `rob_io_commit_valids_0` is true. The generated Verilog assigns that signal as `can_commit_0 & ~can_throw_exception_0 & ~block_commit`, where `can_commit_0` includes ROB head valid, ROB head not busy, and no CSR stall. The runner samples once per full simulated cycle after reset and has a same-cycle duplicate guard.

See `docs/verilator_commit_signal_proof.md`.

## Official Simulator Recovery

Status: BLOCKED

The diagnostic script found Java, Verilator, conda, and the generated model artifacts. It did not find Chipyard source, RISC-V ELF toolchain, `libfesvr`, `libdramsim`, sbt, mill, Spike, or a final official simulator.

First official build blocker:

```text
FIRST_BLOCKER: generated makefile target directory does not exist: /root/chipyard/sims/verilator
make: *** No rule to make target '/root/chipyard/sims/verilator/generated-src/chipyard.TestHarness.SmallBoomConfig/SimDRAM.cc', needed by 'SimDRAM.o'.  Stop.
```

See `reports/equivalence/gate2_5/chipyard_environment.txt`, `reports/equivalence/gate2_5/chipyard_dependency_matrix.csv`, and `reports/equivalence/gate2_5/chipyard_build.log`.

## Noninterference

Official noninterference: BLOCKED

The official non-traced vs traced simulator comparison cannot run until the official Chipyard simulator path is restored. Standalone noninterference remains limited to the C++ trace adapter: it reads generated model fields and maintains local trace/metadata state, but it does not prove official emulator noninterference.

## Gate 2 Final Conclusion

Gate 2 remains PARTIAL PASS, now with corrected standalone semantics.

READY_FOR_PROVISIONAL_GATE_3=true means only the following are allowed:

- Architectural commit comparison against the fixed loadmem-backed standalone traces.
- Event-order exploratory comparison using ROB allocate, writeback, branch resolve, flush, commit, metadata, and tohost events.

The following remain disallowed:

- Strict Cycle Equivalence.
- Official BOOM Cycle Equivalence.
- PPA/timing optimization based on these traces.
- Claiming official Chipyard/FESVR/DRAMSim equivalence.

## Reproducible Commands

```sh
cd /home/lab_726/boom/hls_boom
scripts/build_reference_programs.sh
scripts/build_boom_trace.sh
scripts/run_boom_trace.sh --loadmem tb/programs/boom_reference/build/independent_alu.hex --output reference/boom_traces/independent_alu_commit.jsonl --max-cycles 3000
scripts/run_boom_trace.sh --loadmem tb/programs/boom_reference/build/raw_chain.hex --output reference/boom_traces/raw_chain_cycle.jsonl --max-cycles 3000
scripts/run_boom_trace.sh --loadmem tb/programs/boom_reference/build/branch_taken.hex --output reference/boom_traces/branch_taken_cycle.jsonl --max-cycles 3000
scripts/run_boom_trace.sh --loadmem tb/programs/boom_reference/build/branch_not_taken.hex --output reference/boom_traces/branch_not_taken_cycle.jsonl --max-cycles 3000
scripts/run_boom_trace.sh --loadmem tb/programs/boom_reference/build/nested_branch.hex --output reference/boom_traces/nested_branch_cycle.jsonl --max-cycles 3000
scripts/check_boom_trace.sh reference/boom_traces/independent_alu_commit.jsonl
scripts/check_boom_trace.sh reference/boom_traces/raw_chain_cycle.jsonl
scripts/check_boom_trace.sh reference/boom_traces/branch_taken_cycle.jsonl
scripts/check_boom_trace.sh reference/boom_traces/branch_not_taken_cycle.jsonl
scripts/check_boom_trace.sh reference/boom_traces/nested_branch_cycle.jsonl
scripts/validate_trace_against_elf.py --trace reference/boom_traces/independent_alu_commit.jsonl --loadmem tb/programs/boom_reference/build/independent_alu.hex --report reports/equivalence/gate2_5/independent_alu_elf_check.md
scripts/diagnose_chipyard_simulator.sh
scripts/build_official_chipyard_sim.sh
```
