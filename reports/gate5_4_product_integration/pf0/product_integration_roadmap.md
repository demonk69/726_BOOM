# Product Integration Roadmap

```text
RECOMMENDED_PRODUCT_INTEGRATION_ORDER=PF1_EXCEPTION_RECOVERY_FOUNDATION_THEN_PF2_PRODUCT_PREDECODE_PREDICTOR_FRONTEND_THEN_PF3_FB_FTQ_ATOMIC_ALLOCATION_THEN_PF4_FTQ_REFERENCE_BRANCH_COMPARISON_THEN_PF5_COMMIT_BIM_UPDATE_FTQ_RECLAIM_THEN_PF6_FULL_RTL_PPA_ACCEPTANCE
NEXT_GATE=GATE5_4_PF1_EXCEPTION_RECOVERY_FOUNDATION
```

| Gate | Scope | Exit dependency |
|---|---|---|
| PF1 | architectural exception trap entry/return, unified recovery ownership, EPC/cause, younger squash | recoverable nonterminal exception semantics |
| PF2 | product P1 placement, earliest-CFI selection, P2 conditional request/wait/response, no-CFI/JAL/JALR policies | no FTQ integration yet; cycle and stale-response tests |
| PF3 | one atomic FB enqueue plus F1 allocation, FTQ-full backpressure, reset composition | exactly one entry per admitted nonempty final packet |
| PF4 | exact FTQ reference propagation, actual-vs-predicted branch comparison, same-packet recovery | generation/lane stale tests and full-PC cross-check |
| PF5 | ROB actual direction lifetime, commit-qualified BIM update, consume-before-reclaim ordering | update backpressure and same-index forwarding tests |
| PF6 | full RTL/equivalence/PPA acceptance | protection states and timing accepted |

Candidate B is rejected because deferring exception closure until after FTQ/recovery integration would force trap ownership, younger kill, and fault-owner reclaim semantics to be redesigned. PF1 must not implement Predictor or FTQ.
