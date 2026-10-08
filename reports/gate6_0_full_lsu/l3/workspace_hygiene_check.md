# Workspace Hygiene Check

Baseline checks:

| Check | Result |
| --- | --- |
| `pwd` | `/home/lab_726/boom/hls_boom` |
| repo root | `/home/lab_726/boom/hls_boom` |
| branch | `gate3.8-rtl-verification` |
| HEAD | `2c6c5067274d9c310c637d20752bc271590bcded` |
| ahead/behind vs origin | `0/0` |
| index | empty before edits; no staging performed |
| `src/boom_all.cpp` SHA256 | `d6f885632ddd445729adda8148ea256e67683ccc8e7f2b10c9951e915d92c76c` |

Repository constraints honored:

| Constraint | Status |
| --- | --- |
| No stage | PASS |
| No commit | PASS |
| No push | PASS |
| No clean/reset/stash | PASS |
| Pre-existing dirty files left untouched | PASS_BEST_EFFORT |
| `src/boom_all.cpp` unchanged | PASS |
| L3R1 product source repair | NOT_PERFORMED_BASELINE_CONTROL_FAIL |
| RVC triage reports added | PASS |

Note: the workspace had extensive pre-existing tracked and untracked dirty artifacts before L3 work began.

L3R1 stop condition:

| Check | Result |
| --- | --- |
| Current `rvc_fetch_tests` rerun | FAIL, 1253 failures |
| Clean archive `rvc_fetch_tests` rerun | FAIL, 1253 failures |
| Current vs clean rerun stdout/stderr | identical |
| Product repair performed | no |
| Active EDA processes | 0 |
