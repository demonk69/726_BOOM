# PF5 Training Eligibility

PF5_TRAINING_ELIGIBILITY_POLICY=`ARCHITECTURALLY_COMMITTED_CONDITIONAL_BRANCH_EXACTLY_ONCE`

An update is emitted only from the normal commit path when the ROB head is
valid, complete, non-exceptional, conditional, resolved, and owns a live FTQ
reference whose allocation generation, lane, CFI type, predictor generation,
and BIM metadata index match. The ROB entry is invalidated by that same commit,
which makes the event exact-once. An occupied pending update stalls an otherwise
eligible commit rather than dropping or overwriting training.

Execute and Completion never update BIM state. JAL, JALR, stale references,
wrong lanes, metadata mismatches, unresolved entries, squashed entries, and
exception-killed entries do not emit updates.
