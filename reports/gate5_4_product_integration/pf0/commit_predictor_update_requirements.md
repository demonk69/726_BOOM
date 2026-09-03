# Commit Predictor Update Requirements

Future path:

```text
ROB head commits conditional branch
  -> read actual_taken from validated ROB resolution metadata
  -> use exact FTQ reference to consume predictor metadata index/generation
  -> issue commit-qualified P2 update
  -> retire FTQ lane and reclaim only after update data is consumed/retained
```

1. Predictor metadata remains in FTQ; ROB carries the exact FTQ reference rather than duplicating index/generation fields.
2. `actual_taken` is written into the owner ROB entry at validated resolution and lives through Commit.
3. Final-lane commit can make the FTQ entry reclaimable in the same logical step, but the update read must use consume-before-invalidate ordering.
4. If P2 update cannot be accepted, retain an explicit commit-update record or keep the FTQ entry update-pending; never lose the update or block ROB identity validation ambiguously.
5. F1's current standalone retire-before-read ordering must be adapted or sequenced so a final-lane retire cannot hide metadata needed that step.
6. P2 same-index lookup/update forwarding remains `UPDATE_FORWARD_NEW_VALUE`; a same-step prediction sees the accepted commit update's new counter value.

Predictor generation qualifies predictor-state validity. FTQ allocation generation qualifies FTQ slot identity. They are not interchangeable.
