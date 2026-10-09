# Transaction Stress Final

Final five-config command:

```bash
G6_L3_R2_CONFIGS="4_4 8_8 16_16 4_16 16_4" G6_L3_R2_REUSE_RTL=1 bash scripts/gate6_0/run_l3_r2_extended_rtl_stress.sh
```

Final config status:

- `CONFIG_4_4`: PASS, cases=8, requests=17, responses=16, max_outstanding=4
- `CONFIG_8_8`: PASS, cases=8, requests=21, responses=20, max_outstanding=8
- `CONFIG_16_16`: PASS, cases=8, requests=29, responses=28, max_outstanding=16
- `CONFIG_4_16`: PASS, cases=8, requests=17, responses=16, max_outstanding=4
- `CONFIG_16_4`: PASS, cases=8, requests=29, responses=28, max_outstanding=16

Final required fields:

```text
L3R2_R3_EVIDENCE_COMMIT=dce3de35d1e3c17b5bda215c641fcb3e4a86f8ed
L3R2_R4_CONFIG_4_4=PASS
L3R2_R4_CONFIG_8_8=PASS
L3R2_R4_CONFIG_16_16=PASS
L3R2_R4_CONFIG_4_16=PASS
L3R2_R4_CONFIG_16_4=PASS
L3R2_R4_BACKPRESSURE=PASS
L3R2_R4_STORE_FORWARDING=PASS
L3R2_R4_FLUSH_RECOVERY=PASS
L3R2_R4_MAX_OUTSTANDING_4_4=4
L3R2_R4_MAX_OUTSTANDING_8_8=8
L3R2_R4_MAX_OUTSTANDING_16_16=16
L3R2_R4_ORIGINAL_COVERAGE_PRESERVED=true
L3R2_R4_PRODUCT_SOURCE_UNCHANGED=true
L3R2_R4_FULL_STRESS_STATUS=PASS
L3R2_R4_VERIFICATION_COMPLETE=true
G6_0_L3_MULTI_OUTSTANDING_LOADS_VERIFIED=false
```

`G6_0_L3_MULTI_OUTSTANDING_LOADS_VERIFIED` remains `false` per gate policy, even though R4 reconciled and passed the full R2 stress suite.

## Audit Notes

Each final config log contains 12 `G6_L3_R2_CASE_PASS` markers, 0 `G6_L3_R2_CASE_FAIL` markers, and no `Fatal:`, `SKIP`, or `XFAIL` markers. The final `cases=8` field is the assertion-group counter, not the total PASS-marker count.

The reused RTL was generated from `/home/lab_726/opencode_tmp/g6_l3r2_r2_clean` at commit `105e641089d1a9a5798e00f201eaf2c0a6080da4`. The relevant product-source hashes at current `HEAD` match that clean source root. The local worktree has a pre-existing dirty `src/boom_all.cpp`, but R4 did not modify product source and the dirty worktree file was not used for RTL generation.

`RTL_PROVENANCE_VERIFIED=true` for the reused R4 RTL artifacts listed in `five_config_final_matrix.csv`.
