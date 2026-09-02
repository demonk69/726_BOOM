# Prediction Update Lifecycle

## Three Event Points

### A. Execute Resolution

Execute produces actual taken and actual target from operands and decoded immediate (`src/execute.cpp:155-162`). It compares them with the FTQ prediction and classifies correction. Redirect and speculative younger-state correction happen immediately at resolution. The current overloaded `mispredict` boolean must eventually be split, but P0 does not modify it.

The resolving uop supplies branch PC by `{ftq_idx, halfword_offset}` reconstruction in the future contract; direct `debug_pc` remains current source until integration. Predictor metadata is matched through the same FTQ entry plus generation, with ROB `{rob_idx, allocation_id}` continuing to validate dynamic instruction ownership.

### B. ROB Commit

Predictor training is commit-qualified. The ROB/FTQ retains the resolved actual outcome until the control instruction reaches commit. The predictor consumes `{metadata_token, branch_pc, cfi_type, predicted outcome, actual_taken, actual_target, generation}`. This avoids training wrong-path instructions and avoids rollback machinery in the foundation.

- Squashed instructions do not train.
- Exception-path instructions do not train unless they retire normally; a faulting control instruction does not train.
- Correctly and incorrectly predicted committed conditional branches both train BIM.
- JAL may update future target structures but has no BIM direction update in the foundation.
- JALR/return has no foundation table update.

### C. FTQ Reclaim

F0 live-uop count remains the basic reclaim condition, but predictor integration adds an ordering rule: a zero-live entry cannot be invalidated until any commit-qualified predictor update for that entry has been accepted or conclusively suppressed. Commit width is one, so the commit/update event is singular.

Predictor update and FTQ reclaim may occur in the same cycle only with explicit consume-before-invalidate semantics. They are not the same event: update is table training, reclaim is storage lifetime. Backpressure on the update port must retain the FTQ/ROB update record.

## Required Answers

1. Actual taken is produced by Execute branch predicates; JAL/JALR are unconditionally taken.
2. Actual target is `PC+immediate` for branch/JAL and `(rs1+immediate)&~1` for JALR.
3. Current branch PC is `MicroOp.debug_pc`; future source is FTQ base plus uop halfword offset.
4. Metadata matches by FTQ index/generation and ROB allocation identity.
5. Foundation BIM trains only when the branch commits and update is accepted.
6. Squashed instructions do not train.
7. Faulting/non-retired exception-path instructions do not train.
8. FTQ lives through all uop commit/squash events and any pending commit update.
9. Live count reaches zero after exact-once commit/squash decrements; reclaim also waits for update acceptance.
10. Update and reclaim may share a cycle only under ordered handshake semantics.
