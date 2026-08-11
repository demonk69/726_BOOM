# RVC Reference Method

The decompressor reference is not a copy of the DUT switch.

1. Major classes are selected by a flat, declarative mask/match table
   transcribed from the RV64C opcode map.
2. Expected canonical instructions use independent I/R/S/B/J field encoders.
3. Immediate permutations are transcribed by compressed format, not called
   from production helpers.
4. Directed coverage starts from manually reviewed per-family constants and
   applies six fixed register/immediate boundary patterns, producing 228 checks.
5. The same independent model classifies all 65,536 inputs as supported,
   unsupported, reserved, or non-RVC and compares every public result field.
6. Decode cross verification reconstructs base immediates independently and
   checks the existing `MicroOp` interface; it does not reuse Decode internals.

The four FP compressed memory classes are table-classified `UNSUPPORTED` before
any instruction encoder is called. Manual constants include NOP, LUI positive
and negative forms, HINTs, EBREAK, JR/JALR, reserved zero-immediate cases, and
non-compressed parcels. Machine-readable counts are in
`exhaustive_metrics.csv` and `exhaustive_classification.csv`.
