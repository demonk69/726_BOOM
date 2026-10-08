# First Divergence

## Current vs Clean Baseline

`RVC_FIRST_DIVERGENT_CYCLE=NOT_APPLICABLE_BASELINE_FAILS_IDENTICALLY`

`RVC_FIRST_DIVERGENT_SIGNAL=NONE_CURRENT_AND_CLEAN_BASELINE_STDOUT_STDERR_IDENTICAL`

`BASELINE_VALUE=RVC_FETCH_METRICS assertions=4389 cases=414 packets=104 requests=430 protected=288 failures=1253`

`CURRENT_VALUE=RVC_FETCH_METRICS assertions=4389 cases=414 packets=104 requests=430 protected=288 failures=1253`

The current run and clean archive run have identical stdout and stderr hashes. There is no current-source divergence to trace.

## First Failing Assertion In Both Runs

| Field | Value |
| --- | --- |
| Test | `C/C` |
| Assertion | `FAIL[9]` |
| Existing cycle log | not available |
| Expected | request address is present in constructed image map |
| Actual | request at `0x20004`, outside the constructed image |
| Message | `C/C: request outside constructed image at 0x20004` |

The test constructs two compressed parcels at base `0x20000`, which creates one 32-bit memory word at `0x20000`. The first failing check observes a later IMEM request at `0x20004`, outside that one-word constructed image.

## Provenance Divergence

The meaningful divergence is not current source vs clean source. It is checked-in release evidence vs rerun release source:

| Source | Value |
| --- | --- |
| Release checked-in artifact | `assertions=4111 requests=451 failures=0` |
| Recompiled release source now | `assertions=4389 requests=430 failures=1253` |

This points to test-provenance drift or an unreproduced historical build context, not an L3-owned product change. The release PASS artifact's source hash manifest records `tb/differential/rvc_fetch_tests.cpp` hash `5d134...` and `scripts/gate5_2/run_r2_native_tests.sh` hash `2bcb...`, while the clean release archive contains `f5f...` and `ac3...` respectively.
