# Fetch Buffer Microarchitecture

## Canonical Gate 5.3 Design

The accepted Fetch Buffer is an eight-entry FIFO of complete instructions between Frontend response unpacking and one-wide Decode. It accepts one packet with up to two valid instruction lanes and dequeues at most one instruction per opportunity.

| Property | Canonical value |
|---|---|
| Depth | 8 complete instructions |
| Storage | AUTO |
| Reset | CONTROL_ONLY |
| Packet width | 2 |
| IMEM response | 32 bits / two 16-bit parcels |
| Decode / Dispatch width | 1 / 1 |
| Outstanding requests | One logical request |
| Multi-response aggregation | Disabled |

Each entry contains PC, canonical and original instruction, fetch ID, RVC indication, and fetch-fault metadata. The FIFO owns only complete instructions plus head, tail, and count. It does not store raw response words, partial instructions, request state, FTQ metadata, predictor history, or cache state.

Frontend owns the optional cross-word 16-bit carry and its PC/epoch. One matched response plus optional carry builds at most one packet. Legal packet masks are `00`, `01`, and `11`; `10` is illegal. Packet enqueue is atomic: all valid lanes fit or none are admitted. A dequeue in the same operation contributes one free entry.

Flush and runtime reset clear head, tail, and count. Payload reset is unnecessary because entries outside the occupied FIFO range are unreachable. Redirect handling also invalidates pending packet, retained response, request identity, and carry in Frontend state.

Packet width 2 improves parcel utilization and burst admission for RVC-rich streams. It does not widen Decode, Dispatch, execute, writeback, or commit and therefore carries no backend IPC claim.

See `reports/gate5_3_fetch_buffer/final/architecture_freeze.md` and `reports/gate5_3_fetch_buffer/final/packet_width_decision.md`.
