# G6.0 L1R Pause Checkpoint

```text
PAUSE_REASON=ROOT_FILESYSTEM_SPACE_PRESSURE
L1R_TASK_STATE=PAUSED_READY_TO_RESUME_WITH_SPACE_TARGET_LIMITATION
BASE_COMMIT=9934c02af302a5be66da46a068c81730f6c50ad5
BRANCH=gate3.8-rtl-verification
L1R_CURRENT_SOURCE_HASH=b3cd4bdb13dc00e4b0293bc506268005bb8633b0e0036fc5e18f39314bbab618
MERGED_SOURCE_SHA256=76d78e9052344a0b1e06c0e723c473d37e0b2dd7679a2ab3b8fbbeed9f1dff9c
SRC_BOOM_ALL_SHA256=d6f885632ddd445729adda8148ea256e67683ccc8e7f2b10c9951e915d92c76c
INDEX_EMPTY=true
L1_IMPLEMENTATION_PRESERVED=true
L1_MEMORY_BEHAVIOR_CHANGED=false
PAUSE_CLEANUP_STATE=COMPLETE_WITH_SPACE_TARGET_LIMITATION
```

## Product Diff Paths

```text
include/boom_config.hpp
include/boom_state.hpp
include/boom_types.hpp
include/reset.hpp
src/branch.cpp
src/commit.cpp
src/completion.cpp
src/lsu.cpp
src/reset.cpp
src/synth_module_tops.cpp
src/boom_core_merged.cpp (generated and byte-exact with scripts/generate_merged.sh)
src/boom_all.cpp (pre-existing historical dirty file; excluded and untouched)
```

## Frozen Accepted Results

```text
CONFIG_4_4_STATUS=PASS
CONFIG_8_8_STATUS=PASS
CONFIG_16_16_STATUS=PASS
CONFIG_4_16_STATUS=PASS
CONFIG_16_4_STATUS=PASS
EXHAUSTIVE=PASS_137257_STATES_ERRORS_0
RANDOM=PASS_256_X_4096_ERRORS_0
MINIMAL_LSU=PASS_14_OF_14
W3=PASS_18_OF_18
CURRENT_SOURCE_RTL_LSU_SUBSET=PASS_17_OF_17
ASYMMETRIC_4_16_RTL=PASS
ASYMMETRIC_16_4_RTL=PASS
CANONICAL_SYNTH=PASS_9_OF_9
PARAMETER_SYNTH=PASS_ALL_FIVE_CONFIGS
CONFIG_8_8_PPA=201186_LUT_45262_FF_16_BRAM_3_DSP_6.341NS
REUSE_EXISTING_PASS_EVIDENCE_BY_SOURCE_HASH=true
```

## L1R Progress At Pause

The pause request's listed incomplete statuses predated work completed in this session. The actual durable evidence present at pause is recorded here to avoid wasting time after resume.

```text
FOCUSED_RTL=INCOMPLETE_COMMAND_ENGINE_NATIVE_PASS_RTL_SYNTHESIS_ABORTED
FOCUSED_RTL_ACCEPTED_COUNT=17_OF_80
PF3_FULL_CORE=PASS_12_OF_12_CURRENT_L1_SOURCE
PF5_FULL_CORE=PASS_12_OF_12_CURRENT_L1_SOURCE
PF5_MANDATORY_FAULTS=PASS_2_OF_2
PF6_INTEGRATED=PASS_12_OF_12_CURRENT_L1_SOURCE
PF6_MANDATORY_FAULTS=PASS_2_OF_2
W4=PASS_13_OF_13_CURRENT_SOURCE
W4_TIMEOUT_CLASS=W4_RECOMPILE_PER_CASE_OVERHEAD
CURRENT_COMPATIBLE_RVC_NATIVE=PASS_11_OF_11
CURRENT_COMPATIBLE_RVC_CSIM=PASS_11_OF_11
CURRENT_COMPATIBLE_RVC_RTL=PASS_11_OF_11_CURRENT_L1_SOURCE
DURABLE_ACCEPTED_ARTIFACTS_VERIFIED=false
```

Fresh default 8/8 canonical synthesis reproduced `201186 LUT`, `45262 FF`, `16 BRAM`, `3 DSP`, and `6.341 ns` only with the accepted `BOOM_FTQ_STORAGE_LUTRAM` and `BOOM_PREDICTOR_STORAGE_LUTRAM` flags. PF3 and RVC RTL shared that canonical build. PF5 and PF6 integrated shared one fresh `boom_core_pf4_rtl_top` build. Their raw projects are ephemeral and may be rebuilt from this source hash; durable matrices, traces, and logs remain under this report directory.

## Resume Strategy

1. Verify the current source hash remains `b3cd4bdb13dc00e4b0293bc506268005bb8633b0e0036fc5e18f39314bbab618`.
2. Reuse all source-identical passed evidence listed above.
3. Rebuild one shared default 8/8 current-source RTL only if a remaining suite requires it.
4. Complete focused RTL to exactly 80/80 using the compact command-engine infrastructure.
5. Reuse PF3 12/12 durable evidence if its source and runner hashes remain unchanged.
6. Reuse PF5 12/12 and mandatory fault evidence if hashes remain unchanged.
7. Reuse PF6 integrated 12/12 and mandatory fault evidence if hashes remain unchanged.
8. Reuse W4 13/13 evidence if hashes remain unchanged.
9. Reuse current-compatible RVC native/CSim/RTL 11/11 evidence if hashes remain unchanged.
10. Finalize durable artifact hashes and provenance matrices.
11. Perform final L1 acceptance checks.

```text
NEXT_REQUIRED_ACTION_AFTER_RESUME=CONTINUE_G6_0_L1_VALIDATION_CLOSURE
```

Post-cleanup available space is `14282964992` bytes (`14G` from `df -h`, 70% used). This is below the requested 15 GB minimum; cleanup stopped rather than delete unrelated general user caches. See `l1r_cleanup_log.md` for the deletion ledger and integrity checks.

Do not rerun exhaustive, random, canonical 9-top synthesis, five-config parameter synthesis, minimal LSU, W3, the existing 17-case RTL subset, or asymmetric RTL unless the source hash changes or evidence freshness fails.
