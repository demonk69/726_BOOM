# Independent Reference Method

Native tests use evidence organized differently from the DUT:

1. Directed and structured tests build B/J/I instructions with field encoders and provide independently selected signed offsets, PCs, registers, and expected metadata.
2. The random oracle reconstructs B/J immediates directly from architectural bit positions, sign extends independently, and classifies legal opcodes/funct3 without calling `predecode_cfi` for expected values.
3. The RV64C exhaustive oracle classifies compressed CFI forms directly with flat compressed masks: C.J, C.BEQZ, C.BNEZ, and CR-format C.JR/C.JALR. The already verified `decompress_rvc` supplies the canonical instruction to the DUT. This catches non-CFI expansion false positives without duplicating the decompressor.
4. Current Decode is a secondary cross-check for 6,206 legal compressed CFI expansions. It checks branch/JAL/JALR kind, original PC, and `is_rvc`; it is not the expected-value source.

No expected result calls the DUT, copies its control tree, or changes expected values from observed DUT output.
