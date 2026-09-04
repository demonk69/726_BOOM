#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)"}
: "${BOOM_BUILD_ROOT:=/tmp/boom_hls}"
REPORT="$ROOT/reports/gate5_4_product_integration/pf1"
VITIS_HLS_BIN=${VITIS_HLS:-/home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls}
mkdir -p -- "$REPORT/logs"
HLS_BOOM_ROOT="$ROOT" BOOM_BUILD_ROOT="$BOOM_BUILD_ROOT" "$VITIS_HLS_BIN" \
    -f "$ROOT/scripts/gate5_4/pf1_exception_csim.tcl" >"$REPORT/logs/pf1_exception_csim.log" 2>&1
grep -q 'PF1_PROGRAMS cases=8 failures=0' "$REPORT/logs/pf1_exception_csim.log"
printf '%s\n' 'PF1_EXCEPTION_CSIM_PASS cases=8'
