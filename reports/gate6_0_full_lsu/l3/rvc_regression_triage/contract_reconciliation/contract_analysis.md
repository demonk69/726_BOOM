# Gate6.0 L3R1-S3 RVC Contract Reconciliation

Date: 2026-10-08

Branch: `handoff/gate6-l3r1-20261008`

Base before S3 patch: `af404fa` (`Gate 6.0 L3R1: close historical RVC provenance investigation`)

Product source modified: no

## Executive Result

`L3R1_S3_RVC_CONTRACT_RECONCILED=true`

`RVC_PRODUCT_REPAIR_REQUIRED=false`

`RVC_TEST_CONTRACT_REPAIR_REQUIRED=true`

The current frontend behavior is consistent with the post-Gate5.3 packetized frontend contract: a matched IMEM response may build a two-lane fetch packet, enqueue it through the fetch buffer, and issue a bounded lookahead request in the same native architectural call. The historical Gate5.2 scalar RVC test contract assumed same-call publication and no request outside a one-word constructed image. That assumption is no longer valid for current product code.

S3 therefore repairs the RVC test and audit contracts only. No `src/` product file was changed.

## Cycle-Level Finding

Representative `C/C` stream at PC `0x20000`:

| Implementation | First divergent cycle | Evidence | Interpretation |
| --- | ---: | --- | --- |
| historical `db9d3b2` | cycle 1 | response for `0x20000` publishes packet PC `0x20000`; no request is emitted | scalar frontend drains the lower compressed parcel immediately |
| current `af404fa` product | cycle 1 | response for `0x20000` builds a pending two-lane packet and emits lookahead request `0x20004`; no packet is externally visible yet | packetized frontend/fetch-buffer path can prefetch before publication |

The current packet for PC `0x20000` appears at cycle 3 and PC `0x20002` at cycle 4. This is an allowed latency/visibility difference under the current fetch-buffer contract, not evidence of a wrong decompressed instruction.

Condensed trace evidence is archived in `cycle_trace.csv`.

## Reconciled Contract

The focused RVC tests now check observable behavior rather than obsolete exact internal timing:

| Area | Reconciled rule |
| --- | --- |
| bounded lookahead | Tests provide bounded padding for current frontend lookahead and fail if requests escape the bounded image. |
| publication timing | Tests wait a bounded number of native calls for the target PC instead of requiring same-call publication. |
| stream order | Published uops must match the expected architectural PC order, RVC attribution, original parcel, decompressed instruction, and exception fields. |
| backpressure | Held packets must remain stable while decode/rename is not ready. |
| stale responses | Random testing injects stale ID, stale epoch, stale address, and post-reset stale responses and validates that accepted publications follow the actual matched request stream. |
| throughput | Upper compressed parcels must not wait for a same-word response; a future lookahead response may overlap the publication call. |

## Protected/Decode-Gap Reconciliation

The initial 1253 focused failures were not one class:

| Class | S3 resolution |
| --- | --- |
| `C.SRLI` RV64 shamt[5] | Current decompressor/decode path produces final instruction `0x02045413` and Decode uopc `UOPC_SRLI`; the focused test now validates that final contract. |
| `C.JALR` | Current decompressor/decode path produces final instruction `0x000080e7` and Decode uopc `UOPC_JALR`; the focused test now validates compressed attribution and link semantics through Decode. |
| bounded lookahead/prefetch | Tests now use bounded images and wait for observable target packets. |

## Validation Summary

Clean validation worktree: `/home/lab_726/opencode_tmp/g6_l3r1_s3_validate`

Historical control worktree: `/home/lab_726/opencode_tmp/g6_l3r1_s2_rvc_db9d`

Results:

| Validation | Result |
| --- | --- |
| current RVC focused native | PASS, `RVC_FETCH_METRICS assertions=5839 cases=414 packets=104 requests=1035 protected=288 failures=0` |
| current RVC random native | PASS, `RVC_FETCH_RANDOM_256X2048_PASS`, errors=0 |
| current RVC execute-gap native | PASS, `RVC_JALR_LINK compressed=PASS base=PASS` |
| current RVC throughput audit | PASS, `PASS: Gate 5.2 R2 native throughput audit` |
| historical RVC baseline control | PASS at `db9d3b2`, failures=0 |
| L3 native five-config | PASS, `G6_L3_NATIVE_ALL=PASS` |

Full command/status details are in `validation_matrix.csv`.

## Scope Guard

Changed implementation scope:

| Path | Reason |
| --- | --- |
| `tb/differential/rvc_fetch_tests.cpp` | focused RVC contract repair for bounded lookahead and final Decode semantics |
| `tb/differential/rvc_fetch_random_tests.cpp` | random oracle repair from obsolete exact-state scalar model to observable request/response/publication contract |
| `tb/differential/gate5_2_r2_native_throughput_audit.cpp` | throughput check repair for overlapped future lookahead responses |
| `scripts/gate5_2/run_r2_native_tests.sh` | current execute-gap target link repair: add `src/predictor.cpp`, required by current `BoomCoreState` |

No product source file under `src/` was edited for S3.
