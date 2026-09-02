# Gate 5.4 P1 Baseline Manifest

- Freeze date: 2026-08-24.
- Requested repository: `/home/lab_726/boom`; actual Git worktree: `/home/lab_726/boom/hls_boom`.
- Branch: `gate3.8-rtl-verification`.
- HEAD: `ef051ca4e3673d663f9d10b55029c956fbc0052a` (`Gate 5.3: finalize fetch buffer architecture`).
- Gate 5.3 accepted: `GATE5_3_FETCH_BUFFER_VERIFIED=true`.
- F0 accepted: `GATE5_4_F0_FTQ_PREREQUISITES_REVIEWED=true`.
- P0 accepted: `GATE5_4_P0_PREDICTOR_INTERFACE_REVIEWED=true` and `READY_FOR_PREDECODE_IMPLEMENTATION=true`.
- Historical dirty state is nonempty and preserved. There were 845 status entries at freeze time, including 34 tracked modifications and extensive historical generated/report artifacts.
- `src/boom_all.cpp` was already modified before P1. It is excluded from P1, was not read or edited, and its frozen SHA-256 is `d6f885632ddd445729adda8148ea256e67683ccc8e7f2b10c9951e915d92c76c`.
- P1 may add standalone predecode source, tests, wrappers, generated merged source, scripts, reports, and docs only. It does not connect predecode to product Frontend or backend control flow.

Accepted canonical configuration remains depth 8, AUTO Fetch Buffer storage, CONTROL_ONLY reset, packet width 2, 32-bit IMEM response, one logical outstanding request, and one-wide Decode/Dispatch/Commit. Accepted full-core PPA remains 135953 LUT, 33373 FF, 16 BRAM, 3 DSP, 6.341 ns, with `CORE_CYCLE Pipelined=no`.
