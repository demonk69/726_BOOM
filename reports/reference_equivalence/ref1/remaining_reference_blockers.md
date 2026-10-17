# REF1 Remaining Reference Blockers

`REF1_REFERENCE_MANIFEST_VALID=true`

`REF1_REFERENCE_SIMULATOR=FRESH_RUNS_EXISTING_MODEL_REBUILD_BLOCKED`

`REF1_HLS_RTL_BUILD=PASS_EXISTING_GENERATED_RTL_8_8_SOURCE_SET_MATCH`

`REF1_CONFIG=8_8`

`REF1_REFERENCE_PROGRAMS_RUN=5/5`

`REF1_HLS_PROGRAMS_RUN=5/5`

`REF1_MATCHED_PROGRAM_IMAGES=5/5`

`REF1_ARCH_DIFF_PASS_COUNT=5`

`REF1_ARCH_DIFF_FAIL_COUNT=0`

`REF1_ARCH_DIFF_NOT_RUN_COUNT=0`

`REF1_ORACLE_NEGATIVE_CONTROLS=PASS`

`REF1_TRANSACTION_BOUNDARY_DEFINED=true`

`REF1_DIFFERENTIAL_HARNESS_VALID=true`

`REF1_HIDDEN_SKIP_XFAIL_COUNT=0`

`REF1_REFERENCE_EQUIVALENCE_STATUS=PASS_SUPPORTED_ARCH_SUBSET_PREBUILT_REFERENCE_MODEL_PROVENANCE_LIMITED`

`REF1_REFERENCE_TOP_SHA256=6a3539a09e77f4c01a72269517f05cf8c7fdbd667e4c130116d741a5c2f3f3a9`

`REF1_REFERENCE_HARNESS_SHA256=7acac1f50864c551030cbc3aa9f0cbadb7833bdfaf8655ac912285e5834ba35d`

`REF1_REFERENCE_FIRRTL_SHA256=d55c6acb0faa2e77b737b56c7402d5181409f37d12a5d4c3e5ccce58799b038e`

`REF1_REFERENCE_CORE_CONFIG_SHA256=e6796cb43d2d9d71fcd2ec574c5d9adb16db898e62aa0a04a3653ea09d06bdea`

`REF1_HLS_CANONICAL_SOURCE_SHA256=589f5b5c90bbbbaadc4e2d30c14205c5dc8e5c4c6a904e27de94a4255cd9a5f7`

`REF1_HLS_RTL_TOP_SHA256=abd8c2a8cb27e37877214c2c6dd6c76efda42f4c5594470d0425ba4cd2ed99fa`

`REF1_HLS_RTL_AGGREGATE_SHA256=e7104954480471e3132a7842be56303d75e020a1b497a1cf3609f6a230a6a69b`

`REF1_FINAL_REFERENCE_BOUNDARY=ChipTop/DigitalTop`

`REF1_FIRST_DIFFERENTIAL_TARGET=BoomCore/BoomTile_SUPPORTED_ARCHITECTURAL_SUBSET`

`REF1_TESTHARNESS_SCOPE=SIMULATION_ONLY`

`REF1_PRODUCT_SOURCE_MODIFIED=false`

`PPA_GATE_STATUS=UNRESOLVED`

## Remaining blockers

- Reference runner relink/rebuild is blocked by the missing compatible `verilated_heavy.h`; fresh executions used the existing generated model/executable.
- Reference trace instrumentation lacks general AXI/TileLink request/response events and full final-memory dump; protocol comparison is deferred beyond the observed tohost architectural effect.
- HLS testbench is an IMEM/DMEM stream memory adapter, not a ChipTop/TileLink implementation. TileLink/cache/MMU/L2 equivalence remains `IN_SCOPE_DEFERRED`.
- Per-feature architectural support is limited to the tested integer/control-flow subset. Five program PASS does not imply whole BOOM equivalence.
- REF1 derived the HLS-only testbench by removing five PPA6 data sentinels; all DMEM starts at zero like the reference sparse memory. The derivation is recorded in `ref1_hls_tb_manifest.json`.
