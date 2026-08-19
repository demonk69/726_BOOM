# Gate 5.3 Architecture Freeze

## Canonical Configuration

| Item | Frozen value |
|---|---|
| Fetch Buffer depth | 8 complete instructions |
| Storage policy | AUTO |
| Reset policy | CONTROL_ONLY |
| Packet width | 2 instruction lanes |
| IMEM response | 32 bits, two 16-bit parcels |
| Decode width | 1 |
| Dispatch width | 1 |
| Logical outstanding IMEM requests | 1 |
| Packets built per matched response | At most 1 |
| Multi-response packet aggregation | Disabled |

## Ownership Boundary

The Fetch Buffer is an instruction FIFO. Each occupied entry is a complete `FetchInstruction` containing PC, canonical instruction, original instruction, fetch ID, RVC indication, and fetch-fault metadata. Head, tail, and count determine validity. Payload contents outside the occupied range are semantically unreachable, so control-only reset is sufficient.

The Frontend, not the Fetch Buffer, owns:

- outstanding request identity, expected address, and epoch;
- retained matched response state;
- the optional cross-word 16-bit carry, carry PC, and carry epoch;
- packet construction and pending all-or-nothing admission;
- redirect priority, stale-response draining, and carry invalidation.

One matched response and optional prior carry produce no more than one packet. Legal packet masks are `00`, `01`, and `11`; `10` is illegal. A packet is accepted only when every valid lane fits. Multi-response aggregation is explicitly outside the frozen design.

Decode dequeues at most one instruction per architectural call/cycle interface opportunity. Packet width 2 therefore improves response unpacking, queue admission, and burst absorption; it does not widen Decode, Dispatch, execute, writeback, or commit.

## Excluded State

The frozen Fetch Buffer contains no raw response words, partial instructions, outstanding request state, FTQ index ownership, predictor history, RAS state, branch target metadata, ICache state, or memory refill state. Existing `BranchInfo.ftq_idx` is not evidence of an implemented FTQ contract.

## Change Control

Future work must not silently alter this boundary. A change to response width, packet width, aggregation, queue payload, Decode width, or request concurrency requires a separate architecture and verification gate. Gate 5.4 may review prerequisites but is not authorized to implement FTQ, Predictor, RAS, or ICache.
