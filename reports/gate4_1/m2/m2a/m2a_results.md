# Gate 4.1 M2A Results

Status: **M2A_STANDALONE_MUL_ARITHMETIC_VERIFIED**

`READY_FOR_M2B_INT_INTEGRATION=true`

M2A verifies standalone arithmetic only. It does not claim full-core multiplication, INT-lane integration, W4 completion integration, or `M2_MUL_FAMILY_VERIFIED`.

## Required Answers

1. **ap_int with ordinary g++:** default and canonical `-Iinclude` builds cannot locate `ap_int.h`; adding `-I/home/lab_726/Xilinx/Vitis_HLS/2021.2/include` compiles and runs the 128-bit probe.
2. **Public interface:** `mul.hpp` uses standard `uint64_t`, avoiding a new dependency for existing regressions.
3. **MUL:** full unsigned 64x64 product, low 64 bits returned.
4. **MULH:** signed operands are widened to `ap_int<128>` and product bits 127:64 are returned.
5. **MULHSU:** unsigned 128-bit product high half, subtracting unsigned rhs when signed lhs is negative.
6. **MULHU:** zero-extended `ap_uint<128>` product bits 127:64.
7. **MULW:** low 32-bit operands, low 32-bit product, sign-extended to 64 bits.
8. **Directed result:** 51/51 PASS, including invalid-request behavior.
9. **Random result:** 256 seeds x 512 vectors = 131072 vectors; 40960 edge/pattern vectors, 90112 random vectors, zero mismatches.
10. **synth_mul_top resources:** 612 LUT, 7 FF, 0 BRAM, 33 DSP, latency 1.
11. **DSP count:** 33 total: 15 unsigned 64x64, 15 signed 64x64, and 3 for 32x32.
12. **Estimated period:** 6.463 ns; `PipelineType=no`.
13. **execute.cpp unchanged:** Yes, its SHA-256 remains `70a5682b8c97318c3bfefd63ebd9ad3053544cfc74c57edee178d58edf552649`.
14. **Divider started:** No; no divider files or DIV/REM implementation exist.
15. **M2A status:** `M2A_STANDALONE_MUL_ARITHMETIC_VERIFIED`.
16. **M2B readiness:** `READY_FOR_M2B_INT_INTEGRATION=true`.
