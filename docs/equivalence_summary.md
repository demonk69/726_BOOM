# BOOM-HLS Equivalence Summary

Gate 1 status: VERIFIED for the currently implemented integer/control subset.

Gate 2 status: PARTIAL PASS after Gate 2.5. Standalone loadmem-backed traces are now finite and terminate through a real retired store to `tohost`; the prior ~1,800-commit traces were fixed-cycle self-loop traces and are no longer accepted as complete references. The full original Chipyard/FESVR/DRAMSim emulator path remains blocked because the simulator binary, Chipyard source checkout, FESVR/DRAMSim libraries, and RISC-V ELF/binutils toolchain are missing in this workspace.

Provisional Gate 3 status: PARTIAL PASS. BOOM-vs-HLS csim commit prefixes pass for five programs, but event-order and normalized-cycle checks fail because BOOM resolves branch events ahead of older commits while the HLS trace is serialized. Gate 3.1C now extends the loaded-program architectural comparison through the final `SD` to `tohost` for the minimal LSU subset.

Gate 3.1A status: PARTIAL_MATCH. The old BOOM-vs-HLS global event-order failure is classified as `VALIDATION_METHOD_FALSE_POSITIVE`; corrected partial-order analysis finds 8 legal reorder events and 0 real exposed partial-order violations. RAW/WAR/WAW timing remains `INSUFFICIENT_SIGNAL` because issue, wakeup, rename-source, and complete events are not present in the traces.

## Current Verdicts

| Dimension | Verdict | Evidence |
|---|---|---|
| Architectural Equivalence | PARTIALLY_VERIFIED | Original directed suite passes 25/25; Gate 1 regressions pass 13/13; minimal LSU tests pass 14/14; Vitis HLS complete trace csim passes 5/5; Provisional Gate 3 BOOM-vs-HLS loaded-program prefixes pass 45/45 compared commits; Gate 3.1C BOOM-vs-HLS full loaded-program architectural diff passes 10/10 for HLS C++ and csim. Scope still excludes full BOOM LSU/cache/MMU/FPU/TLB/predictor/TileLink/L2. |
| Microarchitectural Equivalence | PARTIALLY_VERIFIED | Rename/ROB/IQ/frontend subset has Gate 1 coverage. Branch snapshots, multi-lane issue/execute, full busy-table wakeup, and BOOM memory/FPU queues remain absent or partial. |
| Cycle Equivalence | INSUFFICIENT_EVIDENCE | Gate 3.1A shows the old global event-order failure was a validator false positive, but normalized-cycle equivalence is still not verified. The full official emulator path remains blocked and HLS still serializes operations that BOOM performs in parallel. |
| Structural Correspondence | PARTIALLY_VERIFIED | SmallBoom parameters and main integer state sizes match; multiple full BOOM modules remain NOT_IMPLEMENTED. |

## Gate 1 Results

| Check | Result |
|---|---|
| Baseline before this Gate | 20/25 directed tests passed |
| Directed tests after fixes | 25/25 passed |
| New Gate 1 regressions | 13/13 passed |
| Vitis HLS 2021.2 csim | 5/5 observable tests passed |
| M003 BEQ not-taken | VERIFIED closed |
| M004 JALR redirect | VERIFIED closed |
| M006 IMEM delayed response | VERIFIED closed |

## Functional Changes

- `frontend.cpp`: fetch IDs now increment for every accepted request; responses are accepted only when a matching request is outstanding; redirect/flush clears pending response state.
- `rename.cpp`: source physical registers are read from the speculative map table, while `committed_map_table` remains commit/recovery state.
- `issue.cpp`: IQ grants are capped to the implemented ALU execute lanes so ready entries are not dropped.
- `directed_tests.cpp`: WAR, ROB full, BEQ not-taken, JALR, and IMEM backpressure tests now exercise their stated behavior.
- `gate1_regression_tests.cpp`: added WAR/WAW/RAW, stale-pdst recycle, branch recovery, stale IMEM response, and IQ selection regressions.

