# Gate 2 Results

Date: 2026-07-24

Gate 2: PARTIAL PASS - Gate 2.5 loadmem-backed standalone generated-model traces

READY_FOR_PROVISIONAL_GATE_3=true

READY_FOR_GATE_3=false

## Summary

Gate 2.5 found and fixed the semantic flaw in the earlier standalone traces. The previous ~1,800 commits came from hand-encoded fallback programs that ended in `jal x0,0` and ran until fixed `--max-cycles 3000`; those old traces were truncated and not complete program references.

The regenerated traces now terminate by a real retired store to configured `tohost` address `0x80000080`. They are suitable for provisional architectural commit comparison and event-order exploration against the generated BOOM `VTestHarness` model. They are not sufficient for strict Cycle Equivalence or official Gate 3 because ELF/binutils, the full Chipyard/FESVR/DRAMSim simulator, and official noninterference remain blocked.

See `reports/equivalence/gate2_5_results.md`.

## BOOM / Chipyard Version

| Item | Value |
|---|---|
| BOOM commit | unavailable: no git repository/source checkout present |
| Chipyard commit | unavailable: no git repository/source checkout present |
| Rocket Chip commit | unavailable: no git repository/source checkout present |
| Config name | `chipyard.TestHarness.SmallBoomConfig` / `SmallBoomConfig` |
| Verilator executable | `Verilator 5.030 2024-10-27 rev v5.030` |
| Verilator headers used for standalone build | `/tmp/opencode/verilator-4.038-pkg/usr/share/verilator/include` |
| Generated model archive | `/home/lab_726/boom/chipyard.TestHarness.SmallBoomConfig/chipyard.TestHarness.SmallBoomConfig/VTestHarness__ALL.a` |
| Reset vector | `0x10040` |

## Build Status

| Item | Status | Evidence |
|---|---|---|
| Existing generated BOOM archive | PASS | `VTestHarness__ALL.a` links successfully. |
| Standalone BOOM trace simulator build | PASS | `reports/equivalence/gate2/build_boom_trace.log`. |
| Reference programs build | PASS_WITH_FALLBACK | Hand-encoded finite RV64I loadmem images copied because `riscv64-unknown-elf-gcc` is unavailable. |
| Trace semantic validation | PASS_LOADMEM | `reports/equivalence/gate2_5/*_elf_check.md` in loadmem mode. |
| Original simulator binary | BLOCKED | `simulator-chipyard-SmallBoomConfig` not found. |
| Official Chipyard emulator rebuild | BLOCKED | `reports/equivalence/gate2_5/chipyard_build.log`. |
| Official noninterference | BLOCKED | Original Chipyard emulator path unavailable. |

## Trace Outputs

| Required Trace | Path | Status |
|---|---|---|
| Commit trace | `reference/boom_traces/independent_alu_commit.jsonl` | PASS: 109 events, 40 commits, `tohost`, not truncated |
| RAW cycle trace | `reference/boom_traces/raw_chain_cycle.jsonl` | PASS: 109 events, 40 commits, `tohost`, not truncated |
| Branch taken cycle trace | `reference/boom_traces/branch_taken_cycle.jsonl` | PASS: 114 events, 41 commits, `tohost`, not truncated |
| Branch not-taken cycle trace | `reference/boom_traces/branch_not_taken_cycle.jsonl` | PASS: 114 events, 42 commits, `tohost`, not truncated |
| Nested branch cycle trace | `reference/boom_traces/nested_branch_cycle.jsonl` | PASS: 120 events, 42 commits, `tohost`, not truncated |

## Counts

| Metric | Value |
|---|---:|
| Trace files checked | 5 |
| Trace records | 566 |
| Commit records | 205 |
| Metadata records | 10 |
| Tohost records | 5 |
| Branch resolve records | 24 |
| Flush records | 55 |

## Automatic Check Results

See `reports/equivalence/gate2/check_boom_traces.log`.

```text
independent_alu_commit.jsonl: TRACE_CHECK PASS events=109 commits=40 last_cycle=1171
raw_chain_cycle.jsonl: TRACE_CHECK PASS events=109 commits=40 last_cycle=1171
branch_taken_cycle.jsonl: TRACE_CHECK PASS events=114 commits=41 last_cycle=1182
branch_not_taken_cycle.jsonl: TRACE_CHECK PASS events=114 commits=42 last_cycle=1173
nested_branch_cycle.jsonl: TRACE_CHECK PASS events=120 commits=42 last_cycle=1195
```

## Remaining Blockers For Full Gate 2

1. Restore or mount the original Chipyard checkout used to generate `chipyard.TestHarness.SmallBoomConfig`.
2. Provide matching FESVR and DRAMSim2 libraries.
3. Install or expose `riscv64-unknown-elf-gcc`, `riscv64-unknown-elf-readelf`, `riscv64-unknown-elf-objdump`, and `riscv64-unknown-elf-nm`.
4. Rebuild the official simulator and run official traced/non-traced noninterference.
5. Compare official simulator and standalone commit streams before using standalone traces as official evidence.

Cycle Equivalence remains INSUFFICIENT_EVIDENCE.
