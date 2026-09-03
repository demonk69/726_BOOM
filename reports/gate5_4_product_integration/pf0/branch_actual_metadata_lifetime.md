# Branch Actual Metadata Lifetime

Current Execute produces actual condition and target transiently, but Completion only clears ROB busy and emits a one-step BranchUpdate. ROB and Commit store neither `actual_taken` nor `actual_target`; Commit cannot generate a qualified BIM update.

Minimum future ROB-owned resolution state is:

- `branch_resolved`
- `actual_taken` for conditional branches
- a recovery classification or comparison-complete indication

P2 BIM training does not require actual target at Commit. Actual target is needed immediately for Execute recovery and comparison, but must not be added to persistent ROB state solely for a future BTB. If verification or target-mismatch retirement accounting requires it, that must be justified in its own gate.

The metadata write must validate `{rob_idx, rob_allocation_id}` and the uop's exact FTQ reference. Squashed, stale, faulting, or unresolved entries never train.
