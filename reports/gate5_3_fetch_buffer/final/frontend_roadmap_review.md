# Frontend Roadmap Review

## Candidate Comparison

| Candidate | Missing prerequisites | Present consumer | Orphan-state risk | PPA risk | Verification burden |
|---|---|---|---|---|---|
| Predictor first | Fetch-bundle metadata ownership; prediction/update contract; history and RAS checkpoint/repair; redirect pruning; target and confidence semantics | None. `BranchInfo.ftq_idx` exists but is not allocated or consumed; current `brupdate.taken` is not a predictor update contract. | High: tables/history could be written with no durable fetch correlation or repair owner. | High: table reads, history muxing, and target selection enter the Frontend control path. | High: aliasing, update timing, wrong-path repair, RAS, redirect, reset, and RTL tests are all required. |
| FTQ first | Bundle allocation/index propagation; entry schema; wrap/full/backpressure; redirect pruning; lifecycle through Decode/ROB/branch completion | Future predictor update/repair and redirect correlation. Current branch completion is a plausible future consumer but no FTQ contract exists yet. | Medium if implemented as storage only; low for a prerequisite review that first defines consumers and lifetime. | Medium to high depending on payload; a narrow index and bounded metadata contract can be reviewed before storage implementation. | Medium to high: ownership and lifetime can be verified independently before prediction policy. |
| ICache first | Hit/miss/refill interface; memory-side protocol; fault ordering; invalidation/FENCE.I; replacement and outstanding-miss policy | Existing Frontend request interface could consume hits, but the repository has no refill/memory-side owner. | Medium to high: cache arrays without refill and invalidation semantics are not a usable ICache. | High: tags/data arrays and hit selection enter fetch timing and consume BRAM. | High: hit/miss/refill, faults, reset, invalidation, backpressure, and stale-response interaction are required. |

## Recommendation

`NEXT_FRONTEND_STAGE=FTQ_PREREQUISITE_REVIEW`

This recommendation does not default to FTQ implementation. The review must establish the missing correlation boundary that Predictor requires and that current branch completion can eventually consume. Predictor-first would create history/table state without a durable fetch-bundle owner. ICache-first is independently possible but currently lacks a memory-side refill and invalidation contract and does not resolve control-flow metadata ownership.

## Gate 5.4 Review Scope

The prerequisite review may define only:

- what constitutes an FTQ fetch bundle with packet width 2 and Decode width 1;
- index allocation, propagation, wrap, and invalidation lifetime;
- minimum metadata needed by future prediction update and redirect repair;
- the consumer and producer for each field;
- backpressure and flush ordering;
- an isolated verification and PPA plan.

It must reject metadata with no named consumer and must not add FTQ arrays, predictor tables, RAS state, ICache arrays, or product behavior.

```text
READY_FOR_GATE5_4_PREREQUISITE_REVIEW=true
READY_FOR_FTQ_IMPLEMENTATION=false
READY_FOR_PREDICTOR_IMPLEMENTATION=false
READY_FOR_ICACHE_IMPLEMENTATION=false
```
