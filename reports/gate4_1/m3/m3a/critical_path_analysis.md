# Divider Critical Path Analysis

- Top: `synth_divider_top` only.
- Tool/part: Vitis HLS 2021.2, `xczu7ev-ffvc1156-2-e`.
- Target clock: 10.00 ns; estimated period: 5.734 ns; estimated Fmax: 174.41 MHz.
- Pipeline type: `no`; baseline `config_compile -pipeline_loops 0` was retained.
- Reported top scheduling latency: 1-2 cycles per wrapper invocation. This is not arithmetic request latency because persistent state advances once per invocation.
- Iteration path: shift one dividend bit, form shifted remainder, 64-bit compare, conditional 64-bit subtract, shift/append quotient bit, increment 8-bit iteration.
- The expression report identifies the 64-bit compare and subtract path and no multiply, divide, or remainder functional unit.
- Resource result: 3429 LUT, 344 FF, 0 BRAM, 0 DSP.
- No `DATAFLOW`, false-dependence, loop pipeline, or `CORE_CYCLE` directive was used.
- Full-core timing is intentionally not measured in M3A.
