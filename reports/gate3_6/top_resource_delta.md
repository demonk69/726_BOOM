# Gate 3.6 Top Resource Delta

Direct HLS estimate: `boom_core_top` 83286 LUT minus `synth_core_step_top` 45350 LUT = **37936 LUT**.

## Recursive Category Delta

| Category | Step LUT | Core LUT | Delta |
|---|---:|---:|---:|
| Expression | 85 | 103 | 18 |
| FIFO | 335 | 335 | 0 |
| Instance | 34537 | 44302 | 9765 |
| Memory | 258 | 730 | 472 |
| Multiplexer | 10135 | 37816 | 27681 |
| Register | 0 | 0 | 0 |

The product-side categories recursively expand the retained `boom_core_cycle_io` instance and add the small outer shell, so the category deltas sum exactly to 37936 LUT.

The free-running loop is not the primary source: the one-call `boom_core_step_top` product-interface control is 83353 LUT, only 67 LUT above the 83286-LUT free-running top.

The retained `boom_core_cycle_io` hierarchy identifies where the delta is reported, but T3 proves the boundary alone is not causal: force-inlining removes the wrapper and increases the top to 87388 LUT while partitions remain zero.

T4 isolates the dominant trigger. Removing only the product state's HLS reset pragma reduces `boom_core_top` from 83286 to 45602 LUT and changes automatic partition count from 0 to 342. The 37684-LUT same-top delta closes exactly as +9741 helper-instance LUT, +472 memory LUT, and +27471 multiplexer LUT in the resettable implementation. The no-reset product cycle wrapper is 45133 LUT, close to the 45350-LUT direct diagnostic top; the product outer shell remains 469 LUT in both cases.

The no-reset result is attribution evidence only and is rejected because required synthesized reset semantics are lost.

See `top_resource_delta.csv`, `ram_instance_diff.csv`, and `helper_instance_diff.csv` for machine-readable evidence.
