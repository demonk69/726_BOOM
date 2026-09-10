# PF4 FTQ Prediction Lookup Contract

```text
PF4_LOOKUP_INPUT=FTQ_IDX_PLUS_ALLOCATION_GENERATION_PLUS_LANE_PLUS_EXPECTED_CFI_TYPE
PF4_LOOKUP_OUTPUT=REFERENCE_VALID_CFI_MATCH_PREDICTION_VALID_PREDICTED_TAKEN_TARGET_VALID_PREDICTED_TARGET_CFI_TYPE_PREDICTOR_METADATA
PF4_STALE_LOOKUP_RECOVERY_POLICY=CONSERVATIVE_REDIRECT_TO_ACTUAL_ARCHITECTURAL_NEXT_PC_AND_SQUASH_YOUNGER_WORK
```

Validation order is mandatory:

1. The resolving ROB allocation owner is live.
2. `uop.ftq_valid` is true and the FTQ index is in range.
3. The indexed FTQ entry is valid and its allocation generation matches.
4. The referenced lane belongs to both original packet mask and current live mask.
5. Recorded CFI lane and type match the resolving instruction.
6. Only then may prediction direction and target participate in comparison.

A failed generation check must not expose any newer slot payload. A stale,
invalid, or CFI-mismatched lookup is never interpreted as random prediction
bits. For a canonical live branch it forces conservative recovery to actual
target when taken or exact `branch_pc + (is_rvc ? 2 : 4)` when not taken, and
kills younger speculative work.

The lookup is narrow. It does not copy `FtqEntry` into MicroOp, completion, or
ROB. Predictor metadata is observable for validation but PF4 never generates a
Commit BIM update.
