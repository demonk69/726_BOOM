# Predictor Architecture Review

Gate 5.4 P0 selects a minimal interface foundation, not an implementation.

## Architecture

- Request after packet construction/predecode has exact earliest-CFI PC, canonical instruction, RVC length, and static target.
- One packet has at most one selected CFI: earliest valid CFI in PC order.
- The foundation is static direct-target predecode plus a 2-bit BIM for conditional direction.
- Direct conditional/JAL targets are `PC+immediate`; JALR needs a register value or dynamic target predictor and is not predicted by foundation.
- Predicted-taken lane 0 removes lane 1 before Fetch Buffer/FTQ allocation; lane 1 is refetched after a direction correction.
- Predictor response is fixed one cycle. A single pending completed packet waits; no late post-enqueue prediction.
- Response stale protection reuses the 32-bit Frontend epoch plus an exact pending-request token.
- FTQ entry remains one atomically accepted nonempty Fetch Packet. It stores the effective post-prediction mask and only metadata with a producer and consumer.
- Execute performs speculative correction; committed control instructions train the predictor; FTQ reclaim waits for update acceptance.

## Foundation Boundaries

`BTB_REQUIRED_FOR_FOUNDATION=false`: direct branch/JAL targets are static. JALR remains sequential until Execute. A BTB can later add dynamic-target prediction.

`RAS_REQUIRED_FOR_FOUNDATION=false`: return acceleration is valuable but depends on mature JALR call/return classification and speculative recovery. Add after BTB/JALR lifecycle or as a separately verified target source.

`GLOBAL_HISTORY_REQUIRED_FOR_FOUNDATION=false`: a BIM is indexed by branch PC and needs no GHR. No history snapshot belongs in minimal FTQ.

Current source has dead placeholders (`FTQ_DEPTH`, `BranchInfo.ftq_idx`, `BranchUpdate.cfi_type`) but no FTQ/predictor producer or consumer. They are not accepted contracts and were not used to justify extra fields.

## Integration Blockers

- No product predecode or predictor metadata transport.
- Execute overloads `mispredict` with actual taken.
- Branch recovery and Frontend both increment epoch for one resolution.
- Architectural exception handling becomes terminal `ROB_EXCEPTION`; full redirect/reclaim ownership is absent.
- FTQ/predictor update-pending and collision semantics require standalone verification.

Detailed evidence and decisions are in `reports/gate5_4_predictor/p0/`.

## Gate 5.4 P1

The first implementation gate now provides a standalone combinational CFI predecoder and two-lane earliest-CFI helper. It is not connected to Frontend. Native directed, exhaustive RV64C, one-million-word random, persistent packet, generated RTL, and standalone synthesis evidence pass. Predictor/BIM/BTB/RAS/GHR/FTQ state remains absent; product integration remains unauthorized.

## Gate 5.4 P2

P2 now verifies a standalone static-target-plus-BIM predictor. Conditional direction uses a 2-bit saturating counter indexed by `(pc >> 1) & (entries - 1)`; invalid entries are logical weak not-taken. Only commit-qualified conditional updates with matching active generation and PC-derived metadata index train, and a same-index lookup receives the forwarded new value. JAL is statically taken, while JALR and no-CFI produce no valid prediction.

The interface remains blocking: request acceptance on logical call N produces a response on call N+1, held stably under backpressure, with no same-call response-consume/request turnover. This is a logical API contract, not one physical HLS clock. Generated RTL reports best-case top transaction latency 3 and minimum II 4 and passes its 164-case call-to-call protocol matrix.

The canonical standalone choice is 256 entries, explicit LUTRAM, and `LAZY_VALID_INIT`. P2 does not connect predictor state to product Frontend, FTQ, Execute, or Commit. Product predictor/FTQ readiness and implementation remain false; F1 standalone FTQ foundation readiness is true. Detailed evidence is in `reports/gate5_4_predictor/p2/`.
