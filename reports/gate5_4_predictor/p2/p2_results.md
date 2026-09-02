# Gate 5.4 P2 Standalone Predictor Foundation Results

## Verdict

The standalone predictor foundation is verified. It composes the P1 canonical predecoder with a PC-indexed 2-bit BIM and a blocking held-response interface. It is not connected to the product Frontend, FTQ, Execute, or Commit paths, so no product or full-core PPA claim is made.

```text
GATE5_4_P2_PREDICTOR_FOUNDATION_VERIFIED=true
GATE5_4_P2_PREDICTOR_VERIFIED=true

BASELINE_HEAD=ef051ca4e3673d663f9d10b55029c956fbc0052a
BASELINE_BRANCH=gate3.8-rtl-verification
DIRTY_BASELINE_CAPTURED=true
PROTECTED_SOURCE_HASHES_IDENTICAL=true
SRC_BOOM_ALL_CPP_HASH_IDENTICAL=true
PREDICTOR_CORE_API=include/predictor.hpp,src/predictor.cpp
PREDICTOR_STANDALONE_ONLY=true

PREDICTOR_FOUNDATION=STATIC_TARGET_PREDECODE_PLUS_BIM
PREDICTOR_STATEFUL=true
PREDICTOR_LATENCY_MODEL=FIXED_1_CYCLE_BLOCKING_PACKET
PREDICTOR_ENTRIES=256
PREDICTOR_INDEX_WIDTH=8
PREDICTOR_STORAGE=LUTRAM
PREDICTOR_RESET_POLICY=LAZY_VALID_INIT
PREDICTOR_COUNTER_BITS=2
BIM_COUNTER_BITS=2
PREDICTOR_INITIAL_STATE=WEAK_NOT_TAKEN
PREDICTOR_UPDATE_COLLISION_POLICY=UPDATE_FORWARD_NEW_VALUE
BIM_INDEX_POLICY=PC_SHIFT_RIGHT_1_LOW_LOG2_ENTRIES_BITS
BIM_SAME_INDEX_CONFLICT_POLICY=UPDATE_FORWARD_NEW_VALUE
BIM_RESET_POLICY=LAZY_VALID_INIT
P2_CANONICAL_BIM_ENTRIES=256
P2_CANONICAL_BIM_STORAGE=LUTRAM
P2_CANONICAL_BIM_RESET=LAZY_VALID_INIT

PREDICTOR_INDEX_FORMULA=(pc>>1)&(entries-1)
PREDICTOR_UPDATE_POLICY=COMMIT_QUALIFIED_CONDITIONAL_ONLY
PREDICTOR_GENERATION_VALIDATED=true
PREDICTOR_METADATA_INDEX_VALIDATED=true
PREDICTOR_JAL_STATIC_TAKEN=true
PREDICTOR_JALR_PREDICTION_VALID=false
PREDICTOR_NO_CFI_PREDICTION_VALID=false

PREDICTOR_LOGICAL_LATENCY_STEPS=1
PREDICTOR_ACCEPT_CALL=N
PREDICTOR_RESPONSE_CALL=N+1
PREDICTOR_BLOCKING=true
PREDICTOR_RESPONSE_HOLD=true
PREDICTOR_SAME_CYCLE_TURNOVER=false
HLS_TOP_BEST_CASE_LATENCY=3
HLS_TOP_MIN_II=4
HLS_TOP_PIPELINED=false

DIRECTED_STATUS=PASS_5008_OF_5008
COMPOSITION_STATUS=PASS_10620_OF_10620
RANDOM_STATUS=PASS_75497472_CHECKS_ERRORS_0
FOCUSED_RTL_STATUS=PASS_164_OF_164
RANDOM_SEEDS=256
RANDOM_CYCLES_PER_SEED=8192
RANDOM_DEPTHS=4
RANDOM_PREDICTION_ERRORS=0
RANDOM_COUNTER_ERRORS=0
RANDOM_TARGET_ERRORS=0
RANDOM_HANDSHAKE_ERRORS=0
RANDOM_DROP_ERRORS=0
RANDOM_DUPLICATE_ERRORS=0
RANDOM_STABILITY_ERRORS=0
RANDOM_RESET_ERRORS=0
RANDOM_CONFLICT_ERRORS=0
RANDOM_IDENTITY_ERRORS=0
RANDOM_ERRORS=0
DEPTH_SWEEP_CSYNTH_STATUS=PASS_4_OF_4
STORAGE_EXPERIMENT_STATUS=PASS_3_OF_3
RESET_EXPERIMENT_STATUS=PASS_2_OF_2

P1_RERUN_DIRECTED_STATUS=PASS_994_ERRORS_0
P1_RERUN_RVC_STATUS=PASS_65536_ERRORS_0
P1_RERUN_PACKET_STATUS=PASS_256_X_4096_ERRORS_0
P1_RERUN_RANDOM_STATUS=PASS_1000000_ERRORS_0

CANONICAL_SYNTH_LUT=684
CANONICAL_SYNTH_FF=465
CANONICAL_SYNTH_BRAM=0
CANONICAL_SYNTH_DSP=0
CANONICAL_SYNTH_PERIOD_NS=2.989
BIM_64_PPA=LUT_658_FF_449_BRAM_0_DSP_0_PERIOD_2.989_NS
BIM_128_PPA=LUT_653_FF_441_BRAM_1_DSP_0_PERIOD_4.109_NS
BIM_256_PPA=LUT_658_FF_449_BRAM_1_DSP_0_PERIOD_4.109_NS
BIM_512_PPA=LUT_664_FF_457_BRAM_1_DSP_0_PERIOD_4.109_NS

S0_NO_NEW_SCHEDULING_DIRECTIVE=true
STORAGE_BIND_STORAGE_ONLY=true

PRODUCT_SOURCE_CHANGED=false
PRODUCT_FRONTEND_CHANGED=false
PRODUCT_DECODE_CHANGED=false
PRODUCT_BRANCH_CHANGED=false
FULL_CORE_PPA_NOT_APPLICABLE=true
BTB_IMPLEMENTED=false
RAS_IMPLEMENTED=false
GHR_IMPLEMENTED=false
TAGE_IMPLEMENTED=false
FTQ_IMPLEMENTED=false
ICACHE_IMPLEMENTED=false
READY_FOR_GATE5_4_F1_FTQ_FOUNDATION=true
READY_FOR_GATE5_4_F1_STANDALONE_FTQ=true
READY_FOR_STANDALONE_FTQ_IMPLEMENTATION=true
READY_FOR_PRODUCT_PREDICTOR_INTEGRATION=false
READY_FOR_PRODUCT_FTQ_INTEGRATION=false
READY_FOR_PRODUCT_PREDICTOR_IMPLEMENTATION=false
READY_FOR_PRODUCT_FTQ_IMPLEMENTATION=false
READY_FOR_PREDICTOR_IMPLEMENTATION=false
READY_FOR_FTQ_IMPLEMENTATION=false
READY_FOR_ICACHE_IMPLEMENTATION=false
CURRENT_PREDICTOR_IMPLEMENTED=false
CURRENT_FTQ_IMPLEMENTED=false
CURRENT_ICACHE_IMPLEMENTED=false
PREDICTOR_IMPLEMENTED=false
FTQ_IMPLEMENTED=false
ICACHE_IMPLEMENTED=false
READY_FOR_FULL_LSU_IMPLEMENTATION=false
READY_FOR_OFFICIAL_GATE_3=false

GATE5_3_FETCH_BUFFER_VERIFIED=true
GATE5_4_F0_FTQ_PREREQUISITES_REVIEWED=true
GATE5_4_P0_PREDICTOR_INTERFACE_REVIEWED=true
GATE5_4_P1_CFI_PREDECODE_VERIFIED=true
M009=PARTIALLY_VERIFIED
M014=VERIFIED
```

