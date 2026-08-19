#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
REPORT="$ROOT/reports/gate5_3_fetch_buffer/b3i"
VITIS_HLS_BIN=${VITIS_HLS:-/home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls}
mkdir -p "$REPORT/logs"
"$ROOT/scripts/gate5_3/build_b3i_programs.sh" >"$REPORT/logs/csim_program_build.log" 2>&1
HLS_PROJECT_ROOT="$ROOT" "$VITIS_HLS_BIN" \
  -f "$ROOT/scripts/gate5_3/b3i_full_core_csim.tcl" >"$REPORT/logs/b3i_csim.log" 2>&1
grep -q 'GATE5_3_B3I_FULL_CORE 6/6 PASS' "$REPORT/logs/b3i_csim.log"
printf '%s\n' 'GATE5_3_B3I_FULL_CORE_CSIM_PASS 6/6'
