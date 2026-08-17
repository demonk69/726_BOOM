# Gate 5.3 B2 Phase D Generation Provenance

- Phase: `D`, generated full-core RTL verification prerequisite only.
- Acceptance claim: not phase F canonical acceptance.
- Synthesized top: `boom_core_top` only; raw `boom_core_step` was not synthesized.
- Modular source/header SHA-256: `de58592986a52cbb30044a53a95f15872b040a744b4c8c7bacf40f45e7dda107`.
- Current-hash build key: `de58592986a52cbb`.
- Generated merged-source SHA-256: `7068ef7ea7c2f3f91cd6ac5464e64f1e99b2de5610235bfe17605e3b1f979e8b`.
- Generated RTL top: `/home/lab_726/boom/hls_boom/build/gate5_3_fetch_buffer/b2/phase_d/retry_20260816/boom_core_top_hls/solution_phase_d/syn/verilog/boom_core_top.v`.
- Freshness: PASS; all included modular source/header hashes remained stable and no input was newer than RTL.
- Explicit exclusions: `src/boom_all.cpp` (non-modular aggregate), `src/boom_core_merged.cpp` (generated input).
- XSim mixed-RVC result: `11/11 PASS`; every row passed signature and committed tohost checks.
