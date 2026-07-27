# D1_SHARED_BRANCH_MASKS Change

Single-variable structural change: build branch-cycle masks once for the resolved branch and pass the resulting bundle through the branch recovery/release path.

Changed files:

| File | Purpose |
|---|---|
| `src/branch.cpp` | Adds `BranchCycleMasks` with `resolved_clear_mask`, `mispredict_kill_mask`, and `active_branch_mask_after_resolution`; recovery and release consume the shared bundle. |

Architecture and cycle intent:

| Constraint | Status |
|---|---|
| Branch mask width unchanged | Preserved |
| Correct-prediction clear-bit behavior | Preserved |
| Mispredict kill behavior | Preserved |
| Nested branch keep mask | Preserved by using the resolved uop's inherited branch mask |
| CORE_CYCLE pipeline | Disabled |

This simplified HLS model centralizes branch recovery in `branch_module`; the shared mask bundle therefore targets the existing branch helper fanout rather than introducing a new architectural module.
