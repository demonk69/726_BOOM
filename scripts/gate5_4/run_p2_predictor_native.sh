#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
BUILD="$ROOT/build/gate5_4_predictor/p2"
REPORT="$ROOT/reports/gate5_4_predictor/p2"
mkdir -p "$BUILD" "$REPORT/logs"

CXX=${CXX:-g++}
CXXFLAGS=(-std=c++11 -O2 -Wall -Wextra -Werror -Wno-unknown-pragmas -I"$ROOT/include")

"$CXX" "${CXXFLAGS[@]}" \
  "$ROOT/tb/differential/predictor_foundation_tests.cpp" \
  "$ROOT/src/predictor.cpp" -o "$BUILD/predictor_foundation_tests" \
  2>"$REPORT/logs/predictor_directed_compile.log"
"$BUILD/predictor_foundation_tests" | tee "$REPORT/logs/predictor_directed.log"

"$CXX" "${CXXFLAGS[@]}" \
  "$ROOT/tb/differential/predictor_predecode_composition_tests.cpp" \
  "$ROOT/src/predictor.cpp" "$ROOT/src/predecode.cpp" "$ROOT/src/rvc.cpp" \
  -o "$BUILD/predictor_predecode_composition_tests" \
  2>"$REPORT/logs/predictor_composition_compile.log"
"$BUILD/predictor_predecode_composition_tests" | \
  tee "$REPORT/logs/predictor_composition.log"

grep -q 'failures=0' "$REPORT/logs/predictor_directed.log"
grep -q 'failures=0' "$REPORT/logs/predictor_composition.log"
printf '%s\n' GATE5_4_P2_PREDICTOR_NATIVE_PASS
