# L1R2B PATH_B Resolution

## Outcome

- PATH_A canonical acceptance: `70/70` assigned cases PASS.
- PATH_B canonical acceptance: `10/10`; IDs `13,28,29,36,38,39,40,47,48,49` PASS.
- Overall exact canonical acceptance: `80/80`.

## Current-Source Full-Core Attempt

- Top: `boom_core_top`.
- Product source aggregate: `b3cd4bdb13dc00e4b0293bc506268005bb8633b0e0036fc5e18f39314bbab618`.
- Merged source: `76d78e9052344a0b1e06c0e723c473d37e0b2dd7679a2ab3b8fbbeed9f1dff9c`.
- Parameters: default `LQ_DEPTH=8`, `SQ_DEPTH=8`, LUTRAM FTQ/predictor, `xczu7ev-ffvc1156-2-e`, 10 ns.
- Watchdog: 4 GiB soft limit, 8 GiB hard limit, 900 s timeout.
- Result: hard stop after approximately 468 s with exit code 143.
- Peak/final workspace: `8,955,852 KiB`, above the 8 GiB hard limit.
- Last HLS phase: `try_issue_load` scheduling; no generated `boom_core_top.v` exists.
- HLS log: `reports/gate6_0_full_lsu/l1/focused_rtl_redesign/logs/full_core_csynth.log`.

## Bounded Retry

- Canonical-flow audit before fix: `false`; the failed invocation omitted
  `config_compile -pipeline_loops 0`.
- Canonical-flow audit after infrastructure fix: `true`.
- Authorization: one bounded retry, now consumed successfully.
- Limits: 12 GiB soft workspace, 16 GiB hard workspace, 1800 s timeout.
- Result: PASS after 540 s; peak workspace `648,953,856` bytes; peak RSS
  `7,136,813,056` bytes; minimum system MemAvailable `53,816,786,944` bytes.
- Generated RTL hash: `c0165ccea43ccc41d1d2cc5514d8af17764b700b84a84af751c4b331be55fcbe`.
- HLS testbench sources included: `false`.

## Semantic Closure

The ten cases ran in independent XSim processes with an oracle outside the DUT.
The resulting exact evidence is in `path_b_case_evidence.csv`; combined ordered
acceptance is in `canonical_focused_rtl_acceptance.csv`. HLS completion alone
was not counted as case acceptance.

```text
PATH_B_FULL_CORE_RETRY_EXHAUSTED=false
PATH_B_RTL_STATUS=PASS_10_OF_10
FOCUSED_RTL_STATUS=PASS_80_OF_80
NEXT_REQUIRED_ACTION=COMMIT_G6_0_L1
```
