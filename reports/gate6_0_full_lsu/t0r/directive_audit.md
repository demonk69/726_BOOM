# Directive Audit

- Selected A5 adds no HLS directives.
- `CORE_CYCLE_PIPELINED=false`; no `PIPELINE boom_core_step` and no core `DATAFLOW`.
- New DATAFLOW directives: 0.
- New false DEPENDENCE directives: 0.
- New complete ARRAY_PARTITION directives: 0.
- B1 temporarily tested `bind_storage` of the two existing PRF replicas as 1P LUTRAM. Execute synthesis reduced PRF read delay from 1.24 ns to 0.67 ns, but full-core synthesis rejected the shared arrays as multiply allocated (`RAM_1P_LUTRAM, RAM`). All B1 directives were removed.
- Baseline canonical FTQ and predictor LUTRAM flags are unchanged and are provenance inputs, not T0R additions.
