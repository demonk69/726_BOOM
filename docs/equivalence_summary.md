# BOOM-HLS Equivalence Summary

Gate 1 status: VERIFIED for the currently implemented integer/control subset.

Gate 2 status: PARTIAL PASS after Gate 2.5. Standalone loadmem-backed traces are now finite and terminate through a real retired store to `tohost`; the prior ~1,800-commit traces were fixed-cycle self-loop traces and are no longer accepted as complete references. The full original Chipyard/FESVR/DRAMSim emulator path remains blocked because the simulator binary, Chipyard source checkout, FESVR/DRAMSim libraries, and RISC-V ELF/binutils toolchain are missing in this workspace.

Provisional Gate 3 status: PARTIAL PASS. BOOM-vs-HLS csim commit prefixes pass for five programs, but event-order and normalized-cycle checks fail because BOOM resolves branch events ahead of older commits while the HLS trace is serialized. Gate 3.1C now extends the loaded-program architectural comparison through the final `SD` to `tohost` for the minimal LSU subset.

Gate 3.1A status: PARTIAL_MATCH. The old BOOM-vs-HLS global event-order failure is classified as `VALIDATION_METHOD_FALSE_POSITIVE`; corrected partial-order analysis finds 8 legal reorder events and 0 real exposed partial-order violations. RAW/WAR/WAW timing remains `INSUFFICIENT_SIGNAL` because issue, wakeup, rename-source, and complete events are not present in the traces.

Gate 3.2 status: BASELINE_CSYNTH_PASS. The post-LSU timeout root cause was removed for the conservative baseline by eliminating the per-cycle whole-state copy and moving `CORE_CYCLE` pipelining behind `BOOM_HLS_ENABLE_CORE_PIPELINE`. The accepted baseline `boom_core_top` csynth completes; the separate pipeline-enabled performance experiment still times out and remains deferred.

Gate 3.3 status: BRANCH_RECOVERY_SUBSET_PASS and BASELINE_CSYNTH_PASS. Branch tags, masks, rename-map snapshots, free-list allocation rollback, busy recovery, and selective younger-state kill are implemented for the supported single-lane integer/minimal-LSU subset. Conservative `boom_core_top` and finite `boom_core_step_top` csynth both pass with the same 5.898 ns estimate as Gate 3.2, while strict BOOM cycle equivalence remains unverified.

Gate 3.4 status: ANALYSIS_AND_MODULE_BASELINE_COMPLETE_NO_ACCEPTED_OPTIMIZATION. Resource attribution and targeted module csynth baselines are complete. No optimization candidate was applied or accepted; Gate 3.3 remains the accepted PPA configuration.

## Current Verdicts

| Dimension | Verdict | Evidence |
|---|---|---|
| Architectural Equivalence | PARTIALLY_VERIFIED | Original directed suite passes 25/25; Gate 1 regressions pass 13/13; minimal LSU tests pass 14/14; branch snapshot directed/random tests pass 30/30 and 2/2; Vitis HLS complete trace csim passes 5/5; Provisional Gate 3 BOOM-vs-HLS loaded-program prefixes pass 45/45 compared commits; BOOM-vs-HLS full loaded-program architectural diff passes 10/10 for HLS C++ and csim. Gate 3.3 preserves frozen complete traces byte-identically. Scope still excludes full BOOM LSU/cache/MMU/FPU/TLB/predictor/TileLink/L2. |
| Microarchitectural Equivalence | PARTIALLY_VERIFIED | Rename/ROB/IQ/frontend/minimal-LSU subset has directed coverage. Gate 3.3 implements branch tags, masks, snapshots, allocation-list rollback, and selective younger-state kill for the supported subset. Multi-lane issue/execute, strict busy-table cycle timing, and full BOOM memory/FPU queues remain absent or partial. |
| Cycle Equivalence | INSUFFICIENT_EVIDENCE | Gate 3.1A shows the old global event-order failure was a validator false positive, but normalized-cycle equivalence is still not verified. Gate 3.3 synthesis closure is not a strict cycle-equivalence claim. The full official emulator path remains blocked and HLS still serializes operations that BOOM performs in parallel. |
| Structural Correspondence | PARTIALLY_VERIFIED | SmallBoom parameters and main integer state sizes match; Gate 3.3 branch parameter/state inventories match generated FIRRTL for the implemented subset; Gate 3.4 confirms snapshot and allocation-list RTL structures and module baselines. Multiple full BOOM modules remain NOT_IMPLEMENTED. |

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

- Branch snapshot structural absence is no longer current for the supported HLS subset after Gate 3.3, but strict BOOM event/cycle equivalence for branch recovery remains INSUFFICIENT_EVIDENCE.
- M004 remains VERIFIED only for the concrete JALR redirect test; it did not close branch snapshot recovery. Gate 3.3 branch recovery evidence is tracked separately under M009. See `docs/branch_snapshot_status.md`.
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

## Gate 3.2 Result

