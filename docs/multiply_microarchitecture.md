# Multiply Microarchitecture

## M2A Boundary

M2A provides a stateless standalone arithmetic function. `MulRequest` contains only valid, operation, lhs, and rhs; `MulResponse` contains valid and result. It carries no `MicroOp`, ROB identity, destination, branch mask, processor state, completion event, or reset state.

This boundary deliberately prevents M2A from claiming INT-lane or W4 completion integration.

## Arithmetic Structure

The synthesis path uses one unsigned 64x64 product for MUL, MULHU, and the unsigned basis of MULHSU; one signed 64x64 product for MULH; and one 32x32 product for MULW. MULHSU corrects the unsigned high half by subtracting rhs when lhs is negative.

The default M2A HLS result contains three multiplier instances and 33 DSPs. Sharing or binding experiments are deferred until integration-level M2 work; M2A applies no latency-changing directive.

## M2B Integration

M2B maps uopcs 16-20 to `MulOperation` by subtracting the first multiply uopc and places the response in the existing INT execute result slot. The result retains its original `MicroOp` and follows `completion.int_execute`; multiplication does not directly update PRF, wakeup, bypass, or ROB state.

The integration adds 30 DSPs to `synth_execute_top` and `synth_core_step_top`. The integrated estimates are 7.201 ns and 7.257 ns respectively, so M2B is functionally verified but PPA-blocked at the 6.5 ns threshold. No sharing, binding, pipeline, dataflow, dependence, or array-partition optimization was attempted.
