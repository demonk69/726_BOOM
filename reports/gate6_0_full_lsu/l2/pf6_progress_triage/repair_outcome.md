# PF6 Progress Repair Outcome

`PF6_PROGRESS_TRIAGE=PASS`

`PF6_PROGRESS_FAILURE_CLASS=HARNESS_WAIT_CONDITION_BUG`

`PF6_HARNESS_REPAIR_REQUIRED=true`

`L2R_PRODUCT_BUG_FOUND=false`

Repair scope:

- Changed the PF6 testbench observation state from stale generated state 34 to
  current equivalent generated state 35 and synchronized the harness's
  generated hierarchy references to current instances `fu_7823` and `fu_3682`.
- Added an explicit runner option to reuse already hash-verified RTL without
  allowing source mtime to trigger HLS. The hardened reuse path requires a
  caller-supplied expected source hash and an exact per-file `.v`/`.dat`
  SHA-256 manifest; this check is separate from the historical 91-Verilog
  provenance aggregate and rejects missing, extra, or mismatched files.
- Did not modify product source, product RTL, stimulus, program set, fault set,
  termination semantics, signatures, or oracle checks.

Acceptance:

- `PF6_CURRENT_SOURCE_FULL_CORE_RTL=PASS_12_OF_12`
- `PF6_MANDATORY_FAULTS=PASS_2_OF_2`
- Source aggregate remained
  `7fa60c73d7c2a584ecb859e8d4f6ec5270ad5d8d6da9751030af6becdc68d808`.
- Coverage values again match the previously accepted observer behavior,
  including two predicted-taken observations in both mandatory cases.

`G6_0_L2_STORE_TO_LOAD_FORWARDING_VERIFIED=true`

`NEXT_REQUIRED_ACTION=COMMIT_G6_0_L2`
