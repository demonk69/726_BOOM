# RV64M Multiply Semantics

- MUL returns bits 63:0 of the full unsigned 64x64 product; signedness cannot affect the low half.
- MULH sign-extends both 64-bit operands to signed 128-bit values and returns product bits 127:64.
- MULHSU treats lhs as signed and rhs as unsigned. Its high half equals the unsigned-product high half minus rhs when lhs bit 63 is set.
- MULHU zero-extends both operands and returns unsigned product bits 127:64.
- MULW ignores both high 32-bit operand halves, multiplies the low 32-bit values, truncates to 32 result bits, and sign-extends result bit 31 to XLEN.

M2A verifies only these standalone arithmetic semantics. Existing `execute_module` behavior is intentionally unchanged until M2B.
