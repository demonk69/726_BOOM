# Packet Construction Architecture Boundary

## Widths Are Different Concepts

- Storage width: B1 accepts up to four masked entries atomically and stores eight entries.
- Current producer width: B2 creates only lane 0 with `valid_mask=1` (`src/frontend.cpp:141-146`).
- Proposed producer width: at most two ordered entries extracted from one matched 32-bit response.
- Consumer width: Fetch Buffer dequeue, Decode, Dispatch, and Commit remain one (`include/fetch_buffer.hpp:44-65`, `include/boom_config.hpp:11-21`).

The B2 queue already decouples Frontend production from Decode. Its measured long-run native C++ rate follows Decode readiness: approximately 1.0 accepted instruction per architectural call when always-ready, 0.75 at 25% stalls, and 0.5 at 50% stalls (`reports/gate5_3_fetch_buffer/b2/decoupling_metrics.csv`). These are not physical-clock IPC values. Packetization can improve burst fill and remove parser bubbles; it cannot increase Decode, Dispatch, or retirement width.

## A. Response Parcel Unpacking

This means examining only the current matched response and existing carry, producing zero, one, or two complete instructions in program order. It needs no second outstanding request and no response FIFO. The current local response retention is sufficient conceptually, although B3I would replace scalar scheduling with one packet hold.

## B. Multi-Response Aggregation

This means waiting across multiple matched responses until enough complete instructions exist to build a wider packet. A fully populated four-lane packet requires this because one response contains only two parcels. The required age/count, fault-termination, flush, and backpressure state is itself a packet buffer in front of the existing Fetch Buffer. It duplicates queue responsibility and delays partial packet publication.

```text
MULTI_RESPONSE_PACKET_AGGREGATION_REQUIRED=true (scope: fully populated width-4 packet only)
MULTI_RESPONSE_PACKET_AGGREGATION_REQUIRED_FOR_WIDTH4=true
```

Width 4 may remain the physical B1 enqueue API, but lanes 2/3 should remain unused by current-IMEM B3I. It must not be advertised as a reachable four-wide producer.

## One-Outstanding Constraint

Frontend locally tracks one exact `{fetch_id, epoch, address}` ownership record. A matching response clears `request_sent`, and a next request may be issued later in the same Frontend call (`src/frontend.cpp:116-131,240-254`). Redirect/reset invalidates local ownership, but an old request may remain externally outstanding while a replacement request is issued; its eventual response is drained stale. A local constructor may process both parcels without changing this one-logically-tracked-request protocol.

Current bottlenecks by stream are:

- all-C: scalar parser scheduling limits publication to one/call even though one response can encode two; width 2 removes this Frontend limit, then Decode width dominates.
- aligned all-32: IMEM payload is the producer limit at one instruction/response; wider enqueue gives no gain.
- alternating/cross-word: parser/carry scheduling and discarded upper completion parcel are current limits. Retaining a new upper carry after completing an old carry is a parser improvement that an optimized width-1 producer could also make. Width 2 is specifically needed when the upper parcel completes a second instruction, such as carry+C.
- stalled Decode: consumer width/readiness dominates after the depth-8 buffer fills.

## Redirect And Reset

Runtime reset, owned architectural redirect, branch redirect, and generic flush must kill the entire unpublished packet, local response, carry, scalar/packet hold, outstanding ownership, and all Fetch Buffer entries. Whole-packet kill is smaller and safer than lane masking. Current Frontend gives redirect priority over response matching and flushes the buffer (`src/frontend.cpp:66-123`); branch recovery clears frontend state and increments epoch (`src/branch.cpp:263-287`). Same-cycle stale responses remain drained with no state effect.

No FTQ, predictor, BTB/BIM/TAGE/RAS, ICache, wider Decode/backend, Full LSU, or FPU is part of this boundary.
