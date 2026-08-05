# Gate 4.1 M3A Results

## Status

- `M3A_STANDALONE_DIVIDER_VERIFIED=true`
- `READY_FOR_M3B_INT_DIVIDER_INTEGRATION=true`
- `GATE4_1_RV64M_VERIFIED` is not set.
- `READY_FOR_M4` is not set.
- `READY_FOR_FRONTEND_IMPLEMENTATION` is not set.

## Required Answers

1. Divider algorithm: unsigned restoring radix-2 over normalized magnitudes, one shift/compare/subtract step per call.
2. 64-bit iterations: exactly 64 for normal cases.
3. 32-bit iterations: exactly 32 for normal W cases.
4. Fast returns: divisor zero, signed overflow, zero dividend, divisor one, and signed divisor minus one; zero arithmetic iterations while retaining the response protocol.
5. DIV: signed quotient truncated toward zero.
6. DIVU: unsigned 64-bit quotient.
7. REM: signed remainder with the original dividend sign.
8. REMU: unsigned 64-bit remainder.
9. DIVW: low inputs interpreted as signed 32-bit, signed quotient, low result sign-extended.
10. DIVUW: low inputs interpreted as unsigned 32-bit, quotient low word sign-extended.
11. REMW: signed low-word remainder, low result sign-extended.
12. REMUW: unsigned low-word remainder, low result sign-extended.
13. Divide by zero: quotient is all ones; remainder is the original dividend at operation width; every W result is sign-extended.
14. Signed overflow: minimum divided by minus one returns minimum; remainder is zero at both widths.
15. Response holding: `result_pending` keeps valid and result stable until explicit consume and keeps request ready low.
16. Reset: immediately clears busy work or a pending response; no killed response reappears.
17. Directed result: 104/104 PASS.
18. Random result: 65536 operations, 8192 per operation, zero mismatch.
19. Protocol random result: 65536 cycles, zero mismatch, 1228 held-response checks.
20. `synth_divider_top` resources: 3429 LUT, 344 FF, 0 BRAM.
21. DSP count: 0.
22. Estimated period: 5.734 ns.
23. Combinational div/rem inference: no; source and bind/operator audits report zero division/remainder operators.
24. Final state: `M3A_STANDALONE_DIVIDER_VERIFIED`.
25. Integration release: `READY_FOR_M3B_INT_DIVIDER_INTEGRATION=true`.

## Scope Audit

M3A does not integrate the divider into execute, INT-lane completion, W4, ROB, PRF, wakeup, or bypass. It runs neither full-core RTL nor full-core csynth. `CORE_CYCLE` remains unpipelined. The canonical merged source contains `divider.cpp` exactly once and excludes `src/boom_all.cpp`.
