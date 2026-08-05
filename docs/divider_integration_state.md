# Divider Integration State

M3B places exactly one `DividerExecutionState` in canonical `ExecuteState`. It contains the reusable M3A `boom::DividerState`, one valid token, the complete `MicroOp`, and explicit `rob_idx`, `pdst`, `allocation_id`, and `branch_mask` identity fields. No divider state exists in a function-local static, uncontrolled global, or second processor module.

`divider_accept` captures the dynamic identity only after the INT issue grant has a live, busy ROB owner with a matching nonzero allocation ID. Busy or response-pending arithmetic keeps a new DIV/REM request in the IQ. The saved allocation owner is revalidated every execute cycle and immediately before response publication.

Fine-grained reset calls `divider_reset` in both `RESET_CONTROL` and `RESET_EXECUTE`, invalidates the saved token and identity, and clears all execute/completion visibility. Payload clearing beyond those validity barriers is not architecturally required, but the current implementation initializes the wrapper fields as well. Runtime reset therefore cancels active arithmetic and held results; no pre-reset divider token can publish afterward.

Branch recovery clears correctly resolved mask bits in both saved identity views. A mispredict whose mask intersects the saved token resets the arithmetic and invalidates the token. Global exception/flush handling conservatively cancels the divider; this supported subset does not claim the finer exception overlap behavior of full BOOM.
