# RV64M Execution Semantics

## M1 Decode Contract

All RV64M instructions use `IQ_ALU`, integer source/destination metadata, and the fixed INT-compatible issue lane. Multiply operations use `FU_MUL`; divide/remainder operations use `FU_DIV`. Word operations set `fcn_dw=1`; full-width operations set it to zero. `op_fcn` retains `funct3`.

M1 verifies this metadata only. It does not establish correct arithmetic execution.

## Required M2 Multiply Semantics

- MUL: low 64 bits of a 64x64 product.
- MULH: high 64 bits of signed by signed multiplication.
- MULHSU: high 64 bits of signed by unsigned multiplication.
- MULHU: high 64 bits of unsigned by unsigned multiplication.
- MULW: low 32 product bits, sign-extended to 64 bits.

## Required M3-M5 Divide Semantics

Division must use persistent fixed-iteration radix-2 state, not synthesizable C++ `/` or `%`. Signed division truncates toward zero and signed remainder has the dividend sign. Divide by zero, signed minimum divided by -1, and all word sign-extension cases must follow the RISC-V specification.

The terminal result must enter the existing INT execute result and W4 completion/writeback path. Reset, branch kill, response retention, and ROB allocation identity remain mandatory.

## M3A Standalone Divide Semantics

M3A implements arithmetic and a persistent request/response protocol, but does not perform the later execute or W4 integration.

- DIV and DIVW are signed, truncate toward zero, and return the all-ones quotient for division by zero.
- DIVU and DIVUW are unsigned and return the all-ones quotient for division by zero.
- REM and REMW use the dividend sign and return the dividend for division by zero.
- REMU and REMUW are unsigned and return the dividend for division by zero.
- Minimum signed value divided by minus one returns the minimum value; its remainder is zero.
- Every W operation truncates inputs to 32 bits and sign-extends the low 32 result bits, including unsigned W operations.

## M3B Integrated Divide Semantics

Uopcs 21 through 28 map one-to-one to the eight `DivideOperation` values and execute through the fixed INT lane. Divide by zero is an architectural result, never an exception. A response is publishable only while its branch mask survives and its nonzero allocation ID still owns the saved ROB index. Reset, mispredict kill, global exception flush, or stale allocation invalidates the token without writeback, wakeup, bypass, or ROB completion.
