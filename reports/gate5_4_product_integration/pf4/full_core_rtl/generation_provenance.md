# PF4 Current-Source Full-Core RTL Provenance

- Modular source/header SHA-256: `20b4bcd0328224952af16ba4b564dba8060cf2724437d894b268bea124ac8818`.
- Generated merged source SHA-256: `30a8cd5b342182ea307dc4abfc2654a2578920fba0b1d70f05ae362dfa5aa809`.
- Generation/test runner: `scripts/gate5_4/run_pf4_full_core_rtl.sh`.
- HLS top: `boom_core_pf4_rtl_top`; tool version: Vitis HLS/Vivado 2021.2.
- `src/boom_all.cpp` excluded and untouched; generated merged source excluded from aggregate hash.
- Test-only full-core top enables Product FTQ and accepts fixture-only BIM seeds; product ports are unchanged.
- Product programs: `12/12 PASS`.
- Predicted-T younger-fault checks: actual-T masked and actual-NT precise refetch/take, `2/2 PASS`.
- Seeded BIM entries retained their exact 2-bit values after execution.
- The runner recorded every included modular source/header hash before RTL use,
  rejected stale generated RTL by source mtime, and rechecked the aggregate hash
  after simulation. See `source_freshness_manifest.csv`.
- Generated RTL and simulator state lived under
  `/tmp/boom_hls/pf4/full_core_rtl/boom_core_pf4_rtl_top_hls` and were
  `REPRODUCIBLE_WORKSPACE`. `PATH_EXISTS_AT_COMMIT_REQUIRED=false`; the workspace
  was removed after durable matrix, trace, log, and provenance extraction.
- No generated-RTL content hash was retained. Exact generated RTL bytes are not
  accepted durable evidence; current-source provenance is established by the
  source hash gate, generation runner, successful generation log, matrix, and
  committed traces.
