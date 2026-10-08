# S3 Test Patch Review

## Decision

`S3_PATCH_ACCEPTED_FOR_VALIDATION=true`

`PRODUCT_CODE_CHANGED=false`

The S3 patch is limited to test harnesses and the RVC native runner. It does not mask failures by reducing pass/fail checks to unconditional success; it replaces stale exact-timing assumptions with bounded observable contracts and increases focused assertions from the historical 4111/current-failing 4111 shape to 5839 assertions.

## Changed Files

| File | Review |
| --- | --- |
| `tb/differential/rvc_fetch_tests.cpp` | Adds bounded lookahead images and bounded target-PC waits. Keeps instruction, PC, RVC attribution, exception, and Decode semantic checks. Adds explicit `C.SRLI` and `C.JALR` final-contract checks. |
| `tb/differential/rvc_fetch_random_tests.cpp` | Replaces historical scalar internal-state comparison with a random observable oracle over actual requests, accepted responses, stale-response injection, held packet stability, bounded publication order, and instruction semantics. The random data stream avoids CFI encodings so the oracle stays fetch-contract scoped; focused tests still cover `C.JALR`, and adversarial redirects still cover redirect handling. |
| `tb/differential/gate5_2_r2_native_throughput_audit.cpp` | Keeps all throughput scenarios and cadence checks. Updates all-C local-reuse check to reject same-word response dependence while allowing overlapped future lookahead responses. |
| `scripts/gate5_2/run_r2_native_tests.sh` | Adds `src/predictor.cpp` to the execute-gap link line because current `BoomCoreState` constructs predictor state. |

## Anti-Fake-PASS Checks

| Check | Evidence |
| --- | --- |
| focused checks still active | `RVC_FETCH_METRICS assertions=5839 cases=414 packets=104 requests=1035 protected=288 failures=0` |
| random coverage still broad | `seeds=256 cycles_per_seed=2048 total_cycles=524288`; stale ID, stale epoch, stale address, post-reset stale, upper redirects, odd redirects, access faults, and cross-upper faults all nonzero |
| throughput checks still active | all-C, all-32, alternating-C-32, and cross-boundary-heavy scenarios run and report cadence metrics |
| historical control still reproducible | `db9d3b2` runner/source still passes with `RVC_FETCH_CANONICAL_CPP11_UNIQUE_PASS_726` |
| product code untouched | no S3 edits under `src/` |

## Residual Limits

This is native C++ validation. It does not claim HLS `ap_ctrl_hs` physical-clock II or RTL waveform equivalence. L3 native five-config preservation was rerun and passed; RVC CSim/RTL preservation remains outside this S3 native contract reconciliation.
