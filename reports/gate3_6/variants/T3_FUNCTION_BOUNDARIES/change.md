# T3_FUNCTION_BOUNDARIES Change

Evidence-backed single-variable experiment: force-inline `boom_core_cycle_io` into the selected top.

| Item | Value |
|---|---|
| Changed source | `src/boom_core_top.cpp` |
| Changed function | `boom_core_cycle_io` |
| Directive | `#pragma HLS INLINE` |
| Reason | Product-interface N1/N2/N4/N8/while tops retain one 82K-LUT cycle wrapper with zero automatic partitions; direct diagnostic top has no retained wrapper and is 45K LUT |
| Architecture | unchanged |
| State capacity | unchanged |
| Interface | unchanged |
| CORE_CYCLE pipeline | disabled |
| Unroll/dataflow/partition directives | none added |

The experiment does not inline all helpers. It targets only the report-localized retained boundary.
