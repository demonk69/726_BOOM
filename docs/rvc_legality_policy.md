# RVC Legality Policy

Gate 5.2 R1 targets the integer functionality implemented by the current core,
not every instruction permitted by the configured ISA string.

`decompress_rvc()` returns `RvcDecodeResult` with these rules:

| Input class | `valid` | `legal` | `instruction` | `length_bytes` |
|---|---:|---:|---:|---:|
| Supported integer RVC or C.EBREAK | true | true | canonical 32-bit instruction | 2 |
| Reserved compressed encoding | true | false | 0 | 2 |
| C.FLD/C.FSD/C.FLDSP/C.FSDSP | true | false | 0 | 2 |
| Bits `[1:0]=11` | false | false | 0 | 0 |

`compressed` always preserves the original 16-bit input. The four compressed
double-precision memory families are classified `UNSUPPORTED`, because the
current core has no FPU execution path. They must not be converted into an
instruction that Decode would reject later.

HINT encodings are legal. Reserved encodings are never converted to NOP.
C.EBREAK is a legal decompressor result, although current `decode_module` does
not implement architectural EBREAK behavior; this mismatch is explicit in the
instruction matrix and Decode cross-test output.
