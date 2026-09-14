# PF5P Current-Source Preservation Provenance

PF5P was preservation-only. No canonical product functional source changed.
All accepted prior-gate contracts required for PF5 closure pass against the
current PF5 source.

## Results

| Gate | Accepted contract | Result | Durable evidence |
| --- | --- | --- | --- |
| PF1-PF4 | Previously recorded current-source native, CSim, random, long-run, focused RTL, and full-core RTL scopes | PASS | `preservation_summary.csv` |
| PF2 | Current-compatible full-core mixed-RVC RTL | 11/11 PASS | `preservation/pf2_full_core/r2_full_core_rtl_matrix.csv` |
| W3 | Canonical software and focused generated RTL | 400/400 and 11/11 PASS | `preservation/w3/` |
| W4 | Exact downstream multi-writeback oracle | 13/13 PASS | `preservation/w4/logs/w4_multi_writeback_tests.log` |
| M3C | Directed, deterministic random, and native full-core programs | 1,458 checks, 256 x 2,048 cycles, and 15/15 PASS | `preservation/m3c/logs/` |
| R2 | Current-compatible native, Vitis CSim, and full-core RTL | 11/11 each PASS | `preservation/r2/logs/` and PF2 full-core matrix |
| B3I | Packet-aware random, native programs, and fresh full-core RTL | 256 x 4,096, 6/6, and 6/6 PASS | `preservation/b3i/` |

PF2's historical `SHADOW_ONLY` conditional-steering condition was legitimately
advanced by PF4/PF5 and is classified as
`EXPECTED_LATER_GATE_ARCHITECTURAL_ADVANCE`. The older R2 focused harness has a
lifecycle mismatch with the current product and remains
`NON_VERDICT_DIAGNOSTIC_VERSION_MISMATCH`; it was not substituted for the
current-compatible R2 contracts.

B3I used `B3I_PACKET_AWARE_ACCEPTED` semantics. Its generated `boom_core_top`
RTL is keyed to aggregate modular source/header hash
`0e21d79d9d170e44397c3b7e1c5317acf417fe0188735098f5dba5e3c4a0be44` under
`/tmp/boom_hls/pf5_preservation/b3i/rtl/0e21d79d9d170e44/`.

## Freeze

- Canonical merged source SHA-256: `b870fa9e9d31d3f6cd0113a697ef22229661d0aa393f43a9cf5a9fff6345e6bd`.
- PF5 source manifest SHA-256: `610604a0c472508b511a000769128edbb347a841f266cdaea28a71efafcba9ee`.
- Canonical synthesis summary SHA-256: `bc6d188f70ad70435112924ad25dc1c41134399ab2072e790464bcec496f67ce`.
- Historical `src/boom_all.cpp` SHA-256: `d6f885632ddd445729adda8148ea256e67683ccc8e7f2b10c9951e915d92c76c`.
- Frozen PPA reused by unchanged source hash: 213436 LUT, 47337 FF, 16 BRAM,
  3 DSP, 6.341 ns; PF4 LUT delta 1.1957%.

`PF5P_PRODUCT_BUG_FOUND=false`

`PRODUCT_SOURCE_CHANGED=false`

`PF5P_PPA_REUSED_BY_SOURCE_HASH=true`
