# RVC Failure Inventory

`RVC_FAILURE_TOTAL=1253`

`RVC_FAILURE_DISTINCT_TESTS=20`

`RVC_FAILURE_DISTINCT_SIGNATURES=71`

## Evidence

| Artifact | Path | Status |
| --- | --- | --- |
| Original preservation stdout | `/home/lab_726/opencode_tmp/g6_l3/preservation/rvc_report/logs/rvc_fetch_tests.log` | FAIL, summary only |
| Current focused stdout | `/home/lab_726/opencode_tmp/g6_l3_rvc_current/report/logs/rvc_fetch_tests.stdout.log` | FAIL, `failures=1253` |
| Current focused stderr | `/home/lab_726/opencode_tmp/g6_l3_rvc_current/report/logs/rvc_fetch_tests.stderr.log` | 1253 detailed FAIL lines |
| Clean archive stdout | `/home/lab_726/opencode_tmp/g6_l3_rvc_baseline/report/logs/rvc_fetch_tests.stdout.log` | FAIL, `failures=1253` |
| Clean archive stderr | `/home/lab_726/opencode_tmp/g6_l3_rvc_baseline/report/logs/rvc_fetch_tests.stderr.log` | 1253 detailed FAIL lines |
| Archived runner stdout | `/home/lab_726/opencode_tmp/g6_l3_rvc_baseline/runner_report/logs/rvc_fetch_tests.log` | FAIL, `failures=1253` |
| Release checked-in reference | `/home/lab_726/opencode_tmp/g6_l3_rvc_baseline/src/reports/gate5_2_rvc/r2/logs/rvc_fetch_tests.log` | PASS, `failures=0` |

## First Failure

| Field | Value |
| --- | --- |
| `RVC_FIRST_FAILING_TEST` | `C/C` |
| `RVC_FIRST_FAILING_ASSERTION` | `FAIL[9]` |
| `RVC_FIRST_FAILING_CYCLE` | not logged by existing harness |
| `RVC_FIRST_FAILURE_EXPECTED` | no IMEM request outside constructed image; valid word address for `C/C` image is `0x20000` |
| `RVC_FIRST_FAILURE_ACTUAL` | IMEM request at `0x20004` |
| Message | `C/C: request outside constructed image at 0x20004` |

## Dominant Groups

The failures are not 1253 independent bugs. They collapse to 20 grouped test areas and 71 full message signatures.

| Group | Failures | Notes |
| --- | ---: | --- |
| protected `C.SRLI shamt[5]` matrix | 1024 | 256 encodings x 4 repeated signatures |
| protected `C.JALR` matrix | 124 | 31 encodings x 4 repeated signatures |
| reserved and unsupported RVC | 39 | illegal compressed parcel handling |
| mixed/long stream prefetch/final PC | 22 | requests beyond constructed image plus final PC checks |
| fault position checks | 16 | compressed, aligned 32-bit, and cross-upper fault metadata |
| redirect/cross-word/stall/branch recovery | 28 | one-shot control-flow and carry/stall checks |

## Stop Condition

`RVC_BASELINE_CONTROL_STATUS=FAIL`

The clean archive at `2c6c5067274d9c310c637d20752bc271590bcded` reproduces the same stdout and stderr as current source. Per L3R1 instructions, this blocks product repair and requires environment/test provenance investigation first.
