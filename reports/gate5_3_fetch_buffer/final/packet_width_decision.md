# Packet Width Decision

## Decision

`PACKET_WIDTH_2_ACCEPTED`

The canonical packet width is two complete instructions. This is the maximum useful width for the frozen 32-bit response and two 16-bit parcels when multi-response aggregation is disabled.

## Why Width 2 Is Retained

- All-RVC responses can produce two complete instructions. The accepted campaign produced 256 `mask_11` packets for 512 instructions and 100% packet-slot utilization.
- Alternating, 75%-RVC, and branch-heavy streams produce material `mask_11` traffic, with 1.333, 1.6, and 1.6 valid slots per packet respectively.
- Atomic two-entry admission lets the depth-eight buffer absorb response production in its natural granularity.
- Timing remains 6.341 ns and BRAM/DSP are unchanged. Full-core LUT rises 4.672% and FF rises 14.315%; this is bounded and explicitly accepted rather than hidden.
- Width 1 would discard a complete second RVC instruction from a response or require an additional holding/serialization structure. That would create a third architectural state owner rather than simplify the frozen response contract.

## Limits Of The Benefit

All-32 and cross-word-heavy streams produce no useful second lane. Decode and Dispatch remain one wide, so the accepted native campaigns make no end-to-end speedup claim and show small finite-startup regressions. Under sustained Decode stalls, any finite buffer fills and long-run throughput converges to the one-wide consumer rate.

Width 2 is therefore accepted for response utilization, clean ownership, atomic packet admission, and burst fill, not for backend IPC. Future area pressure may reopen packet width only through a separate gate with matched functional, RTL, throughput, and PPA evidence.

## Rejected Alternatives

- Width 1: lower packetization area but loses natural two-complete-instruction response admission and requires serialization state for the second instruction.
- Width 4: unsupported by a single 32-bit response, provides no additional parcel capacity, and would imply forbidden multi-response aggregation or empty lanes.
- Multi-response aggregation: rejected because it changes request concurrency, buffering ownership, fault ordering, redirect invalidation, and verification scope.
