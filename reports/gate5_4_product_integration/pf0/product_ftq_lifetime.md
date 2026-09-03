# Product FTQ Lifetime

F1 live tracking is `LANE_MASK`. Initial live mask equals the final admitted packet mask (`01` or `11`). Every admitted instruction produces exactly one terminal live event: commit or squash, never both.

| Event | FTQ action |
|---|---|
| commit | clear exact `{idx,generation,lane}` after required metadata consumption |
| ordinary squash | clear the exact still-live lane once |
| same-packet predicted mask | excluded lane is absent from initial live mask; no clear event |
| branch mispredict | redirect retains owner entry, intersects owner surviving lanes, removes younger suffix |
| older branch squash | clear/remove all still-live younger lanes/entries including both owner-packet lanes as applicable |
| exception | retain fault owner through EPC/cause capture, kill younger entries, then retire/reclaim owner under defined trap ordering |
| reset | invalidate control state and advance generation; do not payload-reset entries |

Duplicate lane clears and stale-generation events are rejected, not counted. Reclaim is ordered from FTQ head, at most one entry per logical step under F1, and only after live mask is zero and any required predictor update obligation is discharged.
