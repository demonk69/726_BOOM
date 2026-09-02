# Standalone FTQ Contract

`FtqFoundation<Depth, FullPayloadReset>` supports depths 2, 4, 8, 16, 32,
and 64. Depths 2/4 exist for model checking; 8/16/32/64 are evaluated
candidates. The canonical depth is 32 with 5-bit external indices.

Allocation occurs only for `alloc_valid && alloc_ready` with mask `01` or
`11`. Mask `00` is a nonallocating no-op and `10` or masks with high bits set
are rejected. Full is represented by `count==Depth`, never by pointer equality
alone. Each packet receives one entry and one 32-bit allocation generation.

Retire and squash references carry `{ftq_idx,generation,lane}`. A lane event
clears one live bit exactly once. Only a zero-live head can reclaim, with at
most one reclaim per logical step. Reclaim can make room for allocation in the
same step.

Redirect has priority over lane events and allocation. A validated owner is
retained, its live mask is intersected with the surviving-lane mask, every
younger entry is invalidated, and tail becomes owner+1. A stale owner is
rejected. Runtime reset clears all validity/live controls and pointers but not
payload, while advancing the allocation sequence.

P2 predictor generation is retained independently from FTQ allocation
generation. The 64-bit P2 request token is excluded because it is consumed by
the blocking request/response matcher before allocation and is not update
metadata. Logical entry width is 211 bits. Canonical total state is
`32*211 + head(5) + tail(5) + count(6) + next_generation(32) = 6800` bits.
Instruction payload, original/decompressed instruction, faults, carry, ROB,
rename, LSU, and physical-register state are excluded.
