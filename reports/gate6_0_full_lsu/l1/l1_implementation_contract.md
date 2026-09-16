# L1 Implementation Contract

Scope is `PARAMETERIZED_CANONICAL_LQ_SQ_AND_STABLE_IDENTITY`.

Allocation remains at execute-completion acceptance. Entries no longer move during reclaim or squash: allocation scans for a free slot from the queue tail, reclaim clears the exact owner slot, and count tracks valid population. ROB-age load issue, one outstanding load, all-older-store blocking, coupled store address/data, commit-only store issue, and no forwarding/speculation/replay are unchanged.

`L1_MEMORY_BEHAVIOR_CHANGED=false`
