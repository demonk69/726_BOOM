# Gate6.0 L3R1-S2 RVC Historical PASS Provenance Reconstruction

Date: 2026-10-08

Primary branch: `handoff/gate6-l3r1-20261008`

Pushed branch head before S2 investigation: `b6617d29ba2ec2ce6fa15b77a0eefecad5b6fa7a`

Product code modified in primary workspace: no

## Executive Result

The historical RVC PASS source was recovered exactly. The PASS was produced by commit `db9d3b22fb3672751d9e012cf3130296cbf19f3a` (`Gate 5.2 R2: verify mixed-width RVC fetch`), not by the later Gate6 release baseline commit `2c6c5067274d9c310c637d20752bc271590bcded`.

The historical PASS is reproducible today with the current host compiler:

```text
RVC_FETCH_METRICS assertions=4111 cases=414 packets=104 requests=451 protected=288 failures=0
RVC_FETCH_CANONICAL_CPP11_UNIQUE_PASS_726
```

This means the prior blocker was not an unrecoverable historical report inconsistency. It was a baseline provenance mismatch: the checked historical PASS report belongs to the Gate5.2 R2 source/runner at `db9d3b2`, while the clean release baseline used during L3R1 was `2c6c506`, whose RVC source/test/runner set had already changed.

## Historical Source Tracking

Target historical hashes from `reports/gate5_2_rvc/r2/source_hashes_after.txt`:

| File | Target SHA256 | Match |
| --- | --- | --- |
| `tb/differential/rvc_fetch_tests.cpp` | `5d134a4c430f6fd049954a5179e156df4744551d637cacfdcd37a5178b52ff2c` | found |
| `scripts/gate5_2/run_r2_native_tests.sh` | `2bcb5194927677e8e0dda21cc61c413656de472678f7a023bcba4f125c67e65c` | found |

Exact blob matches:

| SHA256 | Git blob | Path |
| --- | --- | --- |
| `5d134a4c430f6fd049954a5179e156df4744551d637cacfdcd37a5178b52ff2c` | `c6a177b12596551255391a9cae1a7524ce06ec81` | `tb/differential/rvc_fetch_tests.cpp` |
| `2bcb5194927677e8e0dda21cc61c413656de472678f7a023bcba4f125c67e65c` | `4739613e7bad4affa1725d2e07efefeaa7d1489f` | `scripts/gate5_2/run_r2_native_tests.sh` |

Only one reachable commit contains both exact file hashes:

| Commit | Date | Subject |
| --- | --- | --- |
| `db9d3b22fb3672751d9e012cf3130296cbf19f3a` | 2026-08-12T08:27:08+08:00 | `Gate 5.2 R2: verify mixed-width RVC fetch` |

Refs containing `db9d3b2`:

```text
gate3.8-rtl-verification
handoff/gate6-l3r1-20261008
origin/gate3.8-rtl-verification
origin/handoff/gate6-l3r1-20261008
```

Evidence files:

| Artifact | Purpose |
| --- | --- |
| `git_path_object_hash_search.txt` | Exact SHA256-to-Git-blob matches. |
| `git_commit_hash_matrix.csv` | Commit-level match matrix; only `db9d3b2` contains both exact hashes. |
| `git_refs_inventory.txt` | Local refs searched after remote fetch. |
| `git_reflog_inventory.txt` | Reflog inventory captured for audit. |
| `local_file_hash_search.csv` | Local worktree/temp file hash checks. |

## Reconstructed Run Matrix

Full matrix: `run_matrix.csv`

| Case | Product | Test | Runner/source list | Result |
| --- | --- | --- | --- | --- |
| `DB9D_HISTORICAL_PASS` | `db9d3b2` | historical `5d134...` | historical `2bcb...` | PASS, failures=0 |
| `RELEASE_2C6_CURRENT_BASELINE` | `2c6c506` | current `f5f023...` | current `ac3c...` | FAIL, failures=1253 |
| `RELEASE_2C6_HIST_TEST_HIST_RUNNER` | `2c6c506` | historical `5d134...` | historical `2bcb...` | link FAIL; runner omits newer frontend dependencies |
| `RELEASE_2C6_HIST_TEST_CURRENT_SOURCE_LIST` | `2c6c506` | historical `5d134...` | current source list | FAIL, failures=1477 |
| `DB9D_PRODUCT_CURRENT_TEST_OLD_SOURCE_LIST` | `db9d3b2` | current `f5f023...` | historical source list | FAIL, failures=864 |
| `HEAD_B6617_CURRENT_RVC` | `b6617d2` | current `f5f023...` | current `ac3c...` | FAIL, failures=1253 |

