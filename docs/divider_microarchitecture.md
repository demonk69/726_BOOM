# Divider Microarchitecture

## M3A Standalone Core

The standalone divider uses unsigned restoring radix-2 division. `divider_accept` normalizes signed operands to magnitudes, and every `divider_step` shifts one dividend bit into the remainder, compares it with the divisor, conditionally subtracts, and appends one quotient bit. No synthesizable `/` or `%` expression is used.

- Full-width operations execute exactly 64 steps.
- Word operations truncate both inputs first and execute exactly 32 steps.
- Divide by zero, signed overflow, zero dividend, divisor one, and signed divisor minus one complete during acceptance with zero arithmetic iterations.
- `busy` blocks another request. A completed result sets `result_pending`, remains bit-stable under arbitrary backpressure, and keeps request ready low until consumed.
- Reset clears busy work and any pending response. All core state is passed explicitly; only `synth_divider_top` owns a static state instance.

The 65th restoring-remainder bit is represented by the pre-shift remainder MSB. If that bit is set, subtraction is mandatory; the wrapped 64-bit subtraction is the low 64 bits of the correct 65-bit result.

M3A is intentionally standalone. It has no `MicroOp`, ROB, PRF, execute, completion, wakeup, bypass, or full-core cycle dependency.

## M3B INT Integration

Canonical `ExecuteState` owns one `DividerExecutionState`, which combines the M3A arithmetic state with the accepted `MicroOp` and explicit ROB index, physical destination, allocation ID, and branch mask. `issue_module` accepts a DIV/REM only when the arithmetic request is ready and the dynamic ROB allocation owner matches. Every later execute cycle advances at most one restoring iteration.

ALU, MUL, branch, and MEM activity may continue while arithmetic is busy. A terminal result remains in the M3A `result_pending` register until the existing INT execute slot is empty; only then is it copied into lane 1 and consumed. It subsequently uses `completion.int_execute` and the unchanged W4 PRF, wakeup, bypass, and ROB-complete machinery. See `divider_integration_state.md` and `divider_completion_arbitration.md`.
