# Protected Decode Gaps

R2 does not claim architectural support for these legal RV64C forms:

- `C.EBREAK` (`0x9002`): decompression is legal, but base Decode does not implement architectural EBREAK.
- `C.SRLI shamt[5]=1`: all 256 legal compressed forms decompress correctly, but current base Decode confuses the RV64 shift-immediate encoding with SRAI.
- `C.JALR`: all 31 legal register forms decompress correctly, but the frozen execute backend writes a `pc+4` link rather than the required compressed `pc+2` link.

The R2 Frontend marks all three classes as explicit illegal-instruction exceptions (cause 2) before base Decode. Original compressed bits and instruction PC are retained. They cannot execute as another legal operation. Closure remains Gate 5.2 R3 scope.
