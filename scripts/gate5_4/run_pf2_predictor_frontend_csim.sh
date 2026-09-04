#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)"}
: "${BOOM_BUILD_ROOT:=/tmp/boom_hls}"
REPORT="$ROOT/reports/gate5_4_product_integration/pf2"
VITIS_HLS_BIN=${VITIS_HLS:-/home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls}
mkdir -p -- "$REPORT/logs"
HLS_BOOM_ROOT="$ROOT" BOOM_BUILD_ROOT="$BOOM_BUILD_ROOT" "$VITIS_HLS_BIN" \
    -f "$ROOT/scripts/gate5_4/pf2_predictor_frontend_csim.tcl" >"$REPORT/logs/pf2_predictor_frontend_csim.log" 2>&1
grep -Fxq 'PF2_DIRECTED_PASS checks=2239 failures=0 conditional_mode=SHADOW_ONLY' \
    "$REPORT/logs/pf2_predictor_frontend_csim.log"
printf '%s\n' 'PF2_PREDICTOR_FRONTEND_CSIM_PASS checks=2239 conditional_mode=SHADOW_ONLY'
