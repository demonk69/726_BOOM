# Packet Fault Semantics

## Precise Rule

A packet must terminate at the first exception in program order:

1. Older complete normal instructions may occupy lower lanes.
2. The fault occupies the next lane.
3. Every younger lane is invalid and no younger carry is created.
4. A partial instruction never occupies a lane.

Thus conceptual `lane0 normal, lane1 fault, lane2 younger` becomes valid lanes 0 and 1 only. Lane 2 must not enter the Fetch Buffer. This preserves a simple precise-exception boundary even though the ROB could eventually retire an older entry before the fault.

Current ROB allocation marks an exception entry valid and immediately non-busy (`src/rob.cpp:29-53`). Commit reports an exception only when that entry reaches the ROB head, then enters `ROB_EXCEPTION` (`src/commit.cpp:31-65`). Terminating at the fault avoids unnecessary younger state and is stricter than relying on later ROB flushing.

## Fault Classes

- Response-level fetch/access fault: response data is unusable. Without carry, emit one fault at current PC. With carry, emit one fault at saved `halfword_pc`; do not inspect the upper parcel.
- Illegal compressed parcel: emit cause 2 with original compressed bits, then terminate the packet.
- Encoding longer than 32 bits: preserve current explicit illegal-instruction behavior and terminate.
- Odd redirect: emit the existing instruction-address-misaligned fault and issue no request.

For cross-word completion, response B's lower parcel completes the oldest instruction. If B faults, the ordered fault is attributed to the saved lower-half start PC. If B succeeds, completion is lane 0; only then may B's upper parcel become lane 1 or a new carry. Program order is always lane order.

## Backpressure

The constructed packet must be held bit-for-bit stable until atomic Fetch Buffer acceptance or a higher-priority flush. A two-entry packet may not partially enqueue one lane. Existing B1 semantics already accept all valid lanes atomically and compact in ascending lane order (`src/fetch_buffer.cpp:45-80`). B3I should use contiguous masks `01` or `11`, with oldest instruction in lane 0.
