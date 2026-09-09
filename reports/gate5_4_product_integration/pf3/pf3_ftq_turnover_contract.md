# PF3 FTQ Turnover Contract

Canonical F1 applies retire/squash, then reclaims at most one zero-live head,
then computes allocation readiness. A full queue therefore permits allocation
in the same logical step only when that step reclaims its head. Redirect has
priority and suppresses allocation. PF3 directed testing fills all 32 entries,
retires the head, and observes simultaneous reclaim/allocation with count
remaining 32.
