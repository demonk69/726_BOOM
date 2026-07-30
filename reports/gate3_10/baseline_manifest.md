# Gate 3.10 Baseline Manifest

Frozen commit: `557bdf55fe4798096b4bd6be68e50c72f8b1e07e`.

The baseline is referenced from immutable commit `557bdf5`; Gate 3.9 evidence is not copied or overwritten.

The live Gate 3.9 HLS database's 20 `*.verbose.sched.rpt` helper schedules are preserved in `reports/gate3_10/baseline_schedule/`.

| Evidence | Frozen path | SHA-256 |
|---|---|---|
| `reports/gate3_9/variants/F1_FINE_GRAIN_RESET/boom_core_top_csynth.rpt` | commit 557bdf5 | `0608cdc427acb1c5830a3d92a5c1bfc84b36d01d9b93ba5064ff3a1bc22789de` |
| `reports/gate3_9/variants/F1_FINE_GRAIN_RESET/conservative_rtl/boom_core_top.v` | commit 557bdf5 | `d2aca3277048bd94c47ed2085919ec559980e1b9090afbf406d583128310318c` |
| `reports/gate3_9/rtl_test_matrix.csv` | commit 557bdf5 | `f639bc15f89aa5f1a9ffcb146fdd7b632f95ac2f8cfb02062b10be7040324add` |
| `reports/gate3_9/reset_latency.csv` | commit 557bdf5 | `15bd5572671abfa1f0e5f6679ea9431ea310e3c4f57c66c6082ea9bbddba320b` |
| `reports/gate3_9/normal_rtl_trace_comparison.csv` | commit 557bdf5 | `516327ccb732ade9b36375ced427288b30113a94fcd75d18a0b0964ed34c4166` |
| `reports/gate3_9/regression_after_artifacts/trace_diff.md` | commit 557bdf5 | `4107431b8f5f33930f4b865a617d959197285e2ab56dbccb85fb18743b89fc7b` |
| `reports/gate3_9/regression_after_artifacts/full_program_architectural_diff.md` | commit 557bdf5 | `2a072e422888014609e5df6a966b482167612fb15650bd4abf8dcc9c1ec9056b` |

BOOM reference traces remain unavailable; M013 and strict cycle equivalence remain insufficient evidence.
