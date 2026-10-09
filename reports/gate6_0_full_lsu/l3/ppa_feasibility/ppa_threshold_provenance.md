# PPA Threshold Provenance

## Scope

This report records the Gate 6.0 L3R5 PPA threshold source and interpretation used by the uncommitted feasibility package.

- Evidence commit: `236733e80a1fd60851436c5b9fe066ff81b09ea7`
- Branch: `handoff/gate6-l3r1-20261008`
- Clean PPA source root: `/home/lab_726/opencode_tmp/g6_l3r5_ppa_source_20261009_ppa1`
- Clean source commit: `236733e80a1fd60851436c5b9fe066ff81b09ea7`
- `src/boom_core_merged.cpp` SHA256: `589f5b5c90bbbbaadc4e2d30c14205c5dc8e5c4c6a904e27de94a4255cd9a5f7`
- `src/boom_all.cpp` clean-source SHA256: `d2a403c7fa1724e5782394e27ffefce90f12b19e5b08dea43ce2b150656b0a74`
- HLS Tcl SHA256: `51071bbd8e083876454ada347786f856cf399e5d09d78d32b717cc130e4c9be0`
- HLS directives SHA256: `4e37fa6f60d80b71056dd966f48c92ad03b10e122dd1fcb367def3c0d14ececd`
- L3 product code modified by this PPA package: `false`

## Baseline

The Gate 6.0 T0 baseline in `reports/gate6_0_full_lsu/t0/baseline_manifest.md` records:

| Metric | Baseline |
|---|---:|
| LUT | 213436 |
| FF | 47337 |
| BRAM_18K | 16 |
| DSP | 3 |
| Estimated period | 6.341 ns |

The canonical baseline source is `reports/gate5_4_pf5_canonical_final/module_csynth_summary.csv`, row `boom_core_top`.

## Gate Budgets

`reports/gate6_0_full_lsu/t0/lsu_cam_ppa_risk.md` defines cumulative budgets from the Gate 6 baseline:

| Gate | LUT review | LUT blocker | BRAM expectation | Period limit |
|---|---:|---:|---|---:|
| L1 | 217705 | 224108 | 16 | 6.500 ns |
| L2 | 224108 | 238340 | 16 | 6.450 ns |
| L3 | 228375 | 245451 | 16 | 6.450 ns |
| L4 | 234780 | 266795 | 16; increase requires review | 6.500 ns |
| L5 | 234780 | 266795 | 16; final measured | 6.500 ns |

`reports/architecture_planning/post_gate5_4/ppa_risk_assessment.md` separately records the global cumulative thresholds:

| Threshold | LUT | Meaning |
|---|---:|---|
| Review | 234780 | +10% cumulative from Gate 6 baseline |
| Blocker | 266795 | +25% cumulative from Gate 6 baseline |

## Interpretation

The L3 planned budget is `228375` LUT review, `245451` LUT blocker, and `6.450 ns` estimated-period limit. The historical global blocker value `266795` exceeds the current device LUT capacity of `230400`, so it cannot be used as a literal current-device pass criterion. This package therefore treats `266795` as historical cumulative planning provenance and keeps `PPA_GATE_STATUS=UNRESOLVED` until the acceptance policy is clarified.
