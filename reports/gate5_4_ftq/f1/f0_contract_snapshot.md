# F0 Contract Snapshot

`GATE5_4_F0_FTQ_PREREQUISITES_REVIEWED=true` and
`READY_FOR_STANDALONE_FTQ_IMPLEMENTATION=true` are frozen from F0.

One entry represents one nonempty Fetch Packet atomically accepted at Fetch
Buffer enqueue. Empty packets and cross-word carry state allocate no entry; a
carry-completion packet uses `carry_pc` as its base. Legal masks are `01` and
`11`. The FTQ is distinct from both Fetch Buffer and ROB, may outlive Fetch
Buffer occupancy, uses in-order live-uop reclamation, and has canonical depth
32. Product integration remains blocked by incomplete exception/recovery
ownership.
