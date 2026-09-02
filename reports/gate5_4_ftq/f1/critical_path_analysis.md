# Critical Path Analysis

The selected 211-bit depth-32 LUTRAM scalar wrapper estimates 3.788 ns and remains
unpipelined. The top is a transactional HLS diagnostic wrapper: latency varies
from 7 to 46 cycles and II from 8 to 47 because fixed-bound reset/redirect
scans are serialized. These physical wrapper cycles are not a product FTQ
integration claim. No `PIPELINE`, `DATAFLOW`, false `DEPENDENCE`, or complete
`ARRAY_PARTITION` directive was introduced; only storage-binding experiments
are present.
