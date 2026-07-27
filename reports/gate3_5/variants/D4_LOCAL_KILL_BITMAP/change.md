# D4_LOCAL_KILL_BITMAP Change

Single-variable structural change: compute local kill bitmaps once for IQ, ROB, LDQ, and STQ cleanup, then use those bitmaps in the state update/compaction phase.

Changed files:

| File | Purpose |
|---|---|
| `src/branch.cpp` | Adds entry-bit helper; computes `iq_kill_bitmap`, `rob_kill_bitmap`, `ldq_kill_bitmap`, and `stq_kill_bitmap`; passes IQ kill bitmap into compaction. |

Architecture and cycle intent:

| Constraint | Status |
|---|---|
| Queue depths unchanged | Preserved |
| Branch mask width unchanged | Preserved |
| Kill timing unchanged | Intended preserved; checked by trace comparison |
| IQ relative survivor order | Preserved by existing stable compact loop |
| ROB tail recovery | Preserved by using the same branch index rule |
| LDQ/STQ cleanup timing | Preserved |
| CORE_CYCLE pipeline | Disabled |

This variant is not combined with any other structural optimization.
