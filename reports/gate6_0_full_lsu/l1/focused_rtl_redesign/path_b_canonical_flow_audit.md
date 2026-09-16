# PATH_B Canonical Synthesis Flow Audit

```text
PATH_B_MATCHES_ACCEPTED_CANONICAL_SYNTH_FLOW_BEFORE_FIX=false
PATH_B_MATCHES_ACCEPTED_CANONICAL_SYNTH_FLOW_AFTER_FIX=true
PATH_B_HLS_TESTBENCH_SOURCES_INCLUDED=false
CURRENT_SOURCE_BOOM_CORE_TOP_PREVIOUSLY_SYNTHESIZED=true
VALIDATION_INFRASTRUCTURE_FIX=true
```

The first PATH_B invocation used the correct top, merged source hash, device,
clock, storage defines, and effective default 8/8 queue depths, but omitted the
accepted canonical `config_compile -pipeline_loops 0` directive. It also added
redundant explicit default-depth defines and used noncanonical project/solution
names. The log consequently shows automatic pipelining of `try_issue_load`
before the 8 GiB workspace hard stop.

The bounded retry uses:

- top `boom_core_top`;
- only `src/boom_core_merged.cpp` as the HLS design source;
- byte-frozen merged hash `76d78e9052344a0b1e06c0e723c473d37e0b2dd7679a2ab3b8fbbeed9f1dff9c`;
- canonical default queue depths from `include/boom_config.hpp`;
- canonical LUTRAM flags, part, 10 ns clock, project name, solution name, and
  baseline directive;
- no SV testbench, host oracle, catalog, reference model, PATH_A wrapper, or
  diagnostic source in the HLS source set.

Durable accepted history is
`canonical_default_8_8/module_csynth_summary.csv`: PASS, 526.23 s,
6,412,852 KiB peak workspace, with completed Verilog generation.
