# Gate 4.0 W3 Regression After

Result: **PASS**. Software suites: **15/15 PASS**, **400 passed, 0 failed** across **197 runs**.

- C++ vs HLS csim normalized trace comparison: 7/7 PASS.
- Normalized architecture/event/cycle checks: 21/21 PASS.
- Full-program architectural diff: 10/10 PASS.
- Partial-order checks: 7/7 PASS; event-order checks are included in the normalized CSV.
- Merged generation, merged compile, synth-top compile, and core-top compile: 4/4 PASS.

Exact per-suite counts and log hashes are in `suite_results.csv`; trace hashes are in `trace_comparison.csv`; campaign metrics are in `random_metrics.csv`; all canonical artifact hashes are in `artifact_hashes.csv`.

Source scope is the modular `src/*.cpp` implementation, generated `src/boom_core_merged.cpp`, and `src/boom_core_top.cpp`. The unreferenced legacy `src/boom_all.cpp` snapshot is excluded from compilation, evidence, and acceptance.
