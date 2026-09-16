# G6.0 L1R Resume Outcome

Historical checkpoint record: this report captures the first resume outcome
before the focused RTL redesign reached `80/80`. It is validation-architecture
provenance, not the current gate verdict. See `l1_results.md` for final status.

## Status

```text
L1R_RESUME_ATTEMPTED=true
CHECKPOINT_L1R_STATUS=BLOCKED
CHECKPOINT_G6_0_L1_PARAMETERIZED_LQ_SQ_VERIFIED=false
CHECKPOINT_FOCUSED_RTL_ACCEPTED=17_OF_80
FOCUSED_RTL_REQUIRED=80_OF_80
CHECKPOINT_NEXT_REQUIRED_ACTION=REDESIGN_FOCUSED_RTL_VALIDATION_ARCHITECTURE
```

All source-identical preservation evidence was reused and verified. PF3, PF5, PF6 integrated, W4, and current-compatible RVC were complete. At this checkpoint, the only remaining Gate 6.0 L1 blocker was the focused RTL threshold.

## Focused RTL Attempts

| Attempt | Result | Runtime/workspace | Evidence |
|---|---|---|---|
| Compact command engine, full persistent `BoomCoreState` | FAIL, exit 137 | 2708.69 s; 305G `.autopilot` workspace | `focused_resume/` |
| Persistent state sliced to LSU/ROB/completion/PRF fields | Stopped by watchdog | Exceeded 20GB before RTL generation | `focused_resume_sliced/` |
| Branch-free LSU/response product group using `BOOM_HLS_W4A_COMPLETION_DIAGNOSTIC` | Stopped by watchdog | Exceeded 20GB in `try_issue_load` RTL generation | `focused_resume_grouped/` |

Every failed workspace was placed on the separate `/home` filesystem, had no accepted RTL result, was checked for active descendants, and was deleted after durable logs were captured. No product source was changed by these attempts.

The evidence shows that Vitis HLS 2021.2 cannot practically generate this dynamic cross-call state command interface. Increasing timeout or disk allowance is not justified: the exact compact build consumed 305G and was OOM-killed, while two narrower designs repeated the same growth pattern.

## Completed Preservation

```text
PF3_FULL_CORE_RTL=PASS_12_OF_12
PF5_FULL_CORE_RTL=PASS_12_OF_12
PF5_MANDATORY_FAULTS=PASS_2_OF_2
PF6_INTEGRATED_RTL=PASS_12_OF_12
PF6_MANDATORY_FAULTS=PASS_2_OF_2
W4=PASS_13_OF_13
RVC_NATIVE=PASS_11_OF_11
RVC_CSIM=PASS_11_OF_11
RVC_FULL_CORE_RTL=PASS_11_OF_11
DURABLE_ACCEPTED_ARTIFACTS_VERIFIED=true
```

## Final Integrity

```text
L1R_CURRENT_SOURCE_HASH=b3cd4bdb13dc00e4b0293bc506268005bb8633b0e0036fc5e18f39314bbab618
SOURCE_MANIFEST_FILES_CHECKED=41
SOURCE_MANIFEST_MISMATCHES=0
MERGED_SOURCE_SHA256=76d78e9052344a0b1e06c0e723c473d37e0b2dd7679a2ab3b8fbbeed9f1dff9c
SRC_BOOM_ALL_SHA256=d6f885632ddd445729adda8148ea256e67683ccc8e7f2b10c9951e915d92c76c
CURRENT_FOCUSED_BUILD_INPUT_AGGREGATE=11d13b35fa8d0b609652ef044f4ebe9cfe47a67345a651f24852f64e4818ef2b
FOCUSED_NATIVE_DIAGNOSTIC=PASS_COMMANDS_8
FOCUSED_CATALOG=PASS_CASES_80_VARIANTS_30
INDEX_EMPTY=true
TRACKED_DELETIONS=0
ACTIVE_EDA_PROCESSES=0
PRODUCT_SOURCE_CHANGED_BY_RESUME=false
SRC_BOOM_ALL_UNCHANGED=true
```

## Required Redesign

Do not rerun the persistent command engine. A future attempt must use stateless, independently synthesized product-boundary scenarios with no dynamic writable cross-call ROB/LSU state. The RTL oracle must remain outside synthesized product logic, and each smaller image must be piloted under a strict workspace/RSS watchdog before expanding to all 80 cases.
