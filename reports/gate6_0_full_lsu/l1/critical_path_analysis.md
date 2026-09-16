# L1 Critical Path Analysis

Default estimated period remains 6.341 ns, 0.141 ns above the 6.200 ns T0R target and below the 6.500 ns L1 limit. The accepted T0 critical path remains the multiplier path rather than an LSU queue scan. No LQxSQ CAM was introduced.

Depth scaling is non-monotonic: 4/4 has 202974 LUT, 8/8 has 201186, and 16/16 has 201812. The small spread and identical period indicate HLS constant folding/mux mapping rather than quadratic address comparison growth. This is not claimed as linear physical scaling and should be revisited in T0R/PPA review.
