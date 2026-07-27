# B1_PACKED_ALLOC_BITMAP Change

Single-variable structural change: replace `branch_state.br_alloc_lists[8][52]` with one packed 52-bit allocation bitmap per branch tag.

Changed files:

| File | Purpose |
|---|---|
| `include/boom_state.hpp` | Adds `BranchAllocBitmap` and fixed-width bit helpers; stores `br_alloc_bitmap[MAX_BRANCH_COUNT]`. |
| `src/rename.cpp` | Records physical destination allocation by setting one bit in each active tag bitmap. |
| `src/branch.cpp` | Uses selected rollback bitmap once; applies per-tag AND-NOT to clear recovered allocations. |
| `src/synth_module_tops.cpp` | Updates attribution top stimulus to seed the packed bitmap. |
| `tb/differential/branch_snapshot_tests.cpp` | Updates representation-specific assertions to use `branch_alloc_test`. |
| `tb/differential/branch_snapshot_random_tests.cpp` | Adds seed override support for Gate 3.5 random-seed expansion. |

Architecture and cycle intent:

| Constraint | Status |
|---|---|
| Branch count/depth unchanged | Preserved |
| Physical register count unchanged | Preserved |
| x0/physical register 0 never recorded | Preserved by `branch_alloc_set` guard |
| Branch resolve/rollback cycle unchanged | Intended preserved; checked by trace comparison |
| CORE_CYCLE pipeline | Disabled |
| Snapshot RAM experiment | Not touched |

This variant is not combined with any other structural optimization.
