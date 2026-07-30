# Gate 4.0 W2 Regression Results

## Source Tests

| Suite | Outcome | Passed | Failed | Runs |
|---|---|---:|---:|---:|
| Directed | PASS | 25 | 0 | 1 |
| Gate 1 regression | PASS | 13 | 0 | 1 |
| IQ compaction | PASS | 10 | 0 | 1 |
| Branch snapshot | PASS | 30 | 0 | 1 |
| Branch randomized | PASS | 42 | 0 | 21 |
| Minimal LSU | PASS | 14 | 0 | 1 |
| Reset architecture | PASS | 14 | 0 | 1 |
| W1 lane interface | PASS | 1 | 0 | 1 |
| W2 dual-grant directed | PASS | 28 | 0 | 1 |

Recorded assertions: **177 passed, 0 failed**. The frozen W1 suites account for 149 assertions and the W2 directed suite adds 28.

The separate W2 random differential campaign passed 64 fixed seeds at 32 cycles per seed, or 2048 reference-model comparisons. It covered 63 dual-grant cycles, all four MEM/INT ready masks, 382 accepted grants, 424 retained grants, and zero dropped generated grants. This campaign reports one campaign-level PASS and is not counted as a 178th assertion.

## Trace And Architecture

- Frozen HLS C++ and Vitis csim traces: 10/10 byte-identical.
- Full-program architectural comparison: 10/10 PASS through retired store-to-`tohost`.
- Partial-order comparison: 8 legal reorder events and 0 real exposed violations; RAW/WAR/WAW timing remains insufficient-signal.

## Generated RTL

- Dedicated synthesized issue-selection cases: 5/5 PASS.
- Full conservative generated-core XSim matrix: 49/49 PASS.
- Canonical final RTL summaries are `issue_rtl/results.csv`, `rtl_run_status.csv`, and `rtl_test_matrix.csv`. Timestamped `*.backup.log` files preserve failed intermediate debug runs and are not final evidence.

Ordinary reset latency changed relative to Gate 3.9: first fetch is 1006 rather than 688 testbench cycles after release, and first commit is 1464 rather than 1105. Runtime reset behavior still passes, but W2 makes no strict cycle-equivalence claim.

## Qualification

The official Chipyard/FESVR/DRAMSim path remains unavailable. These results verify the supported HLS subset and generated RTL, not full BOOM equivalence.
