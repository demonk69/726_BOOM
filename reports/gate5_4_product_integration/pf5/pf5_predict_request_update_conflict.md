# Predictor Request/Update Conflict

The implementation preserves the accepted P2 policy
`UPDATE_FORWARD_NEW_VALUE`. Commit provides only the update PC, 8-bit BIM
metadata index, direction, CFI type, and generation. When a Frontend request
maps to the same index in that predictor step, P2 selects the newly computed
2-bit counter. Different-index requests read their own entry. No FTQ payload is
forwarded on this path.
