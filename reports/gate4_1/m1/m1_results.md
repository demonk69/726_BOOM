# Gate 4.1 M1 Results

Status: **M1_RV64M_DECODE_VERIFIED**

`READY_FOR_M2_MUL_FAMILY=true`

This status verifies decode and INT-compatible issue classification only. It does not claim correct RV64M arithmetic execution, and no divider was implemented.

## Required Answers

1. **All 13 M instructions decode:** Yes, 13/13 exact OP/OP-32 vectors pass.
2. **MUL-family FU mapping:** MUL, MULH, MULHSU, MULHU, and MULW map to `FU_MUL`.
3. **DIV/REM-family FU mapping:** DIV, DIVU, REM, REMU, DIVW, DIVUW, REMW, and REMUW map to `FU_DIV`.
4. **OP illegal near-misses:** All required OP near-misses decode as `UOPC_ILLEGAL`, exception cause 2, `DST_N`, and no MUL/DIV FU classification.
5. **OP-32 illegal near-misses:** Reserved high-word multiply encodings and invalid base `funct7` combinations are illegal with cause 2 and no destination side effect.
6. **`rd=x0`:** MUL, DIV, MULW, and REMUW remain legal decode operations with logical destination zero.
7. **Issue lane classification:** Existing uops 16-20 with `FU_MUL` and 21-28 with `FU_DIV` classify only to the fixed INT-compatible lane. Illegal and malformed combinations are unsupported. MEM lane and inactive FP lane behavior are unchanged.
8. **Directed tests:** 42/42 PASS: 13 legal M, four `rd=x0`, ten illegal near-miss, and fifteen base OP/OP-32 collision vectors.
9. **Old regressions:** W3 400/400, W4 directed 95/95, reset 14/14, and W4 random 128/128 seeds PASS.
10. **Trace results:** C++/Vitis csim 7/7 pairs and 21/21 normalized checks PASS; all 14 generated trace files are byte-identical to frozen W4; full-program diff is 10/10 and partial order is 7/7.
11. **Csynth results:** Vitis HLS 2021.2 conservative csynth passes 3/3 for `synth_decode_top`, `synth_core_step_top`, and `boom_core_top`. Every report has `PipelineType=no`; `CORE_CYCLE` has no pipeline II.
12. **Resource changes:** `synth_decode_top` is 275 LUT/173 FF. Relative to W4, `synth_core_step_top` changes by +2038 LUT/+625 FF and `boom_core_top` by +2219 LUT/+625 FF. BRAM and DSP are unchanged. Product period remains 6.025 ns.
13. **M1 final status:** `M1_RV64M_DECODE_VERIFIED`.
14. **M2 readiness:** `READY_FOR_M2_MUL_FAMILY=true`.

## Scope Guards

- `src/execute.cpp`, `src/completion.cpp`, `src/rob.cpp`, and `src/lsu.cpp` are byte-identical to the W4 baseline.
- `src/boom_all.cpp` remains a pre-existing dirty legacy file and was neither read nor modified for M1.
- Existing trace expectations were not changed.
- `CORE_CYCLE` remains unpipelined.
- M2 multiply arithmetic and M3-M5 divider work have not started.