## Functional Evidence

- Directed tests pass 5,008 checks at all four depths with zero failures.
- P1 predecode/predictor composition passes 10,620 checks and uses the canonical `predecode_cfi` and `decompress_rvc` implementations.
- Random differential testing covers 256 seeds x 8,192 cycles x four depths, totaling 75,497,472 checks. Prediction, counter, target, handshake, drop, duplicate, stability, reset, conflict, and identity errors are all zero.
- Generated canonical LUTRAM RTL passes 164/164 XSim checks, including call-to-call acceptance/response timing, held responses, reset, update collisions, and identity filtering.

## Contract

Conditional branches use `index=(pc>>1)&(entries-1)`. Invalid entries read as logical counter `01` (weak not-taken). Only valid, commit-qualified conditional updates with the active generation and PC-derived metadata index train. A same-index update and lookup observes the forwarded new counter value. JAL is statically taken when predecode supplies its direct target; JALR and no-CFI requests return `prediction_valid=false`.

The logical API accepts a request on step N and presents its response on step N+1. A response remains stable under backpressure; while pending, no request is accepted, including the response-consume step. This one-step logical contract must not be confused with physical HLS wrapper scheduling. The generated `ap_ctrl_hs` top reports best-case transaction latency 3 and minimum II 4, and its reset transaction contains a 256-iteration loop.

## PPA

Actual standalone XML for the selected 256-entry LUTRAM configuration reports 684 LUT, 465 FF, 0 BRAM, 0 DSP, 2.989 ns estimated period, best-case latency 3, minimum II 4, and `PipelineType=no`. These are HLS estimates for the standalone wrapper, not post-route timing or a full-core delta.

## Preservation

HEAD is `ef051ca4e3673d663f9d10b55029c956fbc0052a` on `gate3.8-rtl-verification`. The full dirty baseline is retained in `git_status_before.txt`. Protected source hashes before and after are identical, including legacy excluded `src/boom_all.cpp`. P2's core API consists only of standalone `include/predictor.hpp` and `src/predictor.cpp`; no product source changed.
