# Gate 5.4 Predictor/FTQ Roadmap

`RECOMMENDED_GATE5_4_IMPLEMENTATION_ORDER=IMPLEMENT_PREDECODE_THEN_PREDICTOR_THEN_FTQ`

## Dependency Graph

```text
Gate 5.3 packet/RVC/carry (accepted)
              |
              v
P1 standalone CFI predecode
              |
              v
P2 standalone static-target + BIM predictor
              |
              v
P3 predictor focused RTL/PPA and latency acceptance
              |
              v
F1 standalone depth-32 per-packet FTQ
              |
              v
PF1 atomic predecode/prediction + FB/FTQ metadata integration
              |
              v
PF2 actual/predicted split + redirect/update/recovery integration
              |
              +------> exception/architectural redirect ownership closure
              |
              v
PF3 full-core RTL/PPA acceptance
```

F1 is already independently implementable from F0 and may be developed in parallel after P0, but it must not enter product before P1-P3 freeze predictor metadata width, pending response, and generation semantics. The recommended value order remains predecode, predictor, then FTQ.

## Gates

| Gate | Scope | Exit condition |
|---|---|---|
| P1 | standalone combinational CFI predecode over complete `FetchInstruction` inputs | directed and randomized RV64C/non-RVC classification, immediate, call/return, cross-word-completed vectors; no product integration |
| P2 | standalone 2-bit BIM with static-target request/response model | deterministic reset/init policy, generation rejection, commit-qualified update protocol |
| P3 | focused generated RTL and 64/128/256/512-entry PPA | select entries/storage and confirm fixed one-cycle latency; no full-core integration |
| F1 | standalone depth-32 per-packet FTQ from F0 plus update-pending lifetime clarification | allocate/retire/squash/reclaim/generation tests; no MicroOp change |
| PF1 | integrate predecode response, effective packet mask, atomic FB+FTQ allocation, uop pointer | preserve all Gate 5.3 packet/fault/carry tests; product integration authorization required |
| PF2 | split actual outcome from mispredict; execute correction, commit training, exception/recovery lifecycle | all direction/target/slot/no-prediction cases and stale owner checks |
| PF3 | full-core native/csim/RTL/PPA acceptance | no width expansion/directives; accepted functional and guardrail evidence |

BTB is a later target-prediction gate after foundation measurement, especially for JALR. RAS follows stable JALR call/return classification and redirect recovery. GHR/TAGE follows proven FTQ update/snapshot lifecycle. ICache remains outside Gate 5.4 readiness.
