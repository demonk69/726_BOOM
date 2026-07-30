# R1 Synthesis Summary

- Requested local II: 1
- Achieved local II: 1
- `RESET_ROB_INIT` latency: 32 cycles
- Local helper latency/interval: 34/34 cycles
- Estimated top period: 5.898 ns
- LUT: 48019, delta +20 (+0.04%)
- FF: 12191, delta +57 (+0.47%)
- BRAM_18K: 12, unchanged
- DSP: 3, unchanged
- `CORE_CYCLE`: not pipelined

The normal critical path remains `lsu_module/load_value`; reset logic is not the timing limiter.
