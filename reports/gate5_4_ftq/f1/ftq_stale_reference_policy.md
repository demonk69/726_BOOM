# FTQ Stale Reference Policy

`FTQ_STALE_REFERENCE_POLICY=PER_ENTRY_32_BIT_ALLOCATION_GENERATION_PLUS_LANE`  
`FTQ_GENERATION_BITS=32`

Index-only references alias after ring wrap. Frontend epoch reuse would couple
this standalone block to unresolved product recovery ownership. F1 therefore
uses a monotonically advancing allocation sequence stored in every entry and
returned with the index. Reads, retire, squash, and redirect require exact
`{index,generation}` match; lane events additionally identify the lane.
Runtime reset invalidates controls and advances rather than clears the sequence.
Modulo-2^32 alias is the explicit bounded lifetime of this foundation and must
be revisited in PF0 before product integration.
