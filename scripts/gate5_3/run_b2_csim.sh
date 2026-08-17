#!/usr/bin/env bash
set -euo pipefail
ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
REPORT="$ROOT/reports/gate5_3_fetch_buffer/b2"
mkdir -p "$REPORT/logs"
bash "$ROOT/scripts/gate5_2/build_rvc_programs.sh" >"$REPORT/logs/b2_csim_program_build.log" 2>&1
/home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls \
  -f "$ROOT/scripts/gate5_3/b2_full_core_csim.tcl" >"$REPORT/logs/b2_csim.log" 2>&1
grep -q 'GATE5_2_R2_FULL_CORE_RVC 11/11 PASS' "$REPORT/logs/b2_csim.log"
printf '%s\n' 'GATE5_3_B2_FULL_CORE_CSIM_RVC_PASS 11/11'
