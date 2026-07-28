# T3_FUNCTION_BOUNDARIES Csynth Summary

| Top | Status | Period | LUT | FF | BRAM_18K | DSP | Runtime | Partitions |
|---|---|---:|---:|---:|---:|---:|---:|---:|
| `synth_core_step_top` | PASS | 5.898 ns | 45350 | 12111 | 12 | 3 | 58.82 s | 342 |
| `boom_core_ncycle_n1_top` | PASS | 5.898 ns | 87412 | 22182 | 16 | 3 | 67.97 s | 0 |
| `boom_core_ncycle_n2_top` | PASS | 5.898 ns | 87441 | 22123 | 16 | 3 | 67.97 s | 0 |
| `boom_core_ncycle_n4_top` | PASS | 5.898 ns | 87443 | 22125 | 16 | 3 | 68.06 s | 0 |
| `boom_core_ncycle_n8_top` | PASS | 5.898 ns | 87445 | 22127 | 16 | 3 | 68.29 s | 0 |
| `boom_core_top` | PASS | 5.898 ns | 87388 | 22117 | 16 | 3 | 66.25 s | 0 |

Verdict: `REJECTED_PPA`.

The directive successfully inlined `boom_core_cycle_io`; no cycle-wrapper report or RTL module remained. It did not trigger the direct diagnostic top's automatic scalarization: automatic partition count remained zero in the product and N-cycle tops, helper LUT remained 44302, mux LUT remained 37639, and direct state-memory LUT increased to 5011. Full-core LUT increased by 4102 and FF increased by 5506. N1 through N8 remain flat at 87412-87445 LUT, so neither fixed trip count nor the free-running loop accounts for the gap.
