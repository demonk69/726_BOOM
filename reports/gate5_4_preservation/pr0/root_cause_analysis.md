# Root Cause Analysis

## Classification

```text
PRESERVATION_GAP_ROOT_CAUSE=RUNNER_DRIFT
INITIAL_TAXONOMY=D_EXPECTATION_DRIFT
```

The immediate operational defect is runner selection drift: PF1 preservation selected the B2 scalar test instead of the Gate 5.3 accepted B3I runner. The apparent expected-zero contradiction is expectation drift because the zero result from one test/contract was compared with counters from a different test/contract. The test files themselves did not drift.

## Matrix Proof

- A passes: accepted source plus accepted test produces all-zero errors and byte-for-byte reproduces the accepted log.
- B fails: accepted source plus current-selected B2 test produces exactly `drop=425388`, `ordering_error=174170`.
- C passes: current baseline-equivalent source plus accepted test produces all-zero errors.
- D fails: current baseline-equivalent source plus current-selected B2 test reproduces exactly the same failure as B.
- Additional PF1 check passes: PF1 source hashes plus accepted test produce all-zero errors.

Only the test-version axis changes the verdict. The source-version axis changes neither result nor counters.

## Contract Boundary

B2 accepted a scalar producer: the test reads one old `producer_uop` and predicts at most one enqueue. B3I intentionally introduced `pending_packet`, packet mask 1/3, and atomic admission of up to two complete instructions. Gate 5.3 acceptance consequently introduced `fetch_packet_2lane_random_tests.cpp`, whose independent oracle models two lanes, carry, packet pending, and atomic capacity. The old B2 test was retained in the tree but was not the B3I acceptance runner.

No Gate 5.3-to-current contract change exists in this scope; the relevant committed source/test hashes are identical. PF1 exception recovery changes Frontend redirect ownership, but the packet-aware accepted harness passes on PF1 source. P1/P2/F1 standalone commits do not modify the scoped random tests, runners, or Fetch source.

## Exclusions

- `TEST_DRIFT`: false as file drift; both test hashes are unchanged after acceptance.
- `CONTRACT_DRIFT`: false after the accepted checkpoint; B2-to-B3I evolution predates and defines acceptance.
- `LATENT_GATE5_3_BUG`: false; accepted checkpoint reproduces exactly and current/PF1 source passes accepted test.
- `CURRENT_PRE_PF1_PRODUCT_REGRESSION`: false; A and C are identical PASS.
- `CURRENT_PRODUCT_REGRESSION_PRE_PF1`: false.
- PF1 causation: false.

## Required Action

`RESTORE_ACCEPTED_PRESERVATION_HARNESS`: use `scripts/gate5_3/run_b3i_random.sh` for the frozen Gate 5.3 contract. If B2 historical coverage remains useful, version and label it as a B2-only legacy contract; do not call its packet-era failures Gate 5.3 regressions.

No product repair is indicated. PR1 Fetch Buffer gap repair is not released.
