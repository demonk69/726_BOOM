# Gate 4.1 M-extension Gap Analysis

M1 closes exact decode and INT-compatible issue classification for all 13 RV64M operations. It does not close arithmetic execution.

## Closed In M1

- Exact OP and OP-32 M-extension matching requires `funct7=0x01`.
- All legal operations carry the existing uop, FU, integer-IQ, register, width, and destination metadata.
- Reserved OP-32 high-multiply encodings and invalid base-ALU `funct7` combinations trap as illegal instruction cause 2.
- `FU_MUL` uops 16-20 and `FU_DIV` uops 21-28 classify only to the fixed INT-compatible lane.
- Existing non-M traces and W4 completion/writeback behavior are preserved.

## Remaining After M1

- M2 must replace the incomplete legacy MUL behavior and implement MUL/MULH/MULHSU/MULHU/MULW semantics.
- M3-M5 must add a persistent iterative divider and integrate DIV/REM and word variants.
- No M instruction beyond the legacy narrow MUL placeholder is currently architecturally executable.
- Full RV64M generated-RTL and architectural-program verification remains M6 work.
