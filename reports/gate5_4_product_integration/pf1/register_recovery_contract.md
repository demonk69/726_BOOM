# PF1 Register Recovery Contract

Exception take restores the speculative integer map table from the committed
map table. Physical registers reachable from committed mappings remain
allocated; all other nonzero physical registers are rebuilt into a unique free
list. The busy table is cleared because every retained mapping is committed.

ROB, Decode, dispatch packets, issue queue/grants, execute result slots,
divider state, completion holding state, LDQ/STQ, and branch tags/snapshots are
cleared. PRF payload is not cleared. `rob.next_allocation_id` and
`lsu.next_transaction_id` are preserved so delayed pre-exception completion or
memory responses cannot alias newly allocated work.
