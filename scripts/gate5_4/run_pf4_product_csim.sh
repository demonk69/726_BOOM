#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)"}
BUILD_ROOT=${BOOM_BUILD_ROOT:-/tmp/boom_hls/pf4}
PROGRAM_BUILD=${PF4_PROGRAM_BUILD:-$BUILD_ROOT/programs}
mkdir -p -- "$BUILD_ROOT"
PF4_PROGRAM_BUILD="$PROGRAM_BUILD" "$ROOT/scripts/gate5_4/build_pf4_product_programs.sh"
HLS_BOOM_ROOT="$ROOT" BOOM_BUILD_ROOT="$BUILD_ROOT" PF4_PROGRAM_BUILD="$PROGRAM_BUILD" \
  vitis_hls -f "$ROOT/scripts/gate5_4/pf4_product_csim.tcl" | tee "$BUILD_ROOT/product_csim.log"
grep -Fxq 'PF4_PRODUCT_PROGRAMS 12/12 PASS' "$BUILD_ROOT/product_csim.log"
