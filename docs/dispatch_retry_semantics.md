# Dispatch Retry Semantics

The current rename-to-dispatch interface is one persistent packet. `DISPATCH_WIDTH` remains 1 even though the issue queue can grant one MEM uop and one INT uop in the same cycle.

## Packet ownership

`RenameDispatchPacket` contains `valid`, `uop`, and `rob_allocated`.

- Rename may create a packet only when `valid` is clear. Physical-register and branch-tag allocation therefore happen once for that packet.
- A valid packet remains present while ROB allocation or downstream dispatch is blocked. Its ROB fields are filled when the ROB accepts it, and resolved branch-mask bits may be cleared while it is held.
- The ROB accepts only a packet with `rob_allocated == false`. It allocates one ROB slot, assigns a nonzero `rob_allocation_id`, copies both `rob_idx` and `rob_allocation_id` into the ROB entry and packet, and then sets `rob_allocated`.
- Re-entering `rob_allocate` with the same packet does not allocate another entry or advance `next_allocation_id`.
- Issue may consume or queue the packet only after `rob_allocated` is set.

The allocation ID is part of the dynamic ROB identity. A completion is valid only when both its `rob_idx` and `rob_allocation_id` match the live ROB entry; an index match alone is insufficient after ROB-slot reuse.

## Consumption

Issue clears the packet in exactly three cases:

1. Its direct MEM or INT grant is accepted by the corresponding execute lane.
2. It is successfully inserted into the issue queue.
3. It is an exception uop already owned by the ROB, which is its terminal owner.

A packet awaiting ROB ownership remains held while the ROB is full. After ROB allocation, downstream lane or load/store-queue pressure causes a direct grant to be rejected, but the packet can still transfer into a free IQ slot. It remains in the dispatch packet only when it is neither accepted directly nor successfully inserted into the IQ. A retained packet is not also copied into the issue queue or accepted directly.

The direct path handles only this one packet. Independent dual acceptance applies to the fixed MEM and INT issue lanes and can consume two distinct issue-queue grants in one cycle; it does not make rename or dispatch two-wide.

## Upstream backpressure

Frontend and decode hold their state while the dispatch packet is valid. They also hold while an unconsumed decode uop is waiting for rename resources. Rename clears decode valid only after successfully creating the persistent packet.

This structure can insert a bubble after packet consumption because frontend and decode observe the packet state in their ordered core-cycle calls.

## Branches and reset

- Correct branch resolution clears the resolved bit from the packet uop.
- Mispredict recovery clears the packet only when its branch mask intersects the mispredict mask. It also kills matching younger IQ, execute, LSU, and ROB state.
- Runtime reset has priority over normal core execution in the top-level wrapper: while reset sequencing is incomplete, only `boom_core_reset_step` runs. The reset control phase clears the dispatch packet, and reset also clears issue grants and execute completion slots.
- Reset does not rewind the ROB allocation-ID or LSU transaction-ID counters. This prevents ordinary pre-reset identities from immediately aliasing post-reset work.

## Boundaries

- `DISPATCH_WIDTH` and `DECODE_WIDTH` are 1.
- There is no second persistent dispatch packet and no two-uop rename/ROB-allocation path.
- The FP lane is reserved and held unavailable by the current issue/execute path.
- This behavior is W3 dual fixed-lane issue/execute support, not W4 or full wide issue.
