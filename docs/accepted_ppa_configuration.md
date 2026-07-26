# Accepted PPA Configuration

Accepted baseline: Gate 3.3 conservative `boom_core_top`.

| Setting | Value |
|---|---|
| Tool | Vitis HLS 2021.2 |
| Top | `boom_core_top` |
| `CORE_CYCLE` pipeline | Disabled by default |
| Estimated period | 5.898 ns |
| LUT | 83286 |
| FF | 16611 |
| BRAM_18K | 16 |
| DSP | 3 |

Gate 3.4 did not accept a new PPA optimization because no single-variable optimization candidate was applied and no LUT reduction was demonstrated. Gate 3.3 remains the accepted configuration.
