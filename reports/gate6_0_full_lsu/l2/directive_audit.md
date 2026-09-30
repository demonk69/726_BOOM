# Gate 6.0 L2 Directive Audit

- No `DATAFLOW` directive was added.
- No `DEPENDENCE` override was added.
- No complete `ARRAY_PARTITION` was added.
- Two targeted `ARRAY_PARTITION cyclic factor=2 dim=1` directives apply only
  to `state.lsu.stq_address` and `state.lsu.stq_data` when `SQ_DEPTH >= 16`.
- No loop-wide `UNROLL` was added.
- No core-cycle pipeline directive was added.
- Canonical synthesis retains `config_compile -pipeline_loops 0`.
- Existing FTQ and predictor LUTRAM controls remain enabled.
- L2 forwarding is implemented as ordinary sequential C++ control/data logic.
- No forced SQ RAM core or implementation binding is present.
