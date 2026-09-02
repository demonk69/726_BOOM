# Predictor Generation and Stale Protection

`PREDICTOR_GENERATION_TOKEN=true`

A fixed one-cycle response can become stale when reset, branch recovery, architectural redirect, generic flush, or a future ICache event intervenes. The response must be drained but ignored unless its generation and pending request token match live frontend state.

## Options

| Scheme | Advantages | Risks | Decision |
|---|---|---|---|
| Reuse 32-bit Frontend epoch | already tags IMEM requests, changes on reset/redirect, one speculation generation owner | current branch recovery increments epoch both in `branch.cpp:288` and `frontend.cpp:97`; ownership must be consolidated before integration | RECOMMENDED_FOR_FOUNDATION |
| Independent predictor generation | decouples predictor pipeline flush and can advance without IMEM | two generations can disagree; requires explicit atomic update/order at every redirect | DEFER until predictor can outlive/decouple from frontend epoch |

Generation alone identifies a speculation era, not a particular request. The one-live-request latency contract also needs a pending prediction token containing or matching exact CFI PC, packet identity, and epoch. A delayed response after reset/redirect is discarded and cannot alter packet mask, next PC, FTQ, or predictor state.

An FTQ index can wrap before a delayed update; therefore update matching uses `{ftq_idx, generation}` plus the ROB allocation identity of the resolving/committing uop. The existing IMEM `fetch_id` is not sufficient because it is dropped before Decode and is not a long-lived packet identity.
