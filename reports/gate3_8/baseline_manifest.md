# Gate 3.8 Baseline Manifest

Date: 2026-07-28

Baseline commit: `4ebc0a309d614de095aa9dfd9e3c18adf6b2c4bc Gate 3.7: characterize CORE_CYCLE pipeline timeout`.

Accepted configuration: Gate 3.3 conservative `boom_core_top`, preserved through Gate 3.7.

| Metric | Value |
|---|---:|
| LUT | 83286 |
| FF | 16611 |
| BRAM_18K | 16 |
| DSP | 3 |
| Estimated period | 5.898 ns |
| `CORE_CYCLE` pipeline | disabled |
| Whole-state source reset directive | retained |

| Artifact | Path |
|---|---|
| Frozen source hashes | `reports/gate3_8/source_hashes_before.txt` |
| Accepted csynth report | `reports/gate3_8/baseline_artifacts/boom_core_top_csynth.rpt` |
| Accepted generated RTL | `reports/gate3_8/baseline_artifacts/conservative_rtl/` |
| Accepted HLS traces | `reports/gate3_8/baseline_artifacts/accepted_hls_traces/` |
| BOOM reference traces | `reports/gate3_8/baseline_artifacts/boom_reference_traces/` |
| Full-program architectural diff | `reports/gate3_8/baseline_artifacts/full_program_architectural_diff.md` |
| Partial-order result | `reports/gate3_8/baseline_artifacts/partial_order.log` |
| Generated RTL port inventory | `reports/gate3_8/rtl_port_inventory.csv` |

Gate 3.8 adds verification infrastructure and two trace programs. It does not change accepted core source, directives, configuration, capacity, interface semantics, or pipeline settings. The copied baseline RTL is the design under test; `fixed_artifacts/conservative_rtl/` is the independently regenerated RTL from the unchanged accepted source.
