#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
BUILD="$ROOT/build/gate5_4_predictor/p1"
REPORT="$ROOT/reports/gate5_4_predictor/p1"
mkdir -p "$BUILD" "$REPORT/logs"
g++ -std=c++11 -O2 -Wall -Wextra -Werror -I"$ROOT/include" \
  "$ROOT/tb/differential/predecode_random_tests.cpp" \
  "$ROOT/src/predecode.cpp" -o "$BUILD/predecode_random_tests" \
  2>"$REPORT/logs/predecode_random_compile.log"
"$BUILD/predecode_random_tests" | tee "$REPORT/logs/predecode_random.log"
grep -q 'GATE5_4_P1_PREDECODE_RANDOM_PASS' "$REPORT/logs/predecode_random.log"
printf '%s\n' 'GATE5_4_P1_PREDECODE_RANDOM_PASS'
