# Minimal LSU Regression Results

Date: 2026-07-24

Scope: Gate 3.1C minimal integer LSU/store-to-`tohost` path.

| Suite | Result |
|---|---:|
| Directed tests | 25/25 PASS |
| Gate 1 regressions | 13/13 PASS |
| Minimal LSU tests | 14/14 PASS |
| HLS prefix trace harness, native C++ | 5/5 PASS |
| HLS complete trace harness through `tohost`, native C++ | 5/5 PASS |
| Vitis HLS csim complete trace harness through `tohost` | 5/5 PASS |
| BOOM vs HLS C++/csim full-program architectural diff | 10/10 PASS |
| Vitis HLS baseline csynth | TIMEOUT after 30 minutes |

Evidence:

- `tb/differential/lsu_minimal_tests.cpp`
- `reports/equivalence/provisional_gate3_1/full_program_architectural_diff.csv`
- `reports/equivalence/provisional_gate3_1/full_program_architectural_diff.md`
- `reports/equivalence/provisional_gate3_1/hls_csynth.log`
- `reference/hls_traces/*_hls_cpp_full.jsonl`
- `reference/hls_traces/*_hls_csim_full.jsonl`

Limitation: this is not a full BOOM LSU/cache/MMU implementation. The implemented path is a conservative minimal integer LSU sufficient for the directed load/store subset and the committed store-to-`tohost` termination used by these loaded-program traces.

Synthesis note: baseline Vitis HLS csynth did not fail source analysis or synthesizability checks before timeout. Both 15-minute and 30-minute attempts reached the HLS transformation/array-partitioning stage and were killed by the tool timeout before a `csynth.rpt` was produced.
