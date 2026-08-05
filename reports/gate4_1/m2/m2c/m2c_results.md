# Gate 4.1 M2C Results

## Status

- `M2_MUL_FAMILY_VERIFIED=true`
- `M2B_PPA_BLOCKER=false`
- `READY_FOR_M3_DIVIDER_CORE=true`
- `READY_FOR_OFFICIAL_GATE_3=false`

M2C accepts a single shared multiply instance without changing any of the five RV64M multiply results, latency, INT-lane behavior, or W4 completion topology. Divider work has not started.

## Accepted Configuration

- Keep one unsigned 64x64 product as the common datapath.
- Compute MULH high bits as unsigned-product high minus `rhs` when `lhs` is negative and minus `lhs` when `rhs` is negative.
- Keep the existing analogous single correction for MULHSU.
- Select high or low product bits by multiply uop.
- Keep the independent 32x32 MULW expression because reusing the 64x64 low bits failed the 6.5 ns full-core threshold.
- Apply `#pragma HLS ALLOCATION operation instances=mul limit=1` in `execute_mul`.
- HLS binds the accepted arithmetic to one `mul_65s_65s_128_1_1` unit using 3 DSP.

## Attribution And Experiments

The M2B 33 DSP total was exactly 15 DSP for unsigned 64x64, 15 DSP for signed 64x64, and 3 DSP for 32x32 MULW. The independent signed product was removed by algebraic sign correction. The allocation directive then merged the remaining mutually exclusive multiply operations into one physical unit.

| Experiment | Mul ns | Mul LUT | Mul DSP | Core ns | Core LUT | Core DSP | Decision |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| M2B baseline | 6.463 | 612 | 33 | 7.257 | 107681 | 33 | Reject PPA |
| Signed-high correction | 4.717 | 701 | 18 | - | - | - | Retain |
| MULW reuse of 64x64 | 6.488 | 681 | 15 | 6.765 | 107750 | 15 | Reject timing |
| BIND_OP DSP | 6.717 | 702 | 18 | - | - | - | Reject timing |
| BIND_OP fabric | 7.090 | 4915 | 3 | - | - | - | Reject timing/LUT |
| ALLOCATION mul=1 | 4.717 | 709 | 3 | 6.341 | 107783 | 3 | Accept |
| INLINE off | 6.463 | 659 | 18 | - | - | - | Reject DSP |

Each row changes one variable relative to the preceding retained structure or accepted structural baseline. Exact reports are under `experiments/`, `candidate15/`, `candidate18/`, and `candidate_alloc1/`; the final rerun is under `final/csynth/`.

## Functional And RTL Evidence

- Standalone multiply: 51/51 directed PASS and 131072 random vectors with zero mismatch.
- Integrated execute: 30/30 directed PASS.
- Persistent integrated execute: 131072 vectors, all five uopcs covered, 16384 held-result checks, zero mismatch.
- Native full-core multiply program: 10/10 architectural checks PASS.
- W3/W4 legacy software: 15/15 suites, 400/400 checks across 197 runs PASS.
- C++/csim normalized traces: 7/7 PASS; normalized architecture/event/cycle checks: 21/21 PASS; full-program architectural diff: 10/10 PASS.
- W4 persistent completion random: 128 seeds PASS with peaks unchanged at three completion sources, two PRF writes, three wakeups, and three bypasses.
- Focused generated `synth_execute_top` RTL: 10/10 PASS.
- Generated `boom_core_top` RTL: 2/2 scenarios PASS; both reach `tohost=1` and match ten architectural register values.

## Canonical Synthesis

All reports use Vitis HLS 2021.2, `xczu7ev-ffvc1156-2-e`, a 10 ns target, and `PipelineType=no`.

| Top | Period ns | LUT | FF | BRAM | DSP |
| --- | ---: | ---: | ---: | ---: | ---: |
| `synth_mul_top` | 4.717 | 709 | 264 | 0 | 3 |
| `synth_issue_top` | 5.019 | 17362 | 4821 | 0 | 0 |
| `synth_execute_top` | 5.430 | 2654 | 770 | 8 | 3 |
| `synth_completion_top` | 5.474 | 37959 | 10779 | 8 | 0 |
| `synth_rob_top` | 3.746 | 29566 | 8123 | 1 | 0 |
| `synth_lsu_top` | 3.474 | 863 | 244 | 0 | 0 |
| `synth_core_step_top` | 6.341 | 107783 | 25509 | 16 | 3 |
| `boom_core_top` | 6.341 | 114712 | 25916 | 16 | 3 |

Canonical synthesis is 8/8 PASS. Compared with M2B `synth_core_step_top`, estimated period improves from 7.257 ns to 6.341 ns and DSP falls from 33 to 3. Core LUT changes from 107681 to 107783, an increase of 102 LUT (0.095%), which is acceptable and far below the rejected fabric alternative.

Raw `boom_core_step` was intentionally not rerun and is not a canonical product target. Its prior aggregate-interface rejection remains a documented Vitis HLS 4096-bit tool limitation, not a functional failure.

## Decision

All five multiply operations pass arithmetic, integration, held-result, legacy W4, generated-RTL, full-core RTL, and canonical synthesis acceptance. Full-core estimated period is at or below 6.5 ns, DSP is reduced by 30, topology and latency are unchanged, and no forbidden optimization was used.

`M2_MUL_FAMILY_VERIFIED=true`

`READY_FOR_M3_DIVIDER_CORE=true`