## Remaining Non-Equivalence

- Branch snapshots and branch-mask-based recovery are still MISMATCH for true multiple-unresolved-branch BOOM behavior.
- M004 remains VERIFIED only for the concrete JALR redirect test; it does not close branch snapshot recovery. See `docs/branch_snapshot_status.md`.
- Only the integer ALU/control subset is implemented.
- Full BOOM LSU, caches, MMU/Sv39, TLB, FPU, predictor, TileLink, and L2 remain NOT_IMPLEMENTED. Gate 3.1C only adds a minimal integer LSU path for directed loads/stores and committed store-to-`tohost` termination.
- Cycle equivalence remains INSUFFICIENT_EVIDENCE because Provisional Gate 3 normalized-cycle checks fail for BOOM-vs-HLS and the full official emulator path remains blocked.

## Gate 2 Result

| Check | Result |
|---|---|
| Original BOOM simulator binary | BLOCKED: not found |
| Reference program build | PASS_WITH_FALLBACK: finite hand-encoded loadmem images copied because `riscv64-unknown-elf-gcc` is missing |
| Standalone trace simulator build | PASS: linked existing generated `VTestHarness__ALL.a` |
| Commit trace | PASS_LOADMEM: `independent_alu_commit.jsonl` checked, 40 commits, `tohost` termination |
| Cycle traces | PASS_LOADMEM: RAW, branch taken, branch not-taken, and nested branch traces checked, 40/41/42 commits |
| ELF/binutils validation | BLOCKED: no ELF files or `riscv64-unknown-elf-*` tools |
| Official noninterference | BLOCKED: original Chipyard emulator path unavailable |
| Ready for provisional Gate 3 | true: architectural commit comparison and event-order exploration only |
| Ready for Gate 3 | false |

## Provisional Gate 3 Result

| Check | Result |
|---|---|
| HLS C++ vs HLS csim traces | PASS: architectural, event-order, and normalized-cycle traces match for five prefix programs |
| BOOM vs HLS csim architectural prefix | PASS: 45/45 compared commits match |
| BOOM vs HLS csim event order | FAIL: BOOM branch-resolution events interleave ahead of older commits |
| BOOM vs HLS csim normalized cycles | FAIL: event order and cycle deltas differ |
| Complete loaded-program architectural diff | PASS: Gate 3.1C HLS C++/csim traces match BOOM through retired `SD` to `tohost` for the minimal LSU subset |
| Official Gate 3 | BLOCKED: official simulator/noninterference path remains unavailable |

See `reports/equivalence/provisional_gate3_results.md`.

## Gate 3.1A Result

| Check | Result |
|---|---|
| Old global event-order failure | VALIDATION_METHOD_FALSE_POSITIVE |
| Dynamic uop map | GENERATED |
| Legal reorder events | 8 |
| Real exposed partial-order violations | 0 |
| Commit order | MATCH |
| RAW/WAR/WAW timing | INSUFFICIENT_SIGNAL |
| Next allowed step | minimal LSU/store-to-`tohost` work, without full BOOM LSU claims |

See `reports/equivalence/provisional_gate3_1_results.md`.

## Gate 3.1C Result

| Check | Result |
|---|---|
| Minimal LSU regressions | 14/14 PASS |
| HLS complete traces through `tohost` | PASS for native C++ and Vitis csim |
| BOOM vs HLS full loaded-program architectural diff | 10/10 PASS |
| Baseline Vitis csynth after LSU | TIMEOUT after 30 minutes, no `csynth.rpt` produced |

See `reports/equivalence/provisional_gate3_1/full_program_architectural_diff.md` and `reports/equivalence/provisional_gate3_1/lsu_test_results.md`.

## Timing Status

The last completed csynth evidence remains the prior 92.13 MHz estimate. The Gate 3.1C post-LSU baseline csynth attempt timed out during HLS transformations before a new report was produced, so timing is not updated.
