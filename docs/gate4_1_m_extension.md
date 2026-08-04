# Gate 4.1 M Extension

## M1 Decode

`M1_RV64M_DECODE_VERIFIED=true` at the source state documented in `reports/gate4_1/m1/`.

- 13/13 legal RV64M encodings decode exactly.
- 10/10 required near-miss vectors are illegal with cause 2.
- 15/15 base OP/OP-32 collision vectors remain legal and unchanged.
- 4/4 `rd=x0` M vectors remain legal decode operations.
- MUL-family uops classify to `FU_MUL`; DIV/REM-family uops classify to `FU_DIV`; both are INT-compatible only.
- Software, random, trace, architecture, partial-order, merged compile, and conservative csynth preservation gates pass.
- `CORE_CYCLE` remains unpipelined.

`READY_FOR_M2_MUL_FAMILY=true` means only that M2 may begin. It is not an RV64M execution claim. Divider work remains prohibited until its later phase.

## M2A Standalone Multiply

`M2A_STANDALONE_MUL_ARITHMETIC_VERIFIED=true` verifies a stateless arithmetic module for MUL, MULH, MULHSU, MULHU, and MULW. Directed tests pass 51/51 and 256 fixed seeds cover 131072 vectors with zero mismatch. Standalone `synth_mul_top` completes at 6.463 ns with 612 LUT, 7 FF, 0 BRAM, and 33 DSP.

`READY_FOR_M2B_INT_INTEGRATION=true`. `execute_module` and W4 completion are unchanged, so neither full-core multiplication nor `M2_MUL_FAMILY_VERIFIED` is claimed.
