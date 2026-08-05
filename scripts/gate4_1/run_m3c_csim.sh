#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
BUILD=${BOOM_M3C_CSIM_BUILD_DIR:-"$ROOT/build/gate4_1/m3c_csim"}
REPORT=${BOOM_M3C_CSIM_REPORT_DIR:-"$ROOT/reports/gate4_1/m3/m3c/csim"}
VITIS_HLS_BIN=${VITIS_HLS:-/home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls}

rm -rf "$BUILD" "$REPORT"
mkdir -p "$BUILD" "$REPORT"
"$ROOT/scripts/generate_merged.sh" > "$REPORT/generate_merged.log" 2>&1
(
  cd "$BUILD"
  HLS_PROJECT_ROOT="$ROOT" FPGA_PART=xczu7ev-ffvc1156-2-e CLOCK_PERIOD=10 \
    "$VITIS_HLS_BIN" -f "$ROOT/scripts/gate4_1/run_m3c_csim.tcl"
) > "$REPORT/vitis_csim.log" 2>&1
grep -q 'M3C native full-core RV64M programs: 15/15 PASS' "$REPORT/vitis_csim.log"
printf '%s\n' 'Gate 4.1 M3C Vitis csim full-core programs: 15/15 PASS.'
