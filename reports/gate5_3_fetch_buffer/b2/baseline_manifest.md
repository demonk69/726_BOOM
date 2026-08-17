# Gate 5.3 B2 Baseline Manifest

- Task start HEAD: `48f50e8769f13f394c2f98eef253f45d1d877650`
- B1 commit: `48f50e8` (`Gate 5.3 B1: verify standalone fetch buffer`)
- Gate 5.2 R3 commit: `dfaf8e9` (`Gate 5.2 R3: close RV64C decode gaps`)
- Accepted entry state: `GATE5_1_FRONTEND_FOUNDATION_VERIFIED=true`, `GATE5_2_RVC_VERIFIED=true`, `GATE5_2_RVC_PROTECTED_DECODE_GAPS=0`, `GATE5_3_B1_FETCH_BUFFER_VERIFIED=true`, `GATE5_3_B1_STANDALONE_ONLY=true`.
- Accepted Frontend: one logical outstanding IMEM request, fetch ID/epoch/address validation, stale drain, redirect-over-response, runtime reset, mixed RV64C/base fetch, PC +2/+4, cross-word assembly, one canonical instruction output.
- Accepted B1: depth 8, storage AUTO, control-only reset, atomic four-lane masked enqueue, one-wide dequeue, no bypass requirement.
- Accepted Gate 5.2 R3 full core: 126903 LUT, 27904 FF, 16 BRAM, 3 DSP, 6.341 ns, `CORE_CYCLE Pipelined=no`.
- Task-before tracked dirty scope is captured verbatim in `git_status_before.txt`. It consists of historical report/log updates plus `src/boom_all.cpp` and `vitis_hls.log`; none is part of B2.
- `src/boom_all.cpp` status: pre-existing modified file, explicitly excluded from B2 edits, generation, compilation manifests, and source-hash acceptance set.

The B2 product path is constrained to one complete instruction per cycle using B1 lane 0 only. No packet constructor, FTQ, predictor, ICache, Decode/Dispatch widening, Full LSU/FPU, DATAFLOW, false dependence, complete array partition, or core-cycle pipeline is in scope.
