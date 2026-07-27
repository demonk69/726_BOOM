# C1_RECOVERY_ENABLE Change

Single-variable structural change: isolate busy-table recovery behind an explicit recovery-enable boundary.

Changed files:

| File | Purpose |
|---|---|
| `src/branch.cpp` | Builds one ROB-derived recovery busy bitmap and applies it through `apply_busy_recovery` only when `recovery_enable` is true. |

Architecture and cycle intent:

| Constraint | Status |
|---|---|
| Recovery remains single-cycle | Preserved |
| ROB scan depth unchanged | Preserved |
| Busy table width unchanged | Preserved |
| Branch mispredict cycle unchanged | Intended preserved; checked by trace comparison |
| Dependent issue timing | Intended preserved; checked by accepted trace comparison |
| CORE_CYCLE pipeline | Disabled |

This variant is not combined with any other structural optimization.
