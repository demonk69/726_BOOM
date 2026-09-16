# Fresh Baseline

- Commit: `90510c73b737bc0fe82af20c40fb8666919db27a`
- Branch: `gate3.8-rtl-verification`; ahead/behind `0/0`; index empty.
- Tool: Vitis HLS 2021.2 build 3367213, IP build 3369179.
- Part: `xczu7ev-ffvc1156-2-e`; requested clock: 10 ns.
- Top/source: `boom_core_top`, canonical `src/boom_core_merged.cpp`.
- Flags: `-std=c++11 -DBOOM_FTQ_STORAGE_LUTRAM -DBOOM_PREDICTOR_STORAGE_LUTRAM`.
- Default queues: LQ=8, SQ=8.
- Result: 201186 LUT, 45262 FF, 16 BRAM, 3 DSP, 6.341 ns.
- Full-core runtime: 510.82 s; peak RSS: 6409908 KiB; workspace: 672 MiB.
- Execute quick-screen reference: 6825 LUT, 916 FF, 8 BRAM, 3 DSP, 6.411 ns.
- Baseline product aggregate SHA-256: `b3cd4bdb13dc00e4b0293bc506268005bb8633b0e0036fc5e18f39314bbab618`.
- Baseline merged SHA-256: `76d78e9052344a0b1e06c0e723c473d37e0b2dd7679a2ab3b8fbbeed9f1dff9c`.

The fresh result exactly reproduces the accepted L1 PPA and timing baseline. No 9-top regression was run.
