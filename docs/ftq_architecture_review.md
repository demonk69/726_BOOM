# FTQ Architecture Review

Gate 5.4 F0 is a read-only prerequisite review. No FTQ is implemented. The current core carries a full 64-bit PC in every MicroOp, has no predicted direction/target/history producer, and uses static sequential/not-taken fetch. Existing `FTQ_DEPTH=16`, `FTQ_IDX_BITS=4`, and `BranchInfo.ftq_idx` are inactive placeholders.

The recommended future entry corresponds to one nonempty two-lane Fetch Packet accepted atomically by the Fetch Buffer. It is allocated at Fetch Buffer enqueue, uses the oldest instruction start as base PC, and is reclaimed by a 2-bit live-instruction count decremented on commit or squash. Carry remains Frontend-owned and never allocates an entry.

Recommended uop reference is a 5-bit index plus a 2-bit halfword offset. This reconstructs ordinary C+C (`+0,+2`) and cross-boundary carry+C (`+0,+4`) without an FTQ cross-boundary flag. `is_rvc` remains per-uop for PC+2 link behavior.

FTQ does not own instruction payload/faults, Frontend request/response/carry, branch mask/tag, rename snapshots, ROB allocation identity, exception state, or PRF state. Predictor direction/target/history/RAS/update metadata remains absent and must not be added until its interface is reviewed.

Depth 32 is recommended: a legal packet can contain only one instruction, so a 32-entry ROB can reference 32 packets. Depth 16 covers a full ROB only in the all-two-lane case and is not capacity-safe. Allocation at enqueue can make FTQ full with frontend-resident packets before ROB count reaches 32, but already allocated packets remain dispatchable; supporting a simultaneous full ROB plus eight one-lane Fetch Buffer packets would require 40 entries and is outside the F0 candidate sweep.

Product integration is deferred. Standalone implementation is architecturally ready but should follow predictor-interface review. See `reports/gate5_4_ftq/f0/f0_results.md` for gate status.

## F1 Standalone Foundation

Gate 5.4 F1 implements the standalone queue without changing the product path.
The post-P2 entry is 211 logical bits: packet base PC/mask, a two-bit live-lane
mask, P2 prediction validity/direction/target, CFI lane/type, the exact 8-bit
BIM metadata index, the 32-bit P2 predictor generation, valid, and a separate
32-bit per-allocation generation. The P2 request token is transient and is not
retained. The live mask replaces a numeric-only count so duplicate retirement
can be rejected; its popcount is the contractual live-uop count.

The queue is a count-qualified ring and supports atomic packet allocation,
one ordered head reclaim per step, simultaneous head reclaim/allocation,
redirect owner retention, same-packet younger-lane kill, younger suffix
invalidation, runtime reset, wrap, and stale reference rejection. Canonical F1
is depth 32, LUTRAM, and CONTROL_ONLY reset. It remains standalone; PF0 must
review product exception/recovery ownership before any integration.
