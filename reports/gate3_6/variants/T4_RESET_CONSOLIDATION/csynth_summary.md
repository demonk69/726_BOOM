# T4_RESET_CONSOLIDATION Csynth Summary

| Top | Status | Period | LUT | FF | BRAM_18K | DSP | Runtime | Partitions |
|---|---|---:|---:|---:|---:|---:|---:|---:|
| `boom_core_top` | PASS | 5.898 ns | 45602 | 12119 | 12 | 3 | 61.29 s | 342 |

Verdict: `REJECTED_RESET_SEMANTICS`.

Removing only `#pragma HLS RESET variable=state` reduces full-core LUT by 37684, FF by 4492, and BRAM by 4. The same product top then closely matches the direct diagnostic top and receives 342 automatic partitions.

This is direct attribution evidence, not an acceptable implementation. Native C++ and HLS csim traces remain identical because csim does not exercise the synthesized hardware reset network. The product contract requires resettable persistent state, so the reset pragma is restored.
