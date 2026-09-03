# Product Mispredict Resolution

Comparison inputs are `predicted_valid`, `predicted_taken`, `target_valid`, `predicted_target`, and selected FTQ CFI lane/type against actual CFI lane/type, `actual_taken`, and `actual_target`.

Priority is one classification per resolution:

1. `CFI_SLOT_MISPREDICT`: actual resolving CFI lane/type does not match the recorded selected CFI.
2. `NO_TARGET_PREDICTION`: actual control transfer is taken but prediction or required target is invalid.
3. `DIRECTION_MISPREDICT`: valid predicted direction differs from actual direction.
4. `TARGET_MISPREDICT`: both predict taken and actual taken, but target is invalid or differs.
5. Correct prediction.

| CFI | Rules |
|---|---|
| conditional | Compare direction first; compare target only when both are taken; committed branch trains BIM with actual direction |
| JAL | Expected valid/taken/static target; slot or target mismatch is recovery; no BIM direction training |
| JALR | Foundation prediction invalid; actual transfer is `NO_TARGET_PREDICTION`; no BIM training |

The current `mispredict=actual_taken` encoding must not survive product integration. Actual outcome, comparison result, and recovery class are separate values.
