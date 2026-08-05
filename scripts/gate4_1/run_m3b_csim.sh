#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
BUILD=${BOOM_M3B_CSIM_BUILD_DIR:-"$ROOT/build/gate4_1/m3b_csim"}
REPORT=${BOOM_M3B_CSIM_REPORT_DIR:-"$ROOT/reports/gate4_1/m3/m3b/csim"}
VITIS_HLS_BIN=${VITIS_HLS:-/home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls}

rm -rf "$BUILD" "$REPORT"
mkdir -p "$BUILD" "$REPORT"
"$ROOT/scripts/generate_merged.sh" > "$REPORT/generate_merged.log" 2>&1
(
  cd "$BUILD"
  HLS_PROJECT_ROOT="$ROOT" FPGA_PART=xczu7ev-ffvc1156-2-e CLOCK_PERIOD=10 \
    "$VITIS_HLS_BIN" -f "$ROOT/scripts/gate4_1/run_m3b_csim.tcl"
) > "$REPORT/vitis_csim.log" 2>&1
grep -q 'M3B native full-core divider programs: 10/10 PASS' "$REPORT/vitis_csim.log"
printf '%s\n' 'Gate 4.1 M3B Vitis csim full-core programs: 10/10 PASS.'
