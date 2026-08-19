# Fetch Buffer Parameterization

Gate 5.3 freezes `FETCH_BUFFER_DEPTH=8`, `FETCH_BUFFER_STORAGE=AUTO`, `FETCH_BUFFER_RESET_POLICY=CONTROL_ONLY`, and `FETCH_PACKET_WIDTH=2`.

## Depth Sweep

| Depth / policy | LUT | FF | BRAM_18K | Period | Latency |
|---|---:|---:|---:|---:|---:|
| 2 AUTO | 3204 | 399 | 0 | 7.162 ns | 0..0 |
| 4 AUTO | 16957 | 2782 | 0 | 5.834 ns | 1..1 |
| 8 AUTO | 1179 | 846 | 0 | 4.574 ns | 1..4 |
| 16 AUTO | 1181 | 597 | 4 | 5.134 ns | 1..4 |

Depth 8 matches the target topology, has the best measured period, and avoids the depth-16 BRAM cost. HLS estimates are non-monotonic and are accepted as reported rather than extrapolated.

## Storage At Depth 8

| Storage | LUT | FF | BRAM_18K | Period |
|---|---:|---:|---:|---:|
| AUTO | 1179 | 846 | 0 | 4.574 ns |
| LUTRAM | 1179 | 652 | 0 | 4.574 ns |
| BRAM | 1153 | 458 | 8 | 5.134 ns |

AUTO is retained because it avoids a target-specific binding while meeting timing without BRAM. The standalone FF difference does not justify freezing an implementation primitive.

## Reset At Depth 8

| Reset | LUT | FF | BRAM_18K | Period | Latency |
|---|---:|---:|---:|---:|---:|
| CONTROL_ONLY | 1179 | 846 | 0 | 4.574 ns | 1..4 |
| PAYLOAD_RESET | 1362 | 865 | 0 | 4.574 ns | 2..11 |

CONTROL_ONLY is canonical because head/tail/count fully determine validity. Resetting unreachable payload adds area and latency without architectural value.

## Packet Width PPA

B2 width 1 to B3I width 2 changes full-core LUT from 129885 to 135953 (`+6068`, `+4.672%`) and FF from 29194 to 33373 (`+4179`, `+14.315%`). BRAM remains 16, DSP remains 3, and estimated period remains 6.341 ns. The cost is accepted for natural two-parcel response unpacking and atomic burst admission, not for a one-wide backend speedup.

Machine-readable results are in `reports/gate5_3_fetch_buffer/final/parameter_ppa_summary.csv`.
