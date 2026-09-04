# Conditional Shadow Policy

`PF2_CONDITIONAL_PREDICTION_MODE=SHADOW_ONLY`.

P1 identifies the conditional and P2 returns its real BIM direction, including WT/ST taken. PF2 records observable response state but does not change Frontend PC and does not mask the younger packet lane.

This is required because prediction metadata is intentionally not propagated to MicroOp, ROB, Execute, or Commit in PF2. If Frontend redirected on predicted taken and the actual branch were not taken, current Execute would have no record that a taken prediction occurred and could not reliably recover to fall-through. Assuming permanent weak-not-taken is not used: directed and random tests actively train counters to `10` and `11` and verify taken responses with unchanged PC/mask.

`PF2_STAGED_DEVIATION_FROM_PF0=CONDITIONAL_TAKEN_RESPONSE_DOES_NOT_MASK_YOUNGER_LANE_OR_STEER_PC_UNTIL_PREDICTION_METADATA_REACHES_BRANCH_EXECUTE`
