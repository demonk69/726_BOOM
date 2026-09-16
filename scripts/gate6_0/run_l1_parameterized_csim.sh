#!/bin/bash
set -euo pipefail

ROOT="${HLS_BOOM_ROOT:-$(cd "$(dirname "$0")/../.." && pwd)}"
BUILD_ROOT="${BOOM_BUILD_ROOT:-/tmp/boom_hls/g6_l1/csim}"
VITIS_HLS="${VITIS_HLS:-/home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls}"

mkdir -p "$BUILD_ROOT"
for config in 4_4 8_8 16_16 4_16 16_4; do
    lq_depth="${config%_*}"
    sq_depth="${config#*_}"
    HLS_BOOM_ROOT="$ROOT" BOOM_BUILD_ROOT="$BUILD_ROOT" \
        LQ_DEPTH="$lq_depth" SQ_DEPTH="$sq_depth" \
        "$VITIS_HLS" -f "$ROOT/scripts/gate6_0/l1_parameterized_csim.tcl" \
        > "$BUILD_ROOT/$config.log" 2>&1
    printf 'L1_CSIM_CONFIG_%s=PASS\n' "$config"
done
