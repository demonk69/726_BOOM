# Gate 5.2 R1 Critical Path Analysis

`synth_rvc_top` is combinational with latency 0, II 1, and `Pipelined=no`.
Vitis HLS 2021.2 reports 1,022 LUT, 0 FF, 0 BRAM, 0 DSP, and a 1.845 ns
estimated period against the 10 ns target.

Local `INLINE` directives on private bit-slice and base-encoder helpers allow
constant propagation. No global directive or product module is changed.

The preservation `synth_frontend_top` reports 827 LUT, 526 FF, 0 BRAM/DSP,
and 5.993 ns, exactly matching the accepted Gate 5.1 standalone Frontend
baseline. The decompressor is not instantiated in Frontend, so these numbers do
not predict R2 integrated PPA.
