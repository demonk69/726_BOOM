# Gate 5.2 R2 Baseline Manifest

- Task-start HEAD: `c2b4e55e45152c2db37b94db4b3a743554ab1024` (`Gate 5.2 R1: verify standalone RVC decompressor`).
- R1 commit: `c2b4e55e45152c2db37b94db4b3a743554ab1024`.
- Gate 5.1R accepted commit: `3f34691b86ed78005d520564dd84416f7e132f42`.
- Gate 5.1 accepted evidence: `reports/gate5_1_frontend/gate5_1_results.md`.
- R1 evidence and hashes: `reports/gate5_2_rvc/r1/baseline_manifest.md`, `source_hashes_before.txt`, and `source_hashes_after.txt`.
- Entry state: `GATE5_1_FRONTEND_FOUNDATION_VERIFIED=true`, `GATE5_2_R1_RVC_DECOMPRESSOR_VERIFIED=true`, `READY_FOR_GATE5_2_R2_RVC_FETCH=true`.
- Task-before dirty scope is frozen in `git_status_before.txt`; protected source hashes are frozen in `source_hashes_before.txt`.
- The tracked dirty files predate R2. R2 does not revert or use them as evidence.
- `src/boom_all.cpp` was dirty before R2 and remains excluded from canonical generation, compilation, synthesis, tests, hashes, and acceptance.

No Gate 5.1 or R1 evidence is overwritten by R2.

## Final R2 Evidence

- Final canonical projects are the ten `/home/lab_726/boom/hls_boom/boom_hls_gate5_2_rvc_r2_repair_*` projects enumerated in `resource_summary.csv`.
- Gate 5.1 comparisons use the frozen values in `gate5_1_resource_baseline.csv`; RVC comparison uses `r1_resource_baseline.csv`.
- The initial full-core RTL timeout was superseded by final generated canonical RTL and 10/10 passing runs. The stale BLOCKED result is corrected in `r2_full_core_results.md` and `r2_full_core_program_matrix.csv`.
- `src/boom_all.cpp` remains explicitly excluded from source hashes, generation, synthesis, simulation, and acceptance.
