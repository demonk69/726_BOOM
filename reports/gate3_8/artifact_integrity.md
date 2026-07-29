# Gate 3.8 Artifact Integrity

Date: 2026-07-28

| Check | Result |
|---|---|
| Frozen core/directive/Gate 3.7 script hashes | PASS, 32/32 unchanged |
| Baseline versus regenerated conservative RTL | PASS, byte-identical directory comparison |
| Intentional frozen-document deltas | `docs/accepted_ppa_configuration.md`, `docs/equivalence_summary.md`, `docs/implementation_status.md` |
| Conservative csynth reproduction | PASS, 83286 LUT / 16611 FF / 16 BRAM_18K / 3 DSP / 5.898 ns |

`reports/gate3_8/source_hashes_before.txt` records the pre-gate manifest. `reports/gate3_8/source_hashes_after.txt` records the final values and differs only for the three frozen documents updated with Gate 3.8 findings. Core source, headers, directives, and inherited Gate 3.7 scripts are unchanged.

The generated RTL comparison used `diff -qr` between `reports/gate3_8/baseline_artifacts/conservative_rtl/` and `reports/gate3_8/fixed_artifacts/conservative_rtl/` and produced no differences.
