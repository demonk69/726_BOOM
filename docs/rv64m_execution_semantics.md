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
