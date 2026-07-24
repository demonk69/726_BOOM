# Gate 3.1A First Event-Order Failure Classification

The old comparator aligned BOOM and HLS events by global stream position. That compares unrelated dynamic uops and is not valid for an out-of-order core.

Programs tied at earliest old mismatch index `0`: `branch_not_taken`, `branch_taken`, `nested_branch`

## First Failing Row In Existing CSV

- Program: `independent_alu`
- Old mismatch index: `1`
- BOOM event: `branch` pc=`0x0000000080000010` instruction=`0x00719c63` cycle=`1158` commit_order=`4`
- HLS event: `commit` pc=`0x0000000080000004` instruction=`0x00700113` cycle=`4` commit_order=`1`
- BOOM ROB index: `3`
- BOOM branch tag: `0`
- BOOM branch mask: `0`
- Same dynamic uop: `false` for the old mismatch pair
- Classification: `LEGAL_REORDER`
- Real causality violation: `false`

## Earliest Mismatch Index Across Programs

- Program: `branch_not_taken`
- Old mismatch index: `0`
- BOOM event: `branch` pc=`0x0000000080000008` instruction=`0x00208463` cycle=`1156` commit_order=`2`
- HLS event: `commit` pc=`0x0000000080000000` instruction=`0x00300093` cycle=`2` commit_order=`0`
- BOOM ROB index: `1`
- BOOM branch tag: `0`
- BOOM branch mask: `0`
- Same dynamic uop: `false` for the old mismatch pair
- Classification: `LEGAL_REORDER`
- Real causality violation: `false`

## Final Classification

The old event-order failure is `VALIDATION_METHOD_FALSE_POSITIVE` for the current traces. BOOM branch-resolution events occurring before older commits are legal out-of-order behavior. The available traces show no same-uop order violation, no commit-order violation, and no committed wrong-path instruction in the compared prefix.

RAW/WAR/WAW timing is not marked verified because the traces do not expose enough issue, wakeup, and rename-source signals. Those constraints are classified as `INSUFFICIENT_SIGNAL`, not as failures.
