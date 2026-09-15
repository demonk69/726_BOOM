# Timing Headroom Assessment

Current margin to 6.500 ns is only 0.159 ns (2.45%). L1 state-only parameterization does not feed the current INT operand-to-multiply cone, so it may proceed before T0R. L2 adds associative store search and load selection near MEM issue and must not start without repaired headroom.

`RECOMMENDED_PRE_LSU_PERIOD_TARGET_NS=6.200`

`RECOMMENDED_PRE_LSU_TIMING_MARGIN_NS=0.300`

The 0.300 ns margin is 4.6% of the 6.5 ns limit and nearly doubles current slack while avoiding an unsupported pipeline change. It is a synthesis acceptance target, not a hardcoded implementation constraint.

Safe T0R experiments, all `NO_FUNCTIONAL_SEMANTIC_CHANGE`:

- E0: unchanged baseline; record 213436/47337/16/3/6.341.
- E1: Execute consumes Issue-resolved accepted-grant operand data while preserving same-cycle bypass/conflict semantics; measure LUT/FF/BRAM/DSP/period and full equivalence.
- E2: sweep only physical mapping of the two PRF banks without changing read latency or dual-write protocol.
- E3: source-equivalent multiplier mapping/partial-product experiment preserving the accepted 64x64 basis, separate MULW expression, one shared instance, and zero added latency.

Previously measured MULW reuse, explicit DSP/fabric binding, extra signed products, latency changes, pipeline, and new scheduling directives are excluded.

`REQUIRE_T0R_BEFORE_L1=false`

`T0R_REQUIRED_BEFORE_L2=true`
