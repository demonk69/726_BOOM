# PF4 Redirect Priority

```text
PF4_REDIRECT_PRIORITY=RUNTIME_RESET>PRECISE_ARCHITECTURAL_EXCEPTION_OR_INTERRUPT>OLDEST_VALID_BRANCH_CORRECTION_OR_UNPREDICTED_JALR>GENERIC_OR_LOCAL_FLUSH>SAFE_JAL_STEERING>VALID_CONDITIONAL_PREDICTION_STEERING>NORMAL_SEQUENTIAL_FETCH
```

Prediction steering is normal speculative next-PC selection, not a recovery
event. It never increments Frontend epoch, rolls rename back, squashes ROB/FTQ,
or overrides a reset, architectural redirect, older correction, or flush.

Completion chooses at most one oldest live branch result by ROB-relative age.
Precise architectural exception recovery runs before Frontend and overrides a
same-cycle branch redirect. Frontend owns the one final PC/epoch mutation after
priority selection; backend recovery owns only younger speculative state and
the generation-qualified FTQ redirect.

JAL static steering remains ahead of conditional steering when it is the oldest
surviving packet CFI. JALR has no Frontend target prediction and redirects from
Execute through the branch-correction priority level.
