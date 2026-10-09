# L3R2 Behavior Comparison

## Summary

CONFIG_4_4 matches the L3 multi-outstanding LSU contract across Native C++, HLS CSim, and generated RTL evidence.

Gate-wide status: `G6_0_L3_MULTI_OUTSTANDING_LOADS_VERIFIED=false` because only CONFIG_4_4 generated RTL was simulated in this L3R2 run.

Native evidence: `logs/config_4_4_native.log`

CSim evidence: `logs/config_4_4_csim.log`

RTL evidence: `logs/config_4_4_simulation.log`

Native all-config evidence: `logs/native_all/`

CSim all-config evidence: `logs/csim_all/`

## Native And CSim

Native CONFIG_4_4 passed:

`G6_L3_MULTI_OUTSTANDING_PASS LQ=4 SQ=4 failures=0`

Native five-config sweep also passed in the clean source worktree:

`G6_L3_NATIVE_ALL=PASS`

Native configs passed: `4_4`, `8_8`, `16_16`, `4_16`, `16_4`.

HLS CSim CONFIG_4_4 passed:

`G6_L3_MULTI_OUTSTANDING_PASS LQ=4 SQ=4 failures=0`

HLS CSim five-config sweep also passed:

`G6_L3_CSIM_ALL=PASS`

CSim configs passed: `4_4`, `8_8`, `16_16`, `4_16`, `16_4`.

Native/CSim contract coverage includes two-response orders, three outstanding requests, max outstanding, stale/duplicate TXIDs, TXID wrap/active-duplicate skip, forwarding/backpressure, same-LQ double issue prevention, bounded exhaustive CONFIG_4_4 response matching, and random model checks.

## RTL Scoreboard

RTL CONFIG_4_4 passed:

`G6_L3_RTL_CONFIG_4_4_PASS cases=5 requests=5 responses=6`

RTL cases:

1. `max_issue_four`: four independent loads issued with TXIDs `0x10..0x13`, `state_lsu_outstanding_load_count_V == 4`.
2. `out_of_order_first`: response for TXID `0x12` completes before older TXIDs and reclaims LQ slot 2.
3. `reissue_after_free`: a new load reuses freed LQ slot 2 and receives TXID `0x14`.
4. `stale_old_txid_reject`: late duplicate response for old TXID `0x12` does not complete the new owner.
5. `remaining_out_of_order_complete`: remaining TXIDs complete out of order and final outstanding count returns to zero.

The RTL testbench drives the generated AXIS DMEM ports and checks generated internal ownership state. It uses hierarchical seeding of ROB/LQ state, matching the established Gate6 L2 Path B focused-RTL method, to isolate LSU ownership/response behavior from frontend program loading.

## Comparison Result

The RTL run exercises the same core invariants accepted by Native/CSim for CONFIG_4_4: multiple simultaneous LQ owners, unique TXID allocation, out-of-order response ownership, slot reuse generation separation, stale TXID rejection, ROB memory completion, and final outstanding-count drain.

No Native/CSim/RTL disagreement is recorded for CONFIG_4_4.

## Scope Limit

This comparison is a focused CONFIG_4_4 RTL proof. Native covers all five L3 parameter configs, but this L3R2 RTL report only claims generated-RTL PASS for CONFIG_4_4.
