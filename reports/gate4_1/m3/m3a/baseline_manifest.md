# Gate 4.1 M3A Baseline Manifest

- Git commit: `21c618ed944bc7046024dfd410f2806eb8b98e75`
- Commit subject: `Gate 4.1 M2C: verify multiply PPA and RTL`
- Baseline status: `M2_MUL_FAMILY_VERIFIED=true`
- Baseline status: `M2B_PPA_BLOCKER=false`
- Entry condition: `READY_FOR_M3_DIVIDER_CORE=true`
- M2C result: `reports/gate4_1/m2/m2c/m2c_results.md`
- M2C resources: `reports/gate4_1/m2/m2c/final/resource_summary.csv`
- Canonical csynth: 8/8 PASS
- Focused generated RTL: 10/10 PASS
- Full-core generated RTL: 2/2 PASS
- `CORE_CYCLE`: `Pipelined=no`
- W4 completion sources: 3
- PRF write ports: 2
- Divider state before M3A: decode verified, execution not implemented
- Task-before dirty state: `git_status_before.txt`
- Task-before canonical hashes: `source_hashes_before.txt`

The existing dirty `src/boom_all.cpp` is explicitly excluded. M3A does not read it into the merged source, modify it, hash it as canonical input, or use it for testing or synthesis.

## Frozen M2C Evidence Hashes

| Evidence | SHA-256 |
| --- | --- |
| `reports/gate4_1/m2/m2c/m2c_results.md` | `1a7224c073c14c0e45fdaf2975312a7969d7f760a8346bcb02ba3d4eae93e975` |
| `reports/gate4_1/m2/m2c/final/resource_summary.csv` | `881ac7fece39696f06e84d5588f7cbd81336997e479f7df4f54bab6afdc3835c` |
| `reports/gate4_1/m2/m2c/final/rtl/rtl_summary.md` | `ca489705b15412315d64868115d91c50d95aa3a895839aee36f2b72f7b41e5c7` |
| `reports/gate4_1/m2/m2c/final/rtl/full_core_rtl_matrix.csv` | `694015c8609b8cd130498b6ffdf5ec6ff5d28c6c66fe9861718593c18498bb43` |
| `reports/gate4_1/m2/m2c/final/tests/logs/m2b_execute_tests.log` | `32f7a3ccf76914ee0a379f19d937cc26bacdfe8ed0169208d71fa530e58a8488` |
| `reports/gate4_1/m2/m2c/final/tests/logs/m2b_execute_random_tests.log` | `8cb9953893e871dc1610b2ebcd9be5a3e8d0101b3cfb0bc3281c40621311fdde` |
| `reports/gate4_1/m2/m2c/final/tests/logs/m2b_full_core_tests.log` | `f5c74c14a89deb5ab3cc7c17d2f9cbf861baa0094d1bad4e95971d1a56c3ec1a` |
