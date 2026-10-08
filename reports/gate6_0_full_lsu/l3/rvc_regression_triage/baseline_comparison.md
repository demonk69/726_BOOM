# Baseline Comparison

## Baseline Source

| Field | Value |
| --- | --- |
| Commit | `2c6c5067274d9c310c637d20752bc271590bcded` |
| Extraction | `git archive` into `/home/lab_726/opencode_tmp/g6_l3_rvc_baseline/src` |
| Main worktree modified | no |
| Runner | `scripts/gate5_2/run_r2_native_tests.sh` |
| Compiler | `g++ (Ubuntu 11.4.0-1ubuntu1~22.04.3) 11.4.0` |
| Flags | `-std=c++11 -O2 -Wall -Wextra -Werror -Wno-error=misleading-indentation -Wno-error=unused-label -Wno-unknown-pragmas -I<root>/include` |

## Source Equivalence

| Item | Current SHA256 | Baseline SHA256 | Changed |
| --- | --- | --- | --- |
| `tb/differential/rvc_fetch_tests.cpp` | `f5f0238106be13b14c35a2077a69a6c8de5f6eba65eb5be980fe467f148d0b6b` | `f5f0238106be13b14c35a2077a69a6c8de5f6eba65eb5be980fe467f148d0b6b` | false |
| `scripts/gate5_2/run_r2_native_tests.sh` | `ac3c72c50ee26537aa26ce8bc2327d9cdc152a6b7c07afb802f328e20a108c53` | `ac3c72c50ee26537aa26ce8bc2327d9cdc152a6b7c07afb802f328e20a108c53` | false |

`RVC_TEST_SOURCE_CHANGED=false`

`RVC_RUNNER_CHANGED=false`

`RVC_BUILD_FLAGS_CHANGED=false`

No tracked current-vs-baseline diff exists in the RVC/front-end source set used by `rvc_fetch_tests`: `frontend.cpp`, `fetch_packet.cpp`, `fetch_buffer.cpp`, `predecode.cpp`, `predictor.cpp`, `rvc.cpp`, `decode.cpp`, or related frontend headers.

## Recompiled Control Result

| Run | stdout SHA256 | stderr SHA256 | Status |
| --- | --- | --- | --- |
| Current focused run | `b7cb21e31c54f9f7cacae344d0d2dcd6ab0f87ded3b93813dce31d31753cdf2b` | `9833e480cb60ca32ea9ca0588994e1c18d3246fc10f16e6e6fe0e031d4c55337` | FAIL |
| Clean archive focused run | `b7cb21e31c54f9f7cacae344d0d2dcd6ab0f87ded3b93813dce31d31753cdf2b` | `9833e480cb60ca32ea9ca0588994e1c18d3246fc10f16e6e6fe0e031d4c55337` | FAIL |
| Clean archive runner run | `b7cb21e31c54f9f7cacae344d0d2dcd6ab0f87ded3b93813dce31d31753cdf2b` | detailed stderr in `/home/lab_726/opencode_tmp/g6_l3_rvc_baseline/runner.stderr.log` | FAIL |

`stdout_cmp_status=0`

`stderr_cmp_status=0`

Current and clean baseline reproduce identical runtime behavior for `rvc_fetch_tests` under the current environment.

## Release Artifact Provenance Conflict

The archived release commit includes a checked-in PASS artifact:

`/home/lab_726/opencode_tmp/g6_l3_rvc_baseline/src/reports/gate5_2_rvc/r2/logs/rvc_fetch_tests.log`

That artifact reports:

`RVC_FETCH_METRICS assertions=4111 cases=414 packets=104 requests=451 protected=288 failures=0`

`RVC_FETCH_CANONICAL_CPP11_UNIQUE_PASS_726`

Recompiling and rerunning the same archived source now reports:

`RVC_FETCH_METRICS assertions=4389 cases=414 packets=104 requests=430 protected=288 failures=1253`

Therefore `RVC_BASELINE_CONTROL_STATUS=FAIL`. The L3R1 repair phase is stopped before product changes.

## Provenance Root Detail

The release PASS report is internally inconsistent with the release commit source tree. Its own source hash manifest records different hashes for the RVC test and runner than the files present in the clean archive:

| File | Clean archive/current SHA256 | Release PASS `source_hashes_after.txt` SHA256 |
| --- | --- | --- |
| `tb/differential/rvc_fetch_tests.cpp` | `f5f0238106be13b14c35a2077a69a6c8de5f6eba65eb5be980fe467f148d0b6b` | `5d134a4c430f6fd049954a5179e156df4744551d637cacfdcd37a5178b52ff2c` |
| `scripts/gate5_2/run_r2_native_tests.sh` | `ac3c72c50ee26537aa26ce8bc2327d9cdc152a6b7c07afb802f328e20a108c53` | `2bcb5194927677e8e0dda21cc61c413656de472678f7a023bcba4f125c67e65c` |

This indicates the checked-in PASS artifact was generated from a different test/runner provenance than the source files now present at `2c6c506...`. The current 1253-failure rerun is therefore a baseline provenance mismatch, not an L3 product regression.
