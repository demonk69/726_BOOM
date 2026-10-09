# Gate 6.0 L3R2-R4 Patch Analysis

R3 evidence was frozen in commit `dce3de35d1e3c17b5bda215c641fcb3e4a86f8ed`.

R4 reconciles the R2 stress harness only. No product source was edited by R4.

## Backpressure

The original R2 check treated internal owner reservation as equivalent to an external AXIS request. R3 showed that generated RTL has a `both` mode register slice on `dmem_req_out`, so owner reservation can precede externally visible `TVALID`.

R4 keeps the backpressure test and changes the assertion point to the real external interface contract:

- `dmem_req_out_TVALID && !dmem_req_out_TREADY` must hold `TVALID` and payload stable.
- No handshake may occur while `TREADY=0`.
- When `TREADY` is restored, exactly one load request must handshake.
- The request payload, LQ owner tuple, response, ROB completion, and final load data are checked.

## Store Forwarding

The original R2 check counted all external DMEM traffic and treated an older store request as a load-forwarding failure. R3 separated load commands (`cmd=0`) from store commands (`cmd=1`) and identified a busy-store seed that exercises forwarding-only behavior.

R4 now proves the forwarding seed before checking the result:

- Older store ROB entry is valid and older than the load.
- Younger load ROB and LQ entries are valid.
- SQ owner tuple, allocation, generation, address, data, and mask match the seeded store.
- Full coverage uses `mask=8'hff` and returns `0x123456789abcdef0`.
- No external DMEM load request is generated and no external outstanding slot is consumed.
- Partial coverage remains a negative test and verifies `forwarding_only=0`.

## Flush With Pending

The original R2 flush test directly poked `state_global_flush`, which is not the recovery contract and can be cleared by the core step path.

R4 uses exception commit recovery:

- Two real outstanding loads are issued first.
- ROB0 is marked exception-ready and the test observes `state_exception_commit_valid`, `state_frontend_redirect_valid`, and `state_global_flush`.
- LQ ownership and outstanding count are checked after recovery.
- A stale response is injected and checked for no wrong completion and no PRF writeback.
- A new load is issued and completed after recovery.
- Reset-with-pending remains a separate reset contract check.

## Scope

Modified files:

- `rtl_tb/gate6_0/g6_l3_multi_outstanding_stress_tb.sv`
- `scripts/gate6_0/run_l3_r2_extended_rtl_stress.sh`

Product source was not modified by this R4 work.
