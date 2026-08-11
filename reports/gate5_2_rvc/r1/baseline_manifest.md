# Gate 5.2 R1 Baseline Manifest

- Git root: `/home/lab_726/boom/hls_boom`
- Git commit: `3f34691b86ed78005d520564dd84416f7e132f42`
- Commit subject: `Gate 5.1R: verify frontend foundation repair`
- Entry condition: `READY_FOR_GATE5_2_RVC=true`
- Accepted baseline: `reports/gate5_1_frontend/gate5_1_results.md`
- Baseline canonical csynth: 9/9 PASS
- Baseline Gate 3.9 full-core RTL: 49/49 PASS
- Baseline focused Frontend RTL: 33/33 PASS
- Baseline W3 regression: 400/400 PASS
- Task-before tracked dirty files are recorded in `git_status_before.txt`.
- Task-before protected hashes are recorded in `source_hashes_before.txt`.

`src/boom_all.cpp` was dirty before R1. It is excluded from canonical source,
tests, synthesis, hashes, evidence, and all R1 modifications.
