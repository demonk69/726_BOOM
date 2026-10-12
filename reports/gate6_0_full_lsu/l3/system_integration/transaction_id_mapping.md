# PPA4 Transaction ID Mapping

Scope: transaction ownership for the experimental PPA4 memory-system adapter.

## DMEM Request Decode

| Field | Bit range | Use |
| --- | --- | --- |
| Transaction ID | `[31:0]` | Stored in the pending table for load responses. |
| Command | `[47:40]` | `8'd1` is treated as store. |
| Mask | `[63:56]` | Store byte mask fallback. |
| Write mask | `[71:64]` | Preferred store byte mask when nonzero. |
| Store indicators | `[88]`, `[104]` | Also treated as store indicators. |
| Address | `[191:128]` | Used for range checks, latency selection, and store-forwarding lookup. |
| Data | `[255:192]` | Store data fallback. |
| Write data | `[319:256]` | Preferred store data when nonzero. |

## Ownership Rules

| Rule | Status | Evidence |
| --- | --- | --- |
| Accepted load requests allocate a pending-table slot. | PASS | Adapter scoreboard `dmem backpressure when pending table is full`. |
| Response TXID equals the accepted request TXID. | PASS | Adapter scoreboard `out-of-order response ownership` and `post-reset transaction ownership`. |
| Variable response latency may reorder independent loads. | PASS | Address bit `[4]` selects 2-cycle or 6-cycle latency; scoreboard observes TXID `0x22` before `0x11`. |
| Reset clears pending ownership state. | PASS | Adapter scoreboard `reset stale response suppression`. |
| Stores do not allocate response entries. | PASS | Store-forwarding path updates compact store buffer only. |
| Store data is forwarded to later matching loads. | PASS | Adapter scoreboard `store forwarding through memory image`. |

## Limits

The adapter proof is not a full software workload proof. It verifies the package wrapper protocol obligations and transaction ID preservation, but `G6_0_L3_MULTI_OUTSTANDING_LOADS_VERIFIED=false` remains for final package acceptance until integrated full-core/system workload evidence is produced.
