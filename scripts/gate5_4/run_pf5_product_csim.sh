#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)"}
BUILD_ROOT=${BOOM_BUILD_ROOT:-/tmp/boom_hls/pf5}
PROGRAM_BUILD=${PF5_PROGRAM_BUILD:-$BUILD_ROOT/programs}
REPORT="$ROOT/reports/gate5_4_product_integration/pf5"
mkdir -p -- "$BUILD_ROOT"
PF5_PROGRAM_BUILD="$PROGRAM_BUILD" "$ROOT/scripts/gate5_4/build_pf5_product_programs.sh"
HLS_BOOM_ROOT="$ROOT" BOOM_BUILD_ROOT="$BUILD_ROOT" PF5_PROGRAM_BUILD="$PROGRAM_BUILD" \
  vitis_hls -f "$ROOT/scripts/gate5_4/pf5_product_csim.tcl" |
  tee "$BUILD_ROOT/product_csim.log" "$REPORT/pf5_product_csim.log"
grep -Fxq 'PF5_PRODUCT_PROGRAMS 12/12 PASS' "$BUILD_ROOT/product_csim.log"
[[ $(grep -c '^PF5_PROGRAM .*training_dropped=0 training_duplicate=0 stale_rejected=0 .*verdict=PASS$' \
  "$BUILD_ROOT/product_csim.log") -eq 12 ]]
printf '%s\n' 'PF5_PRODUCT_CSIM_PASS 12/12'
