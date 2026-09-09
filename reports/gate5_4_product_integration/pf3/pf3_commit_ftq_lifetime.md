# PF3 Commit FTQ Lifetime

The one-wide normal Commit path emits one `{idx,generation,lane}` retire event
after architectural commit. Canonical F1 clears the lane only when the slot and
generation are active and the lane is still live; duplicate and stale events
are rejected. Ordered reclaim occurs only at a zero-live head.
