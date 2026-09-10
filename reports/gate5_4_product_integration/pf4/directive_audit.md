# PF4 Directive Audit

No PF4 functional source diff adds `INLINE`, `UNROLL`, `DATAFLOW`, false
`DEPENDENCE`, or complete `ARRAY_PARTITION`. PF4 adds only interface/reset and
existing-policy LUTRAM storage bindings to its test-only full-core RTL top.

The canonical synthesis flags do not define `BOOM_HLS_ENABLE_CORE_PIPELINE`.
`CORE_CYCLE_PIPELINED=false`.

`PRODUCT_COMMIT_BIM_TRAINING_INTEGRATED=false`. Predictor writes in the PF4
full-core RTL flow come only from the fixture-only seed stream before fetch;
the testbench checks exact seeded 2-bit values after execution.
