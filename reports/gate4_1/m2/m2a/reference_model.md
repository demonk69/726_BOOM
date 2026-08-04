# M2A Independent Reference Model

The directed and random tests do not derive expected values from DUT output.

- MUL and MULHU use an independent `unsigned __int128` product.
- MULH converts both 64-bit bit patterns to signed operands, multiplies as `signed __int128`, converts the product to unsigned representation, and selects bits 127:64.
- MULHSU first computes an unsigned 128-bit product. If the signed lhs has bit 63 set, the expected high half is reduced by the unsigned rhs. This follows from `signed(lhs) = unsigned(lhs) - 2^64`.
- MULW multiplies independently truncated `uint32_t` operands, truncates the product to 32 bits, interprets bit 31 as sign, and extends that signed 32-bit value to 64 bits.

The random model uses a separate xorshift64* PRNG and injects zero, one, all-ones, signed extrema, bit 63, bit 31, sparse, dense, and carry-heavy patterns.
