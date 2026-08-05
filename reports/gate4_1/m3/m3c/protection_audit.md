# M3C Protection And Guardrail Audit

- Frozen expected artifacts under `reference/`: unchanged from Git after M3C execution.
- `src/boom_all.cpp`: excluded from source manifests, compilation, RTL generation, synthesis, and acceptance.
- Product synthesis tops: the canonical wrapper `synth_core_step_top` and product `boom_core_top`; raw `boom_core_step` is not a product top.
- `CORE_CYCLE`: unpipelined; all eight canonical XML reports state `PipelineType=no`.
- Forbidden directives: no `DATAFLOW`, false-dependence, or complete-array-partition directive was introduced in canonical source.
- Frozen topology: 3 completion sources, 2 integer PRF writes, 3 wakeups, 3 bypasses, and 3 ROB completes.
- Scope exclusions: no Frontend implementation, Full LSU, cache, MMU, FPU, or TileLink work was started.
- Final readiness: `READY_FOR_FRONTEND_IMPLEMENTATION=false`, `READY_FOR_FULL_LSU_IMPLEMENTATION=false`, and `READY_FOR_OFFICIAL_GATE_3=false`.