| Check | Result |
|---|---|
| Whole-state copy pattern | REMOVED: no `BoomCoreState next_state = state` in `src/` |
| Baseline `CORE_CYCLE` pipeline | DISABLED by default; gated by `BOOM_HLS_ENABLE_CORE_PIPELINE` |
| Module diagnostic csynth | PASS for all 9 diagnostic tops |
| Finite `boom_core_step_top` csynth | PASS, 44.09s, 5.898 ns estimated period |
| Baseline `boom_core_top` csynth | PASS, 45.56s, 5.898 ns estimated period, 40625 LUT, 15985 FF, 16 BRAM_18K, 3 DSP |
| Baseline automatic partition records | 0 parsed records after refactor |
| Performance pipeline experiment | TIMEOUT after 15 minutes; deferred from accepted baseline |
| Official Gate 3 | BLOCKED: official simulator/noninterference path remains unavailable |

See `reports/gate3_2/gate3_2_results.md`, `reports/gate3_2/baseline_csynth_summary.md`, and `reports/gate3_2/top_csynth_modes.csv`.

## Gate 3.3 Result

| Check | Result |
|---|---|
| Branch parameter inventory | MATCH for MAX_BRANCH_COUNT=8, BR_MASK_BITS=8, BR_TAG_BITS=3, LOGICAL_REG_COUNT=32, INT_PHYS_REGS=52, DISPATCH_WIDTH=1, ISSUE_WIDTH=3, ROB_DEPTH=32, LDQ_DEPTH=8, STQ_DEPTH=8 |
| Branch state inventory | IMPLEMENTED or FUNCTIONAL_SUBSTITUTE for supported subset branch tag allocation, masks, map snapshot/restore, allocation-list rollback, busy recovery, and younger-state kill |
| Branch snapshot directed tests | 30/30 PASS |
| Branch snapshot random tests | 2/2 PASS, seed `0x3a33b007` |
| Existing regressions after branch work | directed 25/25 PASS, Gate 1 13/13 PASS, minimal LSU 14/14 PASS |
| Frozen complete trace preservation | HLS C++ 5/5 byte-identical and Vitis csim 5/5 byte-identical |
| Full-program architectural diff | 10/10 PASS |
| Module diagnostic csynth | PASS for all 9 diagnostic tops |
| Finite `boom_core_step_top` csynth | PASS, 69.78s, 5.898 ns estimated period, 83353 LUT, 16808 FF, 16 BRAM_18K, 3 DSP |
| Conservative `boom_core_top` csynth | PASS, 71.69s, 5.898 ns estimated period, 83286 LUT, 16611 FF, 16 BRAM_18K, 3 DSP |
| Performance pipeline experiment | NOT_RUN through Gate 3.4; prior Gate 3.2 timeout remains deferred |
| Official Gate 3 | BLOCKED: official simulator/noninterference path remains unavailable |

See `reports/gate3_3/gate3_3_results.md`, `reports/gate3_3/branch_snapshot_test_results.md`, and `docs/branch_recovery_source_mapping.md`.

## Gate 3.4 Result

| Check | Result |
|---|---|
| Resource attribution | COMPLETE: top LUT delta localized to branch recovery/control instance hierarchy; dominant direct reports identified |
| Snapshot RTL structure | `RAM_AUTO_1R1W`, 256x8 reg-array RAM, no complete partition |
| `br_alloc_lists` RTL structure | `RAM_AUTO_1R1W`, 416x1 reg-array RAM, no complete partition |
| Module baseline | 12/12 requested Gate 3.4 csynth tops PASS |
| Conservative `boom_core_top` csynth | PASS, 69.33s, 5.898 ns, 83286 LUT, 16611 FF, 16 BRAM_18K, 3 DSP |
| Regressions | directed 25/25, Gate 1 13/13, LSU 14/14, branch directed 30/30, branch random 2/2 PASS |
| Trace preservation | HLS C++ 5/5 and Vitis csim 5/5 byte-identical to frozen Gate 3.3 traces |
| Full-program architectural diff | 10/10 PASS |
| Accepted optimization | NONE; Gate 3.3 remains accepted PPA configuration |

See `reports/gate3_4/gate3_4_results.md` and `reports/gate3_4/resource_attribution.md`.

## Gate 3.5 Result

| Check | Result |
|---|---|
| Single-variable experiments | COMPLETE: B1, B4, C1, D1, D4, and D4-IQ all run independently from the accepted baseline |
| Functional regressions | PASS for all six variants: directed 25/25, Gate 1 13/13, LSU 14/14, branch directed 30/30, branch random 42/42 |
| Trace preservation | PASS for all six variants: HLS C++ and Vitis csim traces 10/10 byte-identical to frozen baseline |
| Full-program architectural diff | PASS for all six variants: 10/10 |
| Best single-variable LUT | D4_LOCAL_KILL_BITMAP, 82789 LUT, -497 versus baseline |
| Accepted optimization | NONE; best reduction is 0.60%, below the 10% threshold |

See `reports/gate3_5/gate3_5_results.md`.

## Timing Status

The latest accepted PPA configuration remains Gate 3.3. Gate 3.5 found one small LUT-reducing candidate but did not accept it because it missed the 10% threshold.
