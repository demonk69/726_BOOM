#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
REPORT="$ROOT/reports/gate5_3_fetch_buffer/b2"
VITIS_HLS=${VITIS_HLS:-/home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls}
TAG=gate5_3_fetch_buffer_b2
mkdir -p "$REPORT/logs/canonical_csynth"

bash "$ROOT/scripts/gate5_3/run_b1_csynth_sweep.sh" \
  >"$REPORT/logs/canonical_csynth/fetch_buffer.log" 2>&1
BOOM_HLS_GATE="$TAG" VITIS_HLS="$VITIS_HLS" "$ROOT/scripts/run_module_csynth.sh" \
  synth_rvc_top synth_frontend_top synth_divider_top synth_mul_top \
  synth_issue_top synth_execute_top synth_completion_top synth_rob_top \
  >"$REPORT/logs/canonical_csynth/modules.log" 2>&1
BOOM_HLS_GATE="${TAG}_core" VITIS_HLS="$VITIS_HLS" "$ROOT/scripts/run_module_csynth.sh" \
  synth_core_step_top boom_core_top \
  >"$REPORT/logs/canonical_csynth/core.log" 2>&1

cp "$ROOT/reports/$TAG/module_csynth_summary.csv" \
  "$REPORT/logs/canonical_csynth/module_csynth_summary.csv"
cp "$ROOT/reports/${TAG}_core/module_csynth_summary.csv" \
  "$REPORT/logs/canonical_csynth/core_csynth_summary.csv"
printf '%s\n' 'GATE5_3_B2_CANONICAL_CSYNTH_COMPLETE tops=11'
