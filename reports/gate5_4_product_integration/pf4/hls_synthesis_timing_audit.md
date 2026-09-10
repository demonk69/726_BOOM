# PF4 HLS Synthesis Timing Audit

```text
HLS_SYNTH_TIMING_AUDIT=PASS
MEASURED_STAGE=VITIS_HLS_CSYNTH_DESIGN
EXISTING_SYNTH_SOURCE_HASH_MATCH=true
EXISTING_BOOM_CORE_TOP_TIMING_VALID=true
SYNTH_ALREADY_AVAILABLE=true
SYNTH_RERUN_PERFORMED=false
TIMING_SOURCE=EXISTING_CURRENT_SOURCE_CANONICAL_HLS_LOG
SYNTH_TOP=boom_core_top
VITIS_HLS_VERSION=2021.2
SYNTH_PART=xczu7ev-ffvc1156-2-e
SYNTH_START_TIME=2026-09-09T22:44:36+08:00
SYNTH_END_TIME=2026-09-09T22:52:10+08:00
SYNTH_WALL_SECONDS=454.90
SYNTH_WALL_HMS=00:07:34.90
SYNTH_USER_SECONDS=450.50
SYNTH_SYSTEM_SECONDS=4.46
SYNTH_MAX_RSS_KB=5930896
FULL_CORE_LUT=210914
FULL_CORE_FF=46551
FULL_CORE_BRAM=16
FULL_CORE_DSP=3
TARGET_CLOCK_NS=10.000
FULL_CORE_PERIOD_NS=6.341
CLOCK_MARGIN_NS=3.659
PF4_PPA_REPORT_MATCH_RAW_HLS=true
SYNTH_TIMING_AUDIT_INCONSISTENCY=false
LUT_DELTA_FROM_PF3=12033
LUT_DELTA_PERCENT_FROM_PF3=6.050
FF_DELTA_FROM_PF3=1108
BRAM_DELTA_FROM_PF3=0
DSP_DELTA_FROM_PF3=0
PERIOD_DELTA_FROM_PF3=0.000
REPOSITORY_HYGIENE_PRESERVED=true
REPOSITORY_TOTAL_BYTES=2611818496
BUILD_TOTAL_BYTES=0
WORKSPACE_SIZE_WARNING=NONE
SRC_BOOM_ALL_UNCHANGED=true
PF4_COMMITTED=false
PF5_STARTED=false
```

## Provenance

- Canonical source: `src/boom_core_merged.cpp`, SHA-256
  `30a8cd5b342182ea307dc4abfc2654a2578920fba0b1d70f05ae362dfa5aa809`.
- Excluded source: `src/boom_all.cpp`, unchanged SHA-256
  `d6f885632ddd445729adda8148ea256e67683ccc8e7f2b10c9951e915d92c76c`.
- Raw Vitis log: `reports/gate5_4_pf4_canonical/module_csynth/boom_core_top.log`.
- Runner timing: `reports/gate5_4_pf4_canonical/module_csynth/boom_core_top.time`.
- Parsed raw-report summary:
  `reports/gate5_4_pf4_canonical/module_csynth_summary.csv`.
- PF4 PPA table: `pf4_canonical_synthesis.csv`.
- Original temporary raw report path:
  `/tmp/boom_hls/pf4/canonical/boom_hls_gate5_4_pf4_canonical_boom_core_top/solution_module/syn/report/boom_core_top_csynth.rpt`.
  The temporary solution workspace has since been removed under workspace
  hygiene policy; its values are preserved in the parsed summary and were
  checked against the raw report during PF4 closure.

The runner used `/usr/bin/time` around one Vitis HLS invocation configured with
top `boom_core_top`, the canonical merged source, LUTRAM defines, 10 ns clock,
and the accepted target part. The raw log shows `csynth_design` beginning and
finishing successfully in 452.46 seconds, followed by normal Vitis exit. The
runner measured 454.90 seconds wall time for the complete invocation. CPU times
come from the Vitis total line; maximum RSS comes from `/usr/bin/time`.