Compiler used for all S2 native runs:

```text
g++ (Ubuntu 11.4.0-1ubuntu1~22.04.3) 11.4.0
```

Product source bundle hashes are deterministic SHA256s of the per-file source-hash manifests:

| Commit | Product source bundle SHA256 | Manifest |
| --- | --- | --- |
| `db9d3b2` | `5f1189ac6455600c49440272ec76eb536c28d0f6870e1e5985971805ee933934` | `product_source_db9d3b2.sha256` |
| `2c6c506` | `0973e247dd30d8555fbc7f7b06b243447f666113094f2ce0cf8e144c51e6c798` | `product_source_2c6c506.sha256` |
| `b6617d2` | `45469ce8ee9dcfa5f0d34b29e576ea8239238404912d5ed3d117cfc687ec1101` | `product_source_b6617d2.sha256` |

## Differential Classification

The historical PASS cannot be compared directly to `2c6c506` or current HEAD as if the source tree were unchanged. Between `db9d3b2` and `2c6c506`, the relevant RVC/frontend scope changed substantially:

```text
include/boom_state.hpp                 | 139 +++++++++-
scripts/gate5_2/run_r2_native_tests.sh |  22 +-
src/branch.cpp                         | 138 ++++++---
src/decode.cpp                         |  22 +-
src/divider.cpp                        |   5 +-
src/fetch_buffer.cpp                   |  89 ++++++
src/fetch_packet.cpp                   | 137 +++++++++
src/frontend.cpp                       | 492 +++++++++++++++++++++++++++------
src/predecode.cpp                      | 105 +++++++
src/predictor.cpp                      | 109 ++++++++
tb/differential/rvc_fetch_tests.cpp    |  34 ++-
```

Classification against the four requested possibilities:

| Possibility | S2 finding |
| --- | --- |
| Product behavior change | Confirmed. `2c6c506` product with historical test and current source list fails 1477 assertions. The first `C/C` failure is present in both historical and current tests, so it is not caused by a newly added assertion. |
| RVC test program change | Confirmed. Historical product with current test fails 864 assertions. The current test moved some protected/decode-gap expectations from frontend rejection to base Decode behavior. |
| Test runner or toolchain change | Runner change confirmed. Historical runner cannot link `2c6c506` product because it omits newer frontend dependencies. Toolchain change is not required to reproduce PASS: `db9d3b2` passes with current g++ 11.4.0. |
| Historical report inconsistent with actual execution | Not primary. The historical PASS report is consistent with `db9d3b2`; it is inconsistent only if incorrectly attributed to `2c6c506`. |

## Test And Runner Differences

Runner change:

The historical runner compiled `rvc_fetch_tests.cpp` with only `frontend.cpp`, `rvc.cpp`, `decode.cpp`, `branch.cpp`, and `divider.cpp`. By `2c6c506`, the runner also required `fetch_packet.cpp`, `fetch_buffer.cpp`, `predecode.cpp`, and `predictor.cpp`, plus an additional `rvc_execute_gap_tests` target. This explains why the exact historical runner cannot be used unchanged against the later product tree.

Test change:

The `C/C` stream and its `request outside constructed image` assertion are unchanged between historical and current tests. Later protected decode-gap expectations did change:

| Area | Historical expectation | Current expectation |
| --- | --- | --- |
| `C.EBREAK` | frontend rejects/faults with illegal instruction cause 2 | frontend passes to Decode; Decode raises breakpoint cause 3 |
| RV64 `C.SRLI shamt[5]` | frontend rejects/faults as protected gap | frontend passes to Decode; Decode accepts non-faulting SRLI |
| `C.JALR` | frontend rejects/faults due frozen backend PC+4 link limitation | frontend passes to Decode with compressed attribution for PC+2 link |

