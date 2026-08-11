# Gate 5.2 R1 RVC Decompressor Results

## Status

- `GATE5_2_R1_RVC_DECOMPRESSOR_VERIFIED=true`
- `READY_FOR_GATE5_2_R2_RVC_FETCH=true`

No broader Gate 5.2 or Fetch Buffer readiness is asserted.

## Required Results

1. Canonical API: `RvcDecodeResult decompress_rvc(uint16_t)` with `valid`,
   `legal`, `instruction`, `compressed`, and `length_bytes`.
2. Non-RVC parcels: `valid=false`, `legal=false`, instruction 0, length 0.
3. Compressed parcels: original input retained and length 2, including reserved
   and unsupported encodings.
4. FP compressed memory classes: 8,192 encodings classified unsupported,
   `legal=false`, instruction 0, because the current core has no FPU.
5. Directed: 228/228 PASS, exceeding the required 150 checks.
6. Exhaustive: 65,536/65,536 exact PASS with 38,551 supported, 8,192
   unsupported, 2,409 reserved, and 16,384 non-RVC. The legal set contains
   394 architectural HINT encodings; each quadrant contains 16,384 inputs.
7. Decode cross: 38,294 current-core-supported expansions PASS with exact uopc,
   source/destination, immediate, branch, load/store, and exception checks.
8. Known protected Decode gaps: C.EBREAK is legal at decompression but current
   Decode lacks architectural EBREAK; 256 C.SRLI shamt[5]=1 forms are also
   excluded from cross acceptance and recorded in the instruction matrix.
9. Merged inclusion/compile: PASS; `src/boom_all.cpp` excluded.
10. `synth_rvc_top`: PASS, 1,022 LUT, 0 FF/BRAM/DSP, 1.845 ns, II 1,
    `Pipelined=no`.
11. `synth_frontend_top` preservation: PASS and exact Gate 5.1 match at 827 LUT,
    526 FF, 5.993 ns.
12. Repair-after regressions: Gate 5.1 focused PASS, W3 400/400 PASS, RV64M
    native full-core 15/15 PASS.
13. Protected source/directive/reference audit: PASS.

## ISA Coverage

- Supported integer instructions: C.ADDI4SPN, C.LW, C.LD, C.SW, C.SD,
  C.NOP, C.ADDI, C.ADDIW, C.LI, C.ADDI16SP, C.LUI, C.SRLI, C.SRAI,
  C.ANDI, C.SUB, C.XOR, C.OR, C.AND, C.SUBW, C.ADDW, C.J, C.BEQZ,
  C.BNEZ, C.SLLI, C.LWSP, C.LDSP, C.JR, C.MV, C.EBREAK, C.JALR,
  C.ADD, C.SWSP, and C.SDSP.
- HINTs remain legal. The 394 HINT encodings include applicable zero-register
  and zero-immediate forms of C.ADDI, C.LI, C.LUI, C.SRLI, C.SRAI, C.SLLI,
  C.MV, and C.ADD; C.NOP remains its named legal instruction.
- Reserved encodings include zero-immediate C.ADDI4SPN/C.ADDI16SP/C.LUI,
  C.ADDIW rd=x0, C.JR rs1=x0, C.LWSP/C.LDSP rd=x0, the reserved RV64 CA
  combinations, and the reserved Quadrant 0 class.
- C.FLD, C.FSD, C.FLDSP, and C.FSDSP are ISA encodings requiring D but are
  `UNSUPPORTED_BY_CURRENT_CORE`; they return `legal=false` rather than a
  floating-point base instruction.
- CI, CIW, CL, CS, CB, CJ, and CSS immediate permutations were independently
  reconstructed and exhaustively compared. This covers sign/zero extension,
  even branch/jump offsets, stack offsets, and x8-x15 prime-register mapping.

## R2 Boundary

R2 may implement RVC fetch sequencing. R1 does not modify Frontend or Decode
and does not implement PC+2, halfword cursor/carry, cross-word assembly, mixed
streams, redirect alignment, or Fetch Buffer behavior.
