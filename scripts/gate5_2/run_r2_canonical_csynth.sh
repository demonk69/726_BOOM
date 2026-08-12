#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
TAG=${BOOM_R2_CSYNTH_TAG:-gate5_2_rvc_r2_repair}
VITIS_HLS=${VITIS_HLS:-/home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls}
REPORT="$ROOT/reports/gate5_2_rvc/r2/logs/canonical_csynth"

mkdir -p "$REPORT"
BOOM_HLS_GATE="$TAG" VITIS_HLS="$VITIS_HLS" "$ROOT/scripts/run_module_csynth.sh" \
  synth_rvc_top synth_frontend_top synth_divider_top synth_mul_top \
  synth_issue_top synth_execute_top synth_completion_top synth_rob_top \
  | tee "$REPORT/module_tops.log"
BOOM_HLS_GATE="${TAG}_core" VITIS_HLS="$VITIS_HLS" \
  "$ROOT/scripts/run_module_csynth.sh" synth_core_step_top boom_core_top \
  | tee "$REPORT/core_tops.log"

cp "$ROOT/reports/$TAG/module_csynth_summary.csv" "$REPORT/module_csynth_summary.csv"
cp "$ROOT/reports/${TAG}_core/module_csynth_summary.csv" "$REPORT/core_csynth_summary.csv"
"$VITIS_HLS" -version > "$REPORT/tool_versions.log" 2>&1

python3 - "$REPORT/module_csynth_summary.csv" 8 \
          "$REPORT/core_csynth_summary.csv" 2 <<'PY'
import csv
import sys

for path, expected in zip(sys.argv[1::2], map(int, sys.argv[2::2])):
    with open(path, newline='') as stream:
        rows = list(csv.DictReader(stream))
    if len(rows) != expected or any(row['status'] != 'PASS' for row in rows):
        raise SystemExit('canonical csynth did not pass every requested top: ' + path)
PY
