# Predictor and FTQ Product Integration

Gate 5.4 PF4 implements conditional BIM steering, effective-lane FTQ metadata,
oldest-branch predicted-vs-actual comparison, correct-prediction no-work, and
precise Frontend-owned recovery. Commit BIM training remains disabled for PF5.
Native, CSim, focused RTL, current-source full-core RTL, preservation, and
canonical synthesis gates pass. Evidence is under
`reports/gate5_4_product_integration/pf4/`.

Gate 5.4 PF0 freezes the integration architecture but implements no product Predictor, FTQ, or recovery changes. The evidence is under `reports/gate5_4_product_integration/pf0/`.

## Frozen Decisions

- Predecode complete canonical instructions immediately after packet construction.
- Select one earliest CFI per packet; send P2 only the selected conditional branch.
- Bypass P2 for no-CFI/fault packets, static-target JAL, and foundation JALR with no target prediction.
- Hold a conditional packet from request N to response N+1; never use standalone wrapper latency/II as product cycles.
- Freeze the post-prediction effective lane mask before a single atomic FB enqueue plus FTQ allocation.
- Carry `{ftq_idx, ftq_generation, lane, halfword_offset}` plus per-uop `is_rvc`; retain full PC in the first integration.
- Keep Frontend epoch/request token, Predictor reset generation, FTQ allocation generation, and ROB allocation ID separate.
- Persist conditional `actual_taken` in the validated ROB owner through Commit; consume FTQ predictor metadata before final reclaim.
- Classify foundation JALR recovery as `NO_TARGET_PREDICTION`, not direction mispredict.

## Stage Boundaries

```text
RESPONSE / CANONICAL PACKET BUILD / P1 PREDECODE
  -> conditional P2 request
PREDICT_WAIT
  -> matching P2 response or bypass result
PACKET_ADMIT
  -> atomic Fetch Buffer enqueue + F1 allocation
```

No-CFI and JAL bypass paths keep the same packet-admission boundary. A conditional response may fire directly with FB and FTQ readiness because P2 holds its response stable; redirect/reset has priority and drains stale responses.

## Recovery Dependency

Current non-ECALL exception behavior enters a terminal `ROB_EXCEPTION` fence. It does not capture trap CSRs, redirect to `mtvec`, squash all younger backend/FTQ ownership, remove the fault owner, or return with MRET. This blocks safe FTQ product lifetime and reclaim semantics.

The unique next gate is `GATE5_4_PF1_EXCEPTION_RECOVERY_FOUNDATION`. Product Predictor integration follows only after unified redirect/generation ownership and recoverable exception semantics are accepted. Product Predictor and FTQ readiness remain false after PF0.

## PF3 Status

Atomic allocation and lifetime focused evidence passes, including wrap,
generation reuse, stale references, same-packet younger kill, and deferred
exception-owner retire. `PRODUCT_FTQ_INTEGRATED=false` remains the product
verdict until PF3 timing, BRAM, product-program, and full-core RTL blockers are
closed. PF4 must not start from this result.

## PF4 Status

Conditional predicted-T and predicted-NT steering is active for the supported
Product-FTQ path. Completion accepts only generation-, live-lane-, and
CFI-qualified FTQ metadata; stale or invalid lookup redirects conservatively to
the actual architectural next PC. Correct conditional/JAL results do not
squash, while conditional corrections and unpredicted JALR recover younger
state. Predicted-T masks a younger fault when the branch is actually taken; an
actual-not-taken correction refetches and precisely takes that fault.

`GATE5_4_PF4_BRANCH_PREDICTION_RECOVERY_VERIFIED=true` and
`READY_FOR_GATE5_4_PF5_COMMIT_BIM_TRAINING=true`.
