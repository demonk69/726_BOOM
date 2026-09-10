# PF4 Branch Recovery Architecture

This contract was derived from the canonical modular source before PF4 product
code changes. `src/boom_all.cpp` was not used.

```text
BRANCH_RESOLUTION_POINT=completion_service_cycle/resolve_oldest_pending_branch_before_completion_service
CURRENT_BRANCH_REDIRECT_POLICY=EXECUTE_ACTUAL_TAKEN_OVERLOADED_AS_MISPREDICT_AND_CONSUMED_ONE_CYCLE_LATER
CURRENT_BRANCH_SQUASH_POLICY=RECOVER_MISPREDICT_PRESERVES_OWNER_AND_KILLS_ONLY_YOUNGER_BRANCH_MASK_OR_ROB_AGE_STATE
CURRENT_RENAME_RECOVERY_POLICY=RESTORE_EXISTING_PER_BRANCH_MAP_SNAPSHOT_AND_ROLL_BACK_OWNER_ALLOCATION_BITMAP
CURRENT_FTQ_BRANCH_SQUASH_POLICY=GENERATION_QUALIFIED_OWNER_REDIRECT_PRESERVES_LANES_THROUGH_BRANCH_AND_INVALIDATES_YOUNGER_ENTRIES
PF4_PREDICTION_METADATA_LOOKUP_POINT=OLDEST_VALID_BRANCH_COMPLETION_BEFORE_CLASSIFICATION_AND_BEFORE_FTQ_REDIRECT
```

## Current Pipeline

`boom_core_step` orders completion/branch resolution before LSU, Commit,
Frontend, decode, rename, ROB, issue, and Execute. Execute therefore produces
actual branch information in cycle N and Completion resolves it in cycle N+1.
Only the oldest valid pending branch is selected, using ROB-head-relative age.

The current Execute result incorrectly overloads `mispredict` with actual taken.
Consequently every taken conditional, JAL, and JALR invokes recovery. PF4 must
separate actual direction/target/fallthrough from correction classification.

## Frozen Recovery Mechanism

The existing branch checkpoint is retained. Rename assigns one of eight branch
tags, saves the map after the branch's own destination rename, and records all
younger physical allocations. Recovery restores that snapshot, frees only
younger allocations, removes younger tags, rebuilds busy state, and keeps all
ancestor tags. A correct prediction only releases the resolved tag and clears
its mask; it never rolls rename back.

The existing recovery path kills younger IQ, issued, Execute, completion, LSU,
ROB, decode, dispatch, Fetch Buffer, and FTQ state. The resolving owner remains
in the ROB and its FTQ lane remains live until ordinary commit/exception
retirement. No new checkpoint architecture is permitted for PF4.

## PF4 Resolution Contract

The oldest live branch completion performs a narrow FTQ lookup using index,
32-bit allocation generation, lane, and CFI type. Comparison is legal only when
entry validity, generation, original/live lane ownership, and CFI identity all
match. The full 211-bit entry must not be copied into MicroOp or ROB.

Conditional and JAL corrections call the existing younger-state recovery only
when validated predicted next PC differs from actual next PC. Correct predicted
NT, correct predicted T, and correct static JAL release the checkpoint without
Frontend redirect, ROB/FTQ squash, or rename rollback. JALR remains unpredicted
and retains execute-time recovery to its actual target.

Frontend is the sole owner of recovery PC, epoch, outstanding IMEM, packet, and
Fetch Buffer mutation. Backend recovery emits the correction and handles
backend/FTQ younger lifetime; it must not independently mutate Frontend PC or
increment epoch.

## Exception And Age Contract

Runtime reset and precise architectural exception have priority over branch
correction. A branch correction kills a younger exception, but cannot remove an
older exception owner. Completion resolves at most one oldest branch per cycle;
a younger branch result cannot issue a second redirect or mutate FTQ after an
older correction.

## Source Anchors

- `src/boom_core_step.cpp`: canonical stage order.
- `src/completion.cpp`: completion capture, age selection, precise fences.
- `src/execute.cpp`: actual branch predicate and targets.
- `src/branch.cpp`: checkpoint release, younger squash, rename/FTQ recovery.
- `src/commit.cpp`: precise exception recovery and FTQ retirement.
- `src/frontend.cpp`: redirect priority, predictor response, packet admission.
- `include/ftq.hpp`: FTQ generation/lifetime contract.
