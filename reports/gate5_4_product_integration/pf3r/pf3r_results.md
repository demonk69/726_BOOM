# Gate 5.4 PF3A Product FTQ Acceptance Completion

## Verdict

PF3A completes PF3 acceptance. Twelve independent assembled product programs
pass native and Vitis CSim with exact architectural signatures and identical
FTQ event counts. The same programs pass current-source canonical full-core RTL
with exact signatures, wrong-path non-commit checks, exception cause/PC, FTQ
allocation/reclaim/redirect, wrap/reuse, generation-retry observability, and a
real mid-stream RTL reset.

The long redirect/reuse program exposed and closed a canonical liveness defect:
a retire event rejected during redirect priority was cleared rather than retried,
leaving the redirect owner live and eventually filling all 32 FTQ entries. The
minimal fix retains that retire event for the next FTQ step. All mandatory PF3
and preservation suites were rerun after the fix. Focused PF2 RTL also exposed
stale prediction-result observability on a later non-conditional JALR. Resetting
those result fields when constructing each new packet closes that regression
without erasing the accepted conditional result in its admission cycle.

```text
GATE5_4_PF3_FTQ_ATOMIC_INTEGRATION_VERIFIED=true
PRODUCT_FTQ_INTEGRATED=true
PF3R_PPA_REPAIR_VERIFIED=true
PF3A_REDIRECT_RETIRE_RETRY_FIX_VERIFIED=true
PF3A_PREDICTION_OBSERVABILITY_RESET_VERIFIED=true
PF3R_GENERATION_SIDECAR_SOURCE_AUDIT=PASS
F1_FTQ_ENTRY_BITS=211
PF3_FTQ_DEPTH=32
PF3_FTQ_STORAGE=LUTRAM
PF3_FTQ_RESET_POLICY=CONTROL_ONLY
PF3_FTQ_INDEX_BITS=5
PF3_FTQ_GENERATION_BITS=32
PF3_FTQ_REFERENCE_BITS=40
PF3_FB_FTQ_ATOMICITY_POLICY=ONE_NONEMPTY_FINAL_PACKET_ONE_ATOMIC_FB_ENQUEUE_AND_EXACTLY_ONE_FTQ_ALLOCATION
PF3_FTQ_TURNOVER_POLICY=RECLAIM_AT_MOST_ONE_ZERO_LIVE_HEAD_THEN_ALLOW_SAME_STEP_ALLOCATION
PRODUCT_CONDITIONAL_PREDICTION_STEERING_ENABLED=false
PRODUCT_BRANCH_PREDICTION_RECOVERY_INTEGRATED=false
PRODUCT_COMMIT_BIM_TRAINING_INTEGRATED=false
DIRECTED_STATUS=PASS_481322_OF_481322
SMALL_STATE_EXHAUSTIVE_STATUS=PASS_DEPTH2_LENGTH6_15625_AND_DEPTH4_LENGTH4_625
RANDOM_STATUS=PASS_256_X_8192_ERRORS_0
LONG_RUN_STATUS=PASS_1000000_STEPS_LEAK_ERRORS_0_MAX_OCCUPANCY_32
CSIM_STATUS=PASS
FOCUSED_RTL_STATUS=PASS_100_OF_100_CASES_700_CHECKS_CURRENT_SOURCE
PROGRAM_NATIVE_STATUS=PASS_12_OF_12
PROGRAM_CSIM_STATUS=PASS_12_OF_12_EXACT_EVENT_MATCH
FULL_CORE_RTL_STATUS=PASS_12_OF_12_CURRENT_SOURCE_EXACT_SIGNATURE_AND_FTQ_EVENTS
PF2_FOCUSED_RTL_PRESERVATION=PASS_116_OF_116_CURRENT_SOURCE
PF2_FULL_CORE_RTL_PRESERVATION=PASS_11_OF_11_CURRENT_CANONICAL_RTL
PF1_FOCUSED_EXCEPTION_RTL_PRESERVATION=PASS_64_OF_64_CURRENT_SOURCE
PF1_FULL_CORE_EXCEPTION_RTL_PRESERVATION=PASS_8_OF_8_CURRENT_CANONICAL_RTL
GATE5_3_PACKET_RANDOM_PRESERVATION=PASS_256_X_4096_ERRORS_0
GATE5_3_B3I_FULL_CORE_NATIVE_PRESERVATION=PASS_6_OF_6
GATE5_3_B3I_FULL_CORE_RTL_PRESERVATION=PASS_6_OF_6_CURRENT_CANONICAL_RTL
BACKEND_M3C_PRESERVATION=PASS_DIRECTED_RANDOM_AND_15_OF_15_PROGRAMS
FULL_CORE_LUT=198881
FULL_CORE_FF=45443
FULL_CORE_BRAM=16
FULL_CORE_DSP=3
FULL_CORE_PERIOD_NS=6.341
LUT_DELTA_FROM_PF2=18074
LUT_DELTA_PERCENT_FROM_PF2=9.996
FF_DELTA_FROM_PF2=7752
BRAM_DELTA_FROM_PF2=0
PERIOD_DELTA_FROM_PF2=0.000
PPA_REVIEW_REQUIRED=false
GATE5_4_PF3_PPA_BLOCKER=false
CORE_CYCLE_PIPELINED=false
SRC_BOOM_ALL_EXCLUDED=true
REPOSITORY_HYGIENE_PRESERVED=true
READY_FOR_GATE5_4_PF4_BRANCH_PREDICTION_RECOVERY=true
```

PF4 has not been started. Conditional prediction steering, predicted-vs-actual
recovery, Commit BIM training, BTB, RAS, GHR, TAGE, and ICache remain disabled
or absent as required.

## Evidence

- `pf3r_program_matrix.csv`: native/CSim signatures and exact event comparison.
- `pf3r_full_core_rtl_matrix.csv`: current canonical RTL signature and kill checks.
- `program_ftq_coverage.csv`: per-program RTL FTQ observability.
- `verification_matrix.csv`: mandatory suite completion.
- `pf3a_redirect_retire_fix.md`: defect, fix, and regression rationale.
- `pf3a_provenance.md`: current-source and generated-artifact hashes.
