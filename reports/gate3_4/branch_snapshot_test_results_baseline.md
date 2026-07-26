# Gate 3.3 Branch Snapshot Test Results

Date: 2026-07-26

## Result

| Suite | Result | Log |
|---|---:|---|
| Directed branch snapshot tests | 30/30 PASS | `reports/gate3_3/logs/branch_snapshot_tests.log` |
| Random branch snapshot tests | 2/2 PASS | `reports/gate3_3/logs/branch_snapshot_random_tests.log` |

Random seed: `0x3a33b007`.

## Directed Coverage

The directed suite covers branch tag allocation/release/full/wrap, nested branch snapshots, map restore, free-list rollback, busy recovery, ROB/IQ/LDQ/STQ younger-state kill, wrong-path store/writeback/commit suppression, stale IMEM response rejection, simultaneous mispredict interactions, ROB wrap, IQ full recovery, and deterministic nested-branch stress.

## Status

The tests validate the HLS branch recovery behavior for the supported single-lane integer/minimal-LSU subset. They do not prove strict cycle equivalence to BOOM because the official BOOM simulator and event traces remain unavailable.
