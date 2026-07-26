# Branch Snapshot Status

Date: 2026-07-26

Gate 3.4 update: resource attribution confirms snapshots are generated as 256x8 `RAM_AUTO_1R1W` storage and `br_alloc_lists` as 416x1 `RAM_AUTO_1R1W` storage. No branch recovery behavior was changed, and M009 remains `PARTIALLY_VERIFIED`.

## Conclusion

M004 and branch snapshots describe different issues. M004 remains the Gate 1 JALR redirect closure. Gate 3.3 separately implements BOOM-style branch tags, masks, rename-map snapshots, free-list allocation rollback, and selective younger-state kill for the supported single-lane integer/minimal-LSU HLS subset.

| Question | Answer |
|---|---|
| Does M004 mean all branch recovery is fixed? | No. M004 only covers the concrete Gate 1 JALR redirect test. |
| Are BOOM branch snapshots implemented in HLS? | Yes for the supported HLS subset. Gate 3.3 allocates `br_tag`, propagates `br_mask`, snapshots the integer rename map, tracks per-branch physical allocations, restores on mispredict, and prunes/clears resolved masks. |
| What BOOM source was used? | The original Chisel checkout is absent. The extraction is based on generated SmallBoomConfig FIRRTL and Verilog only; see `docs/branch_recovery_source_mapping.md`. |
| Does HLS still use a functional substitute? | Partially. Busy recovery is rebuilt from still-valid busy ROB entries after rollback. This is functionally covered for the subset but is not proven cycle-identical to BOOM `RenameBusyTable`. |
| Architectural impact | PARTIALLY_VERIFIED. Directed tests pass 25/25, Gate 1 regressions pass 13/13, minimal LSU tests pass 14/14, branch snapshot tests pass 30/30, branch snapshot random tests pass 2/2, and full-program architectural diff remains 10/10 PASS. |
| Microarchitectural impact | PARTIALLY_VERIFIED. The prior structural absence of branch tags, masks, map snapshots, allocation-list recovery, and selective younger-uop kill is closed for the supported HLS subset. Full BOOM lane parallelism, memory system, predictor, and cycle timing remain outside this claim. |
| Cycle impact | INSUFFICIENT_EVIDENCE. No official BOOM Verilator event trace exists yet, and HLS branch recovery timing is not proven cycle-equivalent. |
| Should M004 be downgraded? | No. M004 remains VERIFIED as a specific JALR redirect mismatch. Branch snapshot recovery remains separately tracked as M009. |
| Is a separate mismatch needed? | Yes. M009 is now PARTIALLY_VERIFIED for the supported subset, with residual full-BOOM and cycle-equivalence risk still open. |

## Required Reporting Rule

Do not close M009 based on M004. Gate 3.3 closure for M009 must cite the dedicated branch snapshot tests, source mapping, and csynth evidence, not the M004 JALR test.
