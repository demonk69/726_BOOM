#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)"}
: "${BOOM_BUILD_ROOT:=/tmp/boom_hls}"
VITIS_HLS_BIN=${VITIS_HLS:-/home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls}
BUILD="$BOOM_BUILD_ROOT/gate5_4_product_integration/pf3/csim"
mkdir -p -- "$BUILD"
HLS_BOOM_ROOT="$ROOT" BOOM_BUILD_ROOT="$BOOM_BUILD_ROOT" "$VITIS_HLS_BIN" \
  -f "$ROOT/scripts/gate5_4/pf3_ftq_atomic_csim.tcl" >"$BUILD/vitis_hls.log" 2>&1
grep -q 'PF3_DIRECTED_PASS.*failures=0' "$BUILD/vitis_hls.log"
printf '%s\n' 'PF3_FTQ_ATOMIC_CSIM_PASS'
