# B4_SET_ROLLBACK Change

Single-variable structural change: preserve `branch_state.br_alloc_lists[8][52]` storage, but build rollback/free/mapped bitsets once inside `rollback_free_list` and apply one duplicate-safe release set to the free list.

Changed files:

| File | Purpose |
|---|---|
| `src/branch.cpp` | Replaces per-preg `preg_in_map` and `branch_free_list_contains` rescans with once-built `rollback_regs`, `currently_free_regs`, and `architecturally_mapped_regs` bitsets. |

Release formula used by this implementation:

```text
release_regs = rollback_regs
             AND NOT currently_free_regs
             AND NOT architecturally_mapped_regs
             AND NOT commit_recycled_regs
             AND NOT physical_register_0
```

`commit_recycled_regs` is zero in the current accepted cycle ordering because `branch_module` executes before `rob_commit_module`; prior commit recycling is already represented in `currently_free_regs`.

Architecture and cycle intent:

| Constraint | Status |
|---|---|
| Branch count/depth unchanged | Preserved |
| Physical register count unchanged | Preserved |
| Allocation-list storage unchanged | Preserved |
| Branch resolve/rollback cycle unchanged | Intended preserved; checked by trace comparison |
| Free-list insertion point | Single release application loop |
| CORE_CYCLE pipeline | Disabled |

This variant is not combined with any other structural optimization.