This means the current 1253 failures must not be treated as one homogeneous failure class.

## Representative Failure Analysis

Representative signature: current clean HEAD and `2c6c506` fail first at:

```text
FAIL[9]: C/C: request outside constructed image at 0x20004
```

Input stream:

| Field | Value |
| --- | --- |
| test case | `C/C` |
| base PC | `0x20000` |
| parcels | `0x0001`, `0x0085` |
| constructed memory image | one 32-bit word at `0x20000` only |
| expected requests | aligned request(s) only inside constructed image, therefore `0x20000` |
| actual first mismatch | request emitted at `0x20004` |

Expected decode/publication behavior:

| Parcel | Expected PC | Expected behavior |
| --- | --- | --- |
| `0x0001` | `0x20000` | compressed packet, no fetch exception, decompressed instruction published |
| `0x0085` | `0x20002` | compressed packet, no fetch exception, decompressed instruction published |

Current product divergence:

The current frontend builds a two-lane fetch packet from the one response word, advances `fe.pc` to `0x20004`, and can issue another request in the same cycle through the bypass path:

```text
src/frontend.cpp:499: const bool bypass_build_can_issue = packet_built && !fe.prediction_pending;
src/frontend.cpp:500: if ((!fe.stalled || bypass_build_can_issue) && !fe.request_sent &&
src/frontend.cpp:503:     req.address = fe.halfword_valid ?
src/frontend.cpp:504:         ((fe.halfword_pc + 2) & ~0x3ULL) : (fe.pc & ~0x3ULL);
```

Related historical behavior:

At `db9d3b2`, the frontend has no fetch-buffer/fetch-packet/predictor composition. The same `C/C` test and assertion pass because the old scalar flow drains the two compressed parcels without requesting `0x20004` before the test completes.

Interpretation:

The first `C/C` failure is a product fetch-policy drift relative to the historical one-word test contract. It may represent either an allowed later prefetch that the legacy test no longer tolerates, or a frontend request-gating regression if the intended contract forbids requesting beyond the constructed fetch image while buffered lanes remain. This S2 investigation identifies the provenance and first divergent behavior; it does not change product code or weaken the test.

Representative test-scope drift signature:

Historical product `db9d3b2` with the current test fails first at:

```text
FAIL[1077]: C.EBREAK was rejected before base Decode
```

This is explained by the test diff: the historical test expected frontend rejection for protected decode gaps, while the current test expects base Decode to handle `C.EBREAK`, RV64 `C.SRLI shamt[5]`, and `C.JALR`. That is a real test semantic change, independent of the `C/C` prefetch signature.

## Conclusions

Historical PASS source identity:

```text
PRODUCT_COMMIT=db9d3b22fb3672751d9e012cf3130296cbf19f3a
PRODUCT_SOURCE_SHA256=5f1189ac6455600c49440272ec76eb536c28d0f6870e1e5985971805ee933934
TEST_SOURCE_SHA256=5d134a4c430f6fd049954a5179e156df4744551d637cacfdcd37a5178b52ff2c
RUNNER_SHA256=2bcb5194927677e8e0dda21cc61c413656de472678f7a023bcba4f125c67e65c
COMPILER_VERSION=g++ 11.4.0
RESULT=PASS
```

Final S2 status fields:

```text
RVC_HISTORICAL_PASS_SOURCE_FOUND=true
RVC_HISTORICAL_RUNNER_FOUND=true
RVC_HISTORICAL_PASS_REPRODUCED=true
RVC_BASELINE_PROVENANCE_IDENTIFIED=true
RVC_FAILURE_ROOT_CAUSE_IDENTIFIED=partial_mixed_product_frontend_fetch_policy_and_test_scope_drift
RVC_PRODUCT_CODE_MODIFIED=false
```

Recommended next decision:

Do not treat `2c6c506` as a clean RVC PASS baseline. Establish a new RVC baseline policy first: either preserve `db9d3b2` as the RVC PASS baseline and run migration deltas forward gate-by-gate, or formally accept the later fetch-buffer/predictor behavior and update the RVC focused tests to distinguish allowed prefetch from illegal fetch side effects.
