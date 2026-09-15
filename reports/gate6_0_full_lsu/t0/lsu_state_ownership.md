# Current LSU State Ownership

`LsuState` in `include/boom_state.hpp:233-264` is mutated by LSU completion acceptance/issue/reclamation, branch recovery, staged reset, and exception cleanup. The ROB is the authoritative memory-operation record (`boom_types.hpp:169-183`); LDQ/STQ entries are secondary ownership/lifetime records keyed by `{rob_idx,rob_allocation_id}`.

## LQ

- Depth 8; estimated logical entry bits 221.
- Fields: five flags; 8-bit ROB index, size and branch mask; 32-bit transaction/allocation IDs; 64-bit address/result.
- No destination or exception field; those are in the uop/ROB. Several declared response/result/status fields are currently inert.
- Allocate when MEM execute completion is accepted (`lsu.cpp:96-112,159-182`), not at dispatch.
- Free only on accepted identity-matching response (`lsu.cpp:115-145`).
- Branch squash compacts survivors and cancels a matching pending tuple (`branch.cpp:157-197`).
- Exception flush reconstructs LSU while preserving `next_transaction_id` (`commit.cpp:35-60`).
- Reset clears pointers, pending tuple, valid and response-pending state (`reset.cpp:242-264`).

## SQ

- Depth 8; estimated logical entry bits 199.
- Fields: seven flags; 8-bit ROB index, mask, size and branch mask; 32-bit allocation ID; 64-bit address/data.
- Allocate atomically on accepted store execute completion. Address and data become ready together.
- Commit point and memory issue point are the nonbusy ROB-head commit path (`commit.cpp:215-236`).
- Free immediately after the committed store request is enqueued; no store response is tracked.
