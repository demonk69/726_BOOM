# Gate 5.4 P0 Predictor Interface Prerequisite Review

## Verdict

The predictor/predecode/FTQ interface and metadata lifecycle review is complete. Product source is unchanged. The actual nested Git worktree `/home/lab_726/boom/hls_boom` is on the requested branch at the accepted Gate 5.3 commit. Historical dirty state and the uncommitted F0 artifacts were preserved without cleanup or commit.

```text
GATE5_4_P0_PREDICTOR_INTERFACE_REVIEWED=true

RECOMMENDED_PREDICTOR_REQUEST_POINT=PARCEL_PREDECODE_STAGE
PREDECODE_REQUIRED=true
FIRST_CFI_POLICY=EARLIEST_VALID_CFI_IN_PC_ORDER

BTB_REQUIRED_FOR_FOUNDATION=false
RAS_REQUIRED_FOR_FOUNDATION=false
GLOBAL_HISTORY_REQUIRED_FOR_FOUNDATION=false
PREDICTOR_GENERATION_TOKEN=true
RECOMMENDED_PREDICTOR_LATENCY_MODEL=FIXED_1_CYCLE_BLOCKING_PACKET
PREDICTOR_FOUNDATION_RECOMMENDATION=STATIC_TARGET_PREDECODE_PLUS_BIM
RECOMMENDED_GATE5_4_IMPLEMENTATION_ORDER=IMPLEMENT_PREDECODE_THEN_PREDICTOR_THEN_FTQ

READY_FOR_PREDECODE_IMPLEMENTATION=true
READY_FOR_STANDALONE_PREDICTOR_IMPLEMENTATION=true
READY_FOR_STANDALONE_FTQ_IMPLEMENTATION=true
READY_FOR_PRODUCT_PREDICTOR_INTEGRATION=false
READY_FOR_PRODUCT_FTQ_INTEGRATION=false

CURRENT_FTQ_IMPLEMENTED=false
CURRENT_PREDICTOR_IMPLEMENTED=false
CURRENT_ICACHE_IMPLEMENTED=false
FTQ_IMPLEMENTED=false
PREDICTOR_IMPLEMENTED=false
ICACHE_IMPLEMENTED=false

READY_FOR_ICACHE_IMPLEMENTATION=false
READY_FOR_FULL_LSU_IMPLEMENTATION=false
READY_FOR_OFFICIAL_GATE_3=false

GATE5_3_FETCH_BUFFER_VERIFIED=true
GATE5_4_F0_FTQ_PREREQUISITES_REVIEWED=true
DECODE_WIDTH=1
DISPATCH_WIDTH=1
COMMIT_WIDTH=1
CORE_CYCLE_PIPELINED=false
SRC_BOOM_ALL_CPP_EXCLUDED=true

M009=PARTIALLY_VERIFIED
M014=VERIFIED
```

## Required Answers

1. Predictor request is issued at the completed parcel/packet predecode boundary, after the earliest valid CFI and its exact PC are known and before Fetch Buffer enqueue.
2. IMEM issue lacks bytes, RVC boundaries, carry completion, CFI slot/type, and a correct BIM branch-PC index. Waiting until enqueue is too late to atomically mask a younger lane or choose next PC.
3. Predecode is required. Current packet construction creates complete canonical instructions but stores no frontend CFI classification.
4. Minimum logical predecode output is valid, instruction PC/length, CFI valid/type, conditional/JAL/JALR, call/return, and static-target valid/target.
5. Select the earliest valid CFI in PC order. If lane 0 is predicted taken, effective mask becomes `01` and lane 1 never enters Fetch Buffer/FTQ; if lane 1 is selected, both lanes remain `11`.
6. Conditional target is predecoded as instruction PC plus sign-extended B immediate, including decompressed C.BEQZ/C.BNEZ.
7. JAL target is instruction PC plus sign-extended J immediate, including decompressed C.J.
8. JALR target depends on runtime `rs1`: `(rs1+I-immediate)&~1`. It needs Execute, BTB-like dynamic prediction, or RAS for recognized returns; it is not purely static.
9. Minimal predictor response is prediction-valid, taken, target-valid/target, CFI lane/type, opaque metadata token, and generation.
10. Foundation FTQ stores prediction validity/direction/target, selected CFI lane/type, update token, and generation in addition to F0 base PC/mask/live count. BIM counters stay predictor-private; GHR/RAS snapshots are deferred.
11. Execute performs correction immediately; only committed nonsquashed control instructions train. FTQ reclaim follows exact live-count decrement and any pending update acceptance.
12. Future types are direction, target, CFI-slot, and return mispredict. Unsupported actual-taken flow is `NO_PREDICTION_REDIRECT`, not a mispredict.
13. BTB is not foundation prerequisite because direct branch/JAL targets are static. JALR target prediction is deferred.
14. RAS is not foundation prerequisite. Add it after stable call/return classification, target prediction, generation, and recovery semantics.
15. GHR is not foundation prerequisite. BIM needs no history and minimal FTQ has no history snapshot.
16. Reuse Frontend epoch as predictor generation and also match the sole pending request token. Drain/drop response after reset/redirect. Consolidate the current duplicate epoch increment before product integration.
17. Use fixed one-cycle blocking packet latency: retain one completed packet until response, then finalize mask/next PC and atomically enqueue/allocate. No late redirect.
18. Standalone predictor implementation is ready against synthetic exact-CFI requests, generation tests, and commit updates; product integration is not ready.
19. Standalone FTQ remains ready per F0, with update-pending lifetime clarified before predictor integration.
20. Product predictor integration is not ready because predecode/transport do not exist, actual outcome is overloaded as mispredict, redirect epoch ownership is duplicated, and recovery interfaces are incomplete.
21. Product FTQ integration is not ready because pointer transport, predictor metadata producer, update/reclaim ordering, and exception recovery are incomplete.
22. Next concrete gate is Gate 5.4 P1 standalone CFI Predecode, followed by P2 standalone Predictor Foundation and P3 focused RTL/PPA.

## Source Findings

- `BoomCoreState` has no predictor or FTQ state (`include/boom_state.hpp:267-289`). Configuration/struct placeholders are not implementations.
- Fetch packet construction supplies exact PC, canonical instruction and `is_rvc`, including cross-word completion (`src/fetch_packet.cpp:73-135`).
- Current Decode first classifies JAL/JALR/branch (`src/decode.cpp:129-159`).
- RVC expands C.J, C.BEQZ/C.BNEZ, C.JR/C.JALR into canonical encodings (`src/rvc.cpp:157-170,190-201`).
- Execute computes actual targets/outcomes and names actual taken `mispredict` (`src/execute.cpp:155-162`).
- Branch completion copies that value into `BranchUpdate.taken` and redirects immediately (`src/branch.cpp:303-319`).
- Commit has no predictor update and terminal exception handling lacks architectural redirect (`src/commit.cpp:31-119`).

## Protection Check

Only this P0 report directory and the two requested docs changed. No tests or synthesis were run because this is an architecture audit. No Predictor/FTQ/ICache or canonical product behavior was implemented; widths, directives, PPA baseline, and excluded `src/boom_all.cpp` remain untouched.
