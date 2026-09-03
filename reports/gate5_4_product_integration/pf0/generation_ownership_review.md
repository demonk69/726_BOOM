# Generation Ownership Review

```text
PRODUCT_PREDICTOR_GENERATION_POLICY=SEPARATE_PREDICTOR_RESET_GENERATION_FROM_FRONTEND_EPOCH_AND_FTQ_ALLOCATION_GENERATION
```

| Generation | Scope | Producer | Consumer | Protects against | Lifetime | Can share? | Reason |
|---|---|---|---|---|---|---|---|
| Frontend epoch | fetch stream | single Frontend redirect owner | IMEM/carry and pending predictor request matching | stale path responses | until redirect/reset | no | path cancellation, currently double-incremented and must be consolidated |
| pending predictor token | one request | Frontend request allocator | predictor response consumer | response/context mismatch | request to response | no | exact transaction identity |
| P2 predictor generation | predictor state/reset epoch | Predictor reset/lazy-init control | response, FTQ metadata, commit update | update against invalid predictor state | reset epoch through commit | no | older correct-path branches must remain trainable across unrelated younger redirects |
| F1 allocation generation | one FTQ slot incarnation | FTQ allocator | reads/retire/squash/redirect | depth-wrap stale alias | allocation through final reclaim | no | per-entry storage identity |
| ROB allocation ID | one ROB slot incarnation | ROB allocator | completion/redirect/recovery | ROB slot reuse | allocation through removal | no | backend owner identity |

P0's initial Frontend-epoch reuse is not adopted unchanged: advancing predictor generation on every path redirect could reject a valid older branch's later commit update. Frontend epoch plus token cancels pending responses; P2 generation changes only when predictor state validity changes. The existing duplicate Frontend epoch increment must be reduced to one owner in its implementation gate.
