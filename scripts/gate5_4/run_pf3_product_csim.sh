#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
BUILD_ROOT=${BOOM_BUILD_ROOT:-/tmp/boom_hls/pf3a}
PROGRAM_BUILD=${PF3_PROGRAM_BUILD:-$BUILD_ROOT/programs}
PF3_PROGRAM_BUILD="$PROGRAM_BUILD" bash "$ROOT/scripts/gate5_4/build_pf3_product_programs.sh"
HLS_BOOM_ROOT="$ROOT" BOOM_BUILD_ROOT="$BUILD_ROOT" PF3_PROGRAM_BUILD="$PROGRAM_BUILD" \
  vitis_hls -f "$ROOT/scripts/gate5_4/pf3_product_csim.tcl" | tee "$BUILD_ROOT/csim.log"
