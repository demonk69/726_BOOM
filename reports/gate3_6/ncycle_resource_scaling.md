# Gate 3.6 N-cycle Resource Scaling

| Top | Status | LUT | FF | BRAM | DSP | Period ns | Partitions | Runtime s |
|---|---|---:|---:|---:|---:|---:|---:|---:|
| boom_core_ncycle_n1_top | PASS | 83353 | 16808 | 16 | 3 | 5.898 | 0 | 72.82 |
| boom_core_ncycle_n2_top | PASS | 83379 | 16810 | 16 | 3 | 5.898 | 0 | 69.75 |
| boom_core_ncycle_n4_top | PASS | 83381 | 16811 | 16 | 3 | 5.898 | 0 | 70.10 |
| boom_core_ncycle_n8_top | PASS | 83383 | 16812 | 16 | 3 | 5.898 | 0 | 69.75 |
| boom_core_top | PASS | 83286 | 16611 | 16 | 3 | 5.898 | 0 | 69.52 |

No N-cycle top is a product replacement. The experiment isolates fixed loop trip count and retained wrapper effects with pipeline and unroll disabled.

## Interpretation

- N1 exactly matches the existing one-call product-interface `boom_core_step_top`: 83353 LUT, 16808 FF, 16 BRAM, 3 DSP.
- N2 through N8 add only 26 to 30 LUT over N1. The `boom_core_cycle_io` instance remains single and unchanged at 82876 LUT.
- The while-top is 67 LUT smaller than N1. The free-running loop therefore does not add the 37936-LUT gap.
- N2/N4/N8 reports contain a finite loop with trip counts 2/4/8 and `Pipelined=no`.
- No unroll transformation was reported, and resources do not scale with N.
- All five designs report zero automatic partition operations. The retained cycle-wrapper storage form is stable across fixed and infinite loops.
