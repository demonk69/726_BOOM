#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
BUILD="$ROOT/build/gate5_4_predictor/p2"
REPORT="$ROOT/reports/gate5_4_predictor/p2"
mkdir -p "$BUILD" "$REPORT/logs"

CXX=${CXX:-g++}
"$CXX" -std=c++11 -O2 -Wall -Wextra -Werror -Wno-unknown-pragmas \
  -I"$ROOT/include" \
  "$ROOT/tb/differential/predictor_foundation_random_tests.cpp" \
  "$ROOT/src/predictor.cpp" -o "$BUILD/predictor_foundation_random_tests" \
  2>"$REPORT/logs/predictor_random_compile.log"
"$BUILD/predictor_foundation_random_tests" 256 8192 | \
  tee "$REPORT/logs/predictor_random.log"

grep -q 'total_errors=0' "$REPORT/logs/predictor_random.log"
printf '%s\n' GATE5_4_P2_PREDICTOR_RANDOM_PASS
