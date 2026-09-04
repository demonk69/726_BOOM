# Predictor Product State Ownership

`BoomCoreState::predictor` is the sole canonical `PredictorFoundation<256>` product instance. Its counter table, valid table, held response, indexing, lazy initialization, generation behavior, and same-index forwarding remain in canonical P2 code. No table is copied into `FrontendState`.

Frontend owns only transaction context: held packet, original/final mask, selected P1 result, pending/request-sent bits, Frontend epoch, P2 generation, and request token. Reset increments `BoomCoreState::predictor_generation`, resets P2 controls, and invalidates Frontend transaction context. Product update input is always inactive; no Commit wiring exists.
