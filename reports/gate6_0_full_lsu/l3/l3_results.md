# Gate 6.0 L3 Results

`G6_0_L3_MULTI_OUTSTANDING_LOADS_VERIFIED=false`

Reason: L3 functional native/CSim and L2 preservation passed, but full acceptance is blocked by uncompleted RTL/synthesis/PPA gates and RVC native preservation triage. The RVC failure is not closed because the clean release archive now fails identically to current source under the current environment.

## Architecture

| Field | Value |
| --- | --- |
| Baseline commit | `2c6c5067274d9c310c637d20752bc271590bcded` |
| Branch | `gate3.8-rtl-verification` |
| Architecture | `PER_LQ_ENTRY_OUTSTANDING_OWNERSHIP` |
| Per-LQ feasible | `true` |
| 4/4 max outstanding | 4 |
| 8/8 max outstanding | 8 |
| 16/16 max outstanding | 16 |
| Active txid uniqueness | `true` |
| Out-of-order response support | `true` |
| Duplicate response rejected | `true` |
| Forwarded load consumes external slot | `false` |

## Functional Evidence

| Matrix | Status | Evidence |
| --- | --- | --- |
| L3 native 4/4, 8/8, 16/16, 4/16, 16/4 | PASS | `/home/lab_726/opencode_tmp/g6_l3/native/*.log` |
| L3 CSim 4/4, 8/8, 16/16, 4/16, 16/4 | PASS | `/home/lab_726/opencode_tmp/g6_l3/csim/*.log` |
| L3 bounded exhaustive | PASS | 64 states, 0 errors, 4/4 |
| L3 random default 8/8 | PASS | 256 seeds x 8192 cycles, 0 errors |
| L2 native preservation | PASS | `/home/lab_726/opencode_tmp/g6_l3/l2_native/*.log` |
| L2 CSim preservation | PASS | `/home/lab_726/opencode_tmp/g6_l3/l2_csim/*.log` |

## Blockers

| Gate | Status | Detail |
| --- | --- | --- |
| Focused RTL PATH_A/PATH_B | NOT_RUN | Catalog/report created, RTL not generated or simulated |
| Full-core synthesis 4/4, 8/8, 16/16 | NOT_RUN | PPA/timing unavailable |
| Critical path | NOT_RUN | Requires synthesis |
| PF6 integrated and faults | NOT_RUN | Not completed |
| W3/W4/M3 preservation | NOT_RUN | Not completed |
| RVC native preservation | FAIL_BASELINE_CONTROL_FAIL | `rvc_fetch_tests` reported 1253 frontend/RVC failures; clean `2c6c506...` archive rerun failed identically |

## L3R1 RVC Triage

| Field | Value |
| --- | --- |
| `L3R1_RVC_REGRESSION_CLOSED` | `false` |
| `RVC_FAILURE_TOTAL_BEFORE` | `1253` |
| `RVC_FAILURE_DISTINCT_TESTS` | `20` |
| `RVC_FAILURE_DISTINCT_SIGNATURES` | `71` |
| `RVC_BASELINE_CONTROL_STATUS` | `FAIL` |
| `RVC_CURRENT_REPRODUCED` | `true` |
| `RVC_CURRENT_ROOT_CAUSE_IDENTIFIED` | `false` |
| Triage directory | `reports/gate6_0_full_lsu/l3/rvc_regression_triage/` |

## PPA

| Threshold | Value | Status |
| --- | ---: | --- |
| LUT review | 234780 | NOT_EVALUATED |
| LUT blocker | 266795 | NOT_EVALUATED |
| BRAM max | 16 | NOT_EVALUATED |
| DSP max | 3 | NOT_EVALUATED |
| Period hard gate | 6.450 ns | NOT_EVALUATED |

## Final Status

`L3_FUNCTIONAL_STATUS=PASS`

`L3_PPA_STATUS=NOT_RUN`

`L3_TIMING_STATUS=NOT_RUN`

`RVC_PRESERVATION_STATUS=FAIL`

`NEXT_REQUIRED_ACTION=INVESTIGATE_RVC_BASELINE_PROVENANCE_OR_TOOLCHAIN_DRIFT`

No stage, commit, or push was performed.
