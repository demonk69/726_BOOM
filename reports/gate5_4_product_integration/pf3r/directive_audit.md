# PF3R Directive Audit

PF3R does not add a `CORE_CYCLE` pipeline, `INLINE`, `UNROLL`, `DATAFLOW`,
false `DEPENDENCE`, or complete `ARRAY_PARTITION` directive. The only new HLS
directive is the targeted storage binding below:

```cpp
#pragma HLS bind_storage variable=state.rob.ftq_generations type=RAM_2P impl=LUTRAM
```

This binding applies only to the 32 x 32-bit ROB FTQ-generation sidecar. The
canonical synthesis report maps it to
`state_rob_ftq_generations_RAM_2P_LUTRAM_1R1W` with zero BRAM.

`CORE_CYCLE_PIPELINED=false`.
