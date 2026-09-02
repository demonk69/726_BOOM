# Gate 5.4 F1 Standalone FTQ Foundation Results

## Verdict

The standalone parameterized FTQ foundation is verified. It is not connected
to Frontend, Fetch Buffer, Decode, ROB, Commit, Execute, branch recovery, or
the product predictor. No full-core PPA change is claimed.

```text
GATE5_4_F1_STANDALONE_FTQ_VERIFIED=true

FTQ_ENTRY_GRANULARITY=PER_FETCH_PACKET
F1_CANONICAL_FTQ_DEPTH=32
F1_CANONICAL_FTQ_STORAGE=LUTRAM
F1_CANONICAL_FTQ_RESET_POLICY=CONTROL_ONLY
F1_FTQ_ENTRY_BITS=211
F1_TOTAL_STATE_BITS=6800
FTQ_INDEX_BITS=5
FTQ_GENERATION_BITS=32
PREDICTOR_GENERATION_BITS=32
F1_LIVE_TRACKING_POLICY=LANE_MASK
FTQ_STALE_REFERENCE_POLICY=PER_ENTRY_32_BIT_ALLOCATION_GENERATION_PLUS_LANE
FTQ_ALLOCATION_POINT=FETCH_BUFFER_ENQUEUE
FTQ_RECLAIM_POLICY=PER_FTQ_LIVE_UOP_COUNT

DIRECTED_STATUS=PASS_275944648_CHECKS_ERRORS_0
SMALL_STATE_EXHAUSTIVE_STATUS=PASS_DEPTH2_LENGTH6_1948717_SEQUENCES_ERRORS_0
RANDOM_SEEDS=256
RANDOM_CYCLES_PER_SEED=16384
RANDOM_DEPTHS=4
RANDOM_ERRORS=0
PREDICTOR_FTQ_COMPOSITION_STATUS=PASS_11080_OF_11080
FOCUSED_RTL_STATUS=PASS_193_OF_193

FTQ_DEPTH8_PPA=LUT_3047_FF_1324_BRAM_0_DSP_0_PERIOD_3.957_NS
FTQ_DEPTH16_PPA=LUT_3053_FF_1085_BRAM_4_DSP_0_PERIOD_4.485_NS
FTQ_DEPTH32_PPA=LUT_3040_FF_974_BRAM_6_DSP_0_PERIOD_4.348_NS
FTQ_DEPTH64_PPA=LUT_3060_FF_991_BRAM_6_DSP_0_PERIOD_4.210_NS
CANONICAL_FTQ_LUT=3006
CANONICAL_FTQ_FF=1358
CANONICAL_FTQ_BRAM=0
CANONICAL_FTQ_DSP=0
CANONICAL_FTQ_PERIOD_NS=3.788

S0_NO_NEW_SCHEDULING_DIRECTIVE=true
FULL_CORE_PPA_NOT_APPLICABLE=true
PRODUCT_FRONTEND_CHANGED=false
PRODUCT_DECODE_CHANGED=false
PRODUCT_ROB_CHANGED=false
PRODUCT_BRANCH_CHANGED=false
REPOSITORY_HYGIENE_PRESERVED=true
BOOM_BUILD_ROOT=/tmp/boom_hls

READY_FOR_GATE5_4_PF0_PRODUCT_INTEGRATION_REVIEW=true
READY_FOR_PRODUCT_PREDICTOR_INTEGRATION=false
READY_FOR_PRODUCT_FTQ_INTEGRATION=false
CURRENT_PREDICTOR_IMPLEMENTED=false
CURRENT_FTQ_IMPLEMENTED=false
CURRENT_ICACHE_IMPLEMENTED=false
BTB_IMPLEMENTED=false
RAS_IMPLEMENTED=false
GHR_IMPLEMENTED=false
TAGE_IMPLEMENTED=false
ICACHE_IMPLEMENTED=false
READY_FOR_FULL_LSU_IMPLEMENTATION=false
READY_FOR_OFFICIAL_GATE_3=false
M009=PARTIALLY_VERIFIED
M014=VERIFIED
```

## Functional Evidence

The directed suite covers allocation, empty/illegal masks, capacity, ordered
reclaim, simultaneous reclaim/allocation, wrap, lane-level retire/squash,
duplicate rejection, redirect, reset, stale references, and metadata. Bounded
depth-2 exploration covers every event sequence through length 6. Persistent
random differential verification runs 256 seeds by 16,384 cycles at each of
four depths with every named error counter at zero. P1 predecode, P2 predictor,
and F1 FTQ composition retains BIM metadata and the independent P2 predictor
generation bit-exactly. Generated canonical LUTRAM RTL passes 193 checks.

## PPA And Scope

All seven standalone csynth candidates completed. Depth 32 remains canonical
because it covers the frozen 32-entry ROB worst case. LUTRAM avoids AUTO's six
BRAMs and forced BRAM's fifteen BRAMs while producing the best period.
CONTROL_ONLY reset avoids a 220-LUT full-payload reset cost. Wrapper latency
and II are diagnostic HLS transaction properties, not an integrated core-cycle
contract.

Protected product hashes are identical. The only scheduling-related pragmas
added are conditional `bind_storage` directives for the storage experiment.
`src/boom_all.cpp` remains pre-existing dirty and excluded. The repository is
13 GB due to retained historical evidence, `/tmp/boom_hls` is 24 KB after
cleanup, and the workspace checker reports no size warning.
