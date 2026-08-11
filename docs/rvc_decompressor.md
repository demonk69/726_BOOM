# RV64C Standalone Decompressor

`boom::decompress_rvc` accepts one 16-bit parcel and returns `RvcDecodeResult`.
It implements the integer RV64C encodings used by the current core.

- Reserved and current-core-unsupported encodings return
  `{valid=true, instruction=0, legal=false, length_bytes=2}`.
- C.FLD/C.FSD/C.FLDSP/C.FSDSP are unsupported because no FPU path exists.
- HINT encodings are legal and expand to their architecturally inert base
  instruction. This includes `C.LUI rd=x0, imm!=0`.
- Inputs with bits `[1:0]=11` return `valid=false`, `legal=false`, and length 0.
- `compressed` preserves the original input for every result.
- The decompressor is stateless. PC advancement, `IALIGN=16`, JALR link-PC
  handling, parcel assembly, and exception creation are not part of this API.

Gate 5.2 R1 deliberately does not connect this block to Frontend or Decode.
That integration requires separate mixed-stream and cross-word verification.
