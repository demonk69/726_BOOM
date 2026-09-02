# P2 Critical Path Analysis

The selected 256-entry LUTRAM standalone wrapper has a 2.989 ns estimated period, versus 4.109 ns for 256-entry AUTO and BRAM. LUTRAM costs 26 additional LUT and 16 additional FF relative to AUTO while eliminating one BRAM and reducing the HLS estimated period by 1.120 ns.

The depth sweep is nearly flat in LUT/FF and changes AUTO inference from distributed logic at 64 entries to one BRAM at 128 entries and above. This inference discontinuity, rather than raw counter-bit growth, dominates the reported period difference. P2 therefore selects explicit LUTRAM for deterministic small-table storage and the lowest measured standalone period.

All predictor tops are unpipelined. The HLS wrapper's best-case latency 3 and minimum II 4 are scheduling properties of `ap_ctrl_hs`; they do not redefine the logical request-on-N/response-on-N+1 API. Reset paths have depth-dependent loop latency. Values are pre-route standalone HLS estimates, not a full-core critical-path or PPA claim.
