#!/usr/bin/env bash
set -euo pipefail
ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
REPORT="$ROOT/reports/gate5_2_rvc/r2"
BUILD="$ROOT/tb/differential/gate5_2_r2_csim_build"
VITIS_HLS_BIN=${VITIS_HLS:-/home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls}
mkdir -p "$REPORT/logs" "$BUILD"
bash "$ROOT/scripts/gate5_2/build_rvc_programs.sh" > "$REPORT/logs/r2_csim_program_build.log" 2>&1
(
  cd "$BUILD"
  HLS_PROJECT_ROOT="$ROOT" "$VITIS_HLS_BIN" -f "$ROOT/scripts/gate5_2/run_r2_csim.tcl"
) > "$REPORT/logs/r2_csim.log" 2>&1
grep -q 'GATE5_2_R2_FULL_CORE_RVC 11/11 PASS' "$REPORT/logs/r2_csim.log"
printf '%s\n' 'Gate 5.2 R3 Vitis csim full-core mixed RVC: 11/11 PASS'
