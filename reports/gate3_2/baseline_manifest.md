# Gate 3.2 Baseline Manifest

Date: 2026-07-25

Frozen Git commit: `6645b3df8fdf2828713513ad22cf4dcceb0a89f0`

Baseline artifacts captured before Gate 3.2 edits:

- Source hashes: `reports/gate3_2/source_hashes_before.txt`
- Directed test log: `reports/gate3_2/logs/directed_before.log`
- Gate 1 regression log: `reports/gate3_2/logs/gate1_before.log`
- Minimal LSU regression log: `reports/gate3_2/logs/lsu_before.log`
- Full-program diff log: `reports/gate3_2/logs/full_program_diff_before.log`
- Previous post-LSU csynth timeout log: `reports/gate3_2/logs/hls_csynth_before.log`
- HLS C++ complete trace baseline: `reports/gate3_2/baseline_traces/hls_cpp/`
- HLS csim complete trace baseline: `reports/gate3_2/baseline_traces/hls_csim/`

Baseline status:

| Check | Result |
|---|---:|
| Directed tests | 25/25 PASS |
| Gate 1 regressions | 13/13 PASS |
| Minimal LSU tests | 14/14 PASS |
| BOOM vs HLS full-program architectural diff | 10/10 PASS |
| Post-LSU Vitis HLS csynth | TIMEOUT, no `csynth.rpt` |

Gate 3.2 edits must preserve the architectural trace contents unless a difference is explicitly recorded and justified.
