# Multiply Microarchitecture

## M2A Boundary

M2A provides a stateless standalone arithmetic function. `MulRequest` contains only valid, operation, lhs, and rhs; `MulResponse` contains valid and result. It carries no `MicroOp`, ROB identity, destination, branch mask, processor state, completion event, or reset state.

This boundary deliberately prevents M2A from claiming INT-lane or W4 completion integration.

## Arithmetic Structure

The synthesis path uses one unsigned 64x64 product for MUL, MULHU, and the unsigned basis of MULHSU; one signed 64x64 product for MULH; and one 32x32 product for MULW. MULHSU corrects the unsigned high half by subtracting rhs when lhs is negative.

The default M2A HLS result contains three multiplier instances and 33 DSPs. Sharing or binding experiments are deferred until integration-level M2 work; M2A applies no latency-changing directive.

## Future Integration

M2B may map uopcs 16-20 to `MulOperation` and place the result in the existing INT execute result slot. It must not add a completion source or directly update PRF, wakeup, or ROB state.
