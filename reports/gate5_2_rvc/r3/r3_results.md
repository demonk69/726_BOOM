# Gate 5.2 R3 Decode Gap Closure Results

## Status

- `GATE5_2_R3_DECODE_GAPS_VERIFIED=true`
- `GATE5_2_RVC_VERIFIED=true`
- `GATE5_2_RVC_PROTECTED_DECODE_GAPS=0`

R3 closes all 288 R2 protected legal RV64C forms without changing the frozen
RVC decompressor or mixed-width Frontend sequencing.

## Closure

- `C.EBREAK`: 1 encoding now decodes as `UOPC_EBREAK` and raises breakpoint
  exception cause 3.
- `C.SRLI shamt[5]=1`: all 256 encodings use RV64 `funct6` shift-immediate
  classification and execute with the complete six-bit shift amount.
- `C.JALR`: all 31 legal register forms retain `is_rvc`; Execute writes `PC+2`
  as the link while ordinary JAL/JALR continue to write `PC+4`.
- Exact per-encoding inventory: `gap_inventory.csv`, 288 unique rows.

## Verification

- Independent decompressor directed/exhaustive: 228/228 and 65,536/65,536 PASS.
- Decode cross-check: 38,551/38,551 legal current-core expansions PASS; no skips.
- Focused native Frontend/Decode: 4,431 assertions, 414 cases, all 288 former
  gaps checked, zero failures.
- Persistent random: 256 seeds x 2,048 cycles, zero errors.
- JALR Execute link: compressed `PC+2` and base `PC+4` PASS.
- Focused generated RTL: 58/58 PASS.
- Mixed full-core programs: native 11/11, Vitis csim 11/11, generated RTL
  11/11 PASS. The R3 program checks `C.SRLI 32` and the `C.JALR` link value.
- Gate 5.1 focused generated-RTL preservation: 33/33 PASS.
- Gate 3.9 full-core generated-RTL preservation: 49/49 PASS.
- RV64M M3C directed/random/full-core preservation: PASS.
- W3 canonical software preservation: 400/400 PASS.
- W4E directed/random/software preservation: PASS.

## PPA

Canonical `boom_core_top`: 126,903 LUT, 27,904 FF, 16 BRAM_18K, 3 DSP,
6.341 ns. `CORE_CYCLE` remains `Pipelined=no`.

Relative to R2: +105 LUT, -588 FF, unchanged BRAM, DSP, and estimated period.
The 6.5 ns acceptance target closes and R3 creates no PPA blocker.
