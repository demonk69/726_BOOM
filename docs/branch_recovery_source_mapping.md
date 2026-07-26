# Branch Recovery Source Mapping

Date: 2026-07-26

## Scope

The original BOOM/Chipyard Chisel source checkout is not present in this workspace. This mapping is based only on the generated SmallBoomConfig FIRRTL and Verilog artifacts under `/home/lab_726/boom/chipyard.TestHarness.SmallBoomConfig/`. FIRRTL source annotations identify Chisel file names and line numbers, but those source files were not available for direct inspection.

## Extracted BOOM Facts

| Mechanism | Generated-source evidence | Extracted behavior |
|---|---|---|
| Branch tag and mask width | `chipyard.TestHarness.SmallBoomConfig.fir:232969-233061` | `BranchMaskGenerationLogic` exposes `br_tag : UInt<3>[1]`, `br_mask : UInt<8>[1]`, `resolve_mask : UInt<8>`, and `mispredict_mask : UInt<8>`. |
| Branch tag pool full | `chipyard.TestHarness.SmallBoomConfig.fir:232977-232982` | An 8-bit `branch_mask` register is full when all bits are set and the current slot is a branch. |
| Branch tag allocation | `chipyard.TestHarness.SmallBoomConfig.fir:232983-233044` | The generated logic selects a free 3-bit tag and builds a one-hot tag mask. With the generated priority order, the final assignment is the lowest available tag. |
| Branch mask propagation | `chipyard.TestHarness.SmallBoomConfig.fir:233045-233061` | New uops receive the current active branch mask with resolved branches cleared; the tracked mask clears on pipeline flush and keeps only the older mask on mispredict. |
| Rename map snapshot | `chipyard.TestHarness.SmallBoomConfig.fir:233237-233269` | `RenameMapTable` stores `remap_table[1][0..31]` into `br_snapshots[ren_br_tag]` when `ren_br_tags[0].valid`. |
| Rename map restore | `chipyard.TestHarness.SmallBoomConfig.fir:233271-233304` | On `brupdate.b2.mispredict`, `map_table[0..31]` is restored from `br_snapshots[br_tag]`. |
| Free-list branch allocation lists | `chipyard.TestHarness.SmallBoomConfig.fir:233545-233660` | `RenameFreeList` forms `alloc_masks`, snapshots/clears the selected branch list, ORs active allocations into per-tag `br_alloc_lists`, and gates rollback deallocation by `brupdate.b2.mispredict`. |
| Free-list rollback | `chipyard.TestHarness.SmallBoomConfig.fir:233554-233563` | `br_deallocs = br_alloc_lists[br_tag]` when the branch update is a mispredict, and rollback deallocations are ORed into the free-list update. |
| Busy table | `chipyard.TestHarness.SmallBoomConfig.fir:233891-233938` | `RenameBusyTable` tracks a 52-bit `busy_table`, clears writeback physical destinations, sets rebusy destinations, and returns source busy bits. |
| Issue slot kill/clear | `chipyard.TestHarness.SmallBoomConfig.fir:219411-219420` | Issue slots clear resolved mask bits and invalidate on `mispredict_mask & slot_uop.br_mask`. |
| Functional-unit kill/clear | `chipyard.TestHarness.SmallBoomConfig.fir:205114-205199` and `chipyard.TestHarness.SmallBoomConfig.fir:205202-205208` | Functional units mark requests/results killed when `mispredict_mask` intersects the uop `br_mask` and clear resolved bits from in-flight masks. |
| LSU branch kill | `chipyard.TestHarness.SmallBoomConfig.fir:293328-293547` | STQ/LDQ entries clear resolved bits and invalidate younger entries whose `br_mask` intersects the mispredict mask. A generated assertion prevents killing committed stores. |
| Dispatch stalls during mispredict | `chipyard.TestHarness.SmallBoomConfig.fir:272710-272714` | Dispatch hazards include nonzero `brupdate.b1.mispredict_mask` and `brupdate.b2.mispredict`. |
| Verilog cross-check | `chipyard.TestHarness.SmallBoomConfig.top.v:240006` | The generated Verilog contains the `BranchMaskGenerationLogic` module corresponding to the FIRRTL extraction. |

## HLS Mapping

| BOOM mechanism | HLS implementation | Status |
|---|---|---|
| 8 active branch tags and masks | `include/boom_config.hpp:36-38`, `include/boom_state.hpp:53-67`, `src/rename.cpp:15-24`, `src/rename.cpp:77-89`, `src/rename.cpp:110-115` | Implemented for the single-lane HLS core. |
| Rename map snapshot after remap | `include/boom_state.hpp:35-42`, `src/rename.cpp:53-58`, `src/rename.cpp:98-112` | Implemented; HLS snapshots the post-rename map and forces logical x0 to physical 0. |
| Map restore on mispredict | `src/branch.cpp:183-191`, `src/branch.cpp:225-250` | Implemented, with committed-map fallback only if no valid snapshot exists. |
| Free-list branch allocation tracking | `include/boom_state.hpp:53-67`, `src/rename.cpp:41-50`, `src/rename.cpp:98-108`, `src/branch.cpp:193-210` | Implemented with per-branch physical destination tracking and duplicate-safe free-list insertion. |
| Branch tag release and pruning | `src/branch.cpp:47-60`, `src/branch.cpp:212-223`, `src/branch.cpp:253-255` | Implemented for correct resolutions and mispredict recovery. |
| Busy source reads and writeback clear | `src/rename.cpp:91-96`, `src/execute.cpp:21-78`, `src/lsu.cpp:148-165` | Implemented in the HLS free-list `busy_table`. |
| Busy recovery after rollback | `src/branch.cpp:173-181`, `src/branch.cpp:225-250` | Functionally implemented by rebuilding busy state from still-valid busy ROB entries. This is a functional HLS recovery, not proven cycle-identical to BOOM `RenameBusyTable`. |
| Younger state kill | `src/branch.cpp:89-171`, `src/branch.cpp:225-250` | Implemented for IQ, issued slots, execute results, LDQ, STQ, ROB, decode, rename, and frontend pending state. |
| In-flight branch-mask clear | `src/branch.cpp:19-21`, `src/branch.cpp:62-74`, `src/issue.cpp:40-68`, `src/execute.cpp:21`, `src/lsu.cpp:165` | Implemented for HLS-visible in-flight structures. |

## Residual Limits

This closes the prior HLS structural absence of branch tags, masks, snapshots, allocation-list rollback, and selective younger-state kill for the supported integer/single-lane subset. It does not prove strict BOOM cycle equivalence because no official Chipyard/FESVR/DRAMSim simulator and no BOOM event trace are available. It also does not implement the full BOOM cache, MMU, FPU, TLB, predictor, TileLink, or L2 behavior.
