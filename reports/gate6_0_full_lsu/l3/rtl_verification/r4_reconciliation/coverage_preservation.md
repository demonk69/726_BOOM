# Coverage Preservation

R4 preserves the original R2 stress scenarios and adds reconciled checks for the three R2 blockers.

The final RTL logs report `cases=8` because the testbench increments the case counter for assertion groups. The emitted `G6_L3_R2_CASE_PASS` evidence is finer grained: each final config log contains 12 PASS markers and 0 FAIL markers.

Original R2 scenario mapping:

| Original R2 scenario | R4 evidence marker |
| --- | --- |
| `MAX_LQ_SATURATION` | `outstanding_slot_saturation`, `max_saturation_reuse_stale` |
| `ARBITRARY_OUT_OF_ORDER_RESPONSE` | `arbitrary_out_of_order_first` |
| `LQ_RELEASE_IMMEDIATE_REUSE` | `lq_release_immediate_reuse` |
| `STALE_RESPONSE_REJECTION` | `stale_response_after_lq_reuse` |
| `TXID_WRAP_DUPLICATE_SKIP` | `txid_wrap_duplicate_skip` |
| `CONSECUTIVE_RESPONSE_HANDLING` | `response_backpressure_consecutive` |
| `RESET_WITH_PENDING_TRANSACTIONS` | `reset_with_pending` |

Original PASS scenarios retained in final logs for all five configs:

- `outstanding_slot_saturation`
- `arbitrary_out_of_order_first`
- `lq_release_immediate_reuse`
- `stale_response_after_lq_reuse`
- `txid_wrap_duplicate_skip`
- `response_backpressure_consecutive`
- `reset_with_pending`

Reconciled scenarios now PASS for all five configs:

- `request_backpressure`
- `store_forwarding`
- `store_forwarding_partial_negative`
- `flush_with_pending`

Concurrency was not reduced:

- LQ4 configs still reach `max_observed_outstanding=4`.
- LQ8 configs still reach `max_observed_outstanding=8`.
- LQ16 configs still reach `max_observed_outstanding=16`.

The R4 harness did not mask failures by adding unbounded waits. Each reconciled check validates transaction payload, owner tuple, completion state, and request/response counts. The partial-cover negative test is bounded and checks that the load does not incorrectly complete through the forwarding-only path.

The stale-response and flush checks do not ignore responses. They inject stale traffic after exception recovery and assert that it causes no ROB completion and no PRF writeback.

Reset, exception recovery, and branch mispredict recovery are not mixed:

- `flush_with_pending` uses exception commit recovery.
- `reset_with_pending` uses reset only.
- Branch mispredict recovery is not used as a substitute for either path in R4.

Final audit result: original coverage preserved, R3 blockers reconciled, and concurrent outstanding pressure preserved at each configured LQ depth.
