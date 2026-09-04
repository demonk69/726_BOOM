# Future Fetch Preservation Policy

## Required Versioning

1. `GATE5_3_B3I_ACCEPTED_CONTRACT` must run `scripts/gate5_3/run_b3i_random.sh` with the accepted test hash and packet-aware source set recorded in `accepted_runner_manifest.md`.
2. `CURRENT_FRONTEND_CONTRACT` may add exception/trap/redirect scenarios, but it must use a packet-aware oracle and report its own version, source hashes, compile command, seed list, and cycle count.
3. `GATE5_3_B2_SCALAR_LEGACY_CONTRACT` may be retained only as historical B2 evidence. It must not be interpreted against the B3I packet producer without an explicitly versioned packet-aware rewrite.

## Harness Rules

- Every preservation result must emit runner path, test path, commit/source manifest, SHA-256 values, full compile command, defines, seed range, cycle count, marker, and all counters.
- A report must not transfer an expected value between different marker names or test hashes.
- The accepted B3I test is a reliable frozen harness: it reproduced the accepted log byte-for-byte on `a48e527`, passed current baseline-equivalent source, and passed PF1 source.
- Current-contract expansion must be additive. Assertions and expected semantics must not be weakened to obtain PASS.
- Product-source failures and harness-contract failures must be reported separately.

## Release Policy

PF2 may consume this PR0 conclusion because the root cause is explained, PF1 passes the accepted Gate 5.3 contract, and the existing versioned B3I runner is reliable. This policy does not authorize Predictor/FTQ work inside PR0 and does not alter any product or test source.
