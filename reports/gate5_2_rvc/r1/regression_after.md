# Gate 5.2 R1 Regression After Repair

| Required suite | Result | Evidence |
|---|---|---|
| RVC directed | 228/228 PASS | `logs/rvc_decompress_tests.log` |
| RVC exhaustive | 65,536/65,536 exact PASS | `logs/rvc_decompress_tests.log` |
| RVC Decode cross | 38,294/38,294 PASS | `logs/rvc_decode_cross_tests.log` |
| Merged source compile | PASS | final canonical merged compile |
| `synth_rvc_top` | PASS | `csynth/synth_rvc_top_csynth.rpt` |
| `synth_frontend_top` preservation | PASS | `csynth/synth_frontend_top_csynth.rpt` |
| Gate 5.1 focused current-source | PASS | `old_regression/repair/frontend/test.log` |
| W3 current-source | 400/400 PASS | `old_regression/repair/w3/regression_after.md` |
| RV64M native full-core | 15/15 PASS | `old_regression/repair/m3c/logs/rv64m_full_core_tests.log` |
| RV64M directed/random | PASS | `old_regression/repair/m3c/logs/` |

W3 additionally reports normalized traces 7/7, architecture/event/cycle checks
21/21, full-program diff 10/10, and partial order 7/7.
