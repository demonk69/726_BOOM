#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
BUILD="$ROOT/build/gate5_4_predictor/p1"
REPORT="$ROOT/reports/gate5_4_predictor/p1"
CXXFLAGS=(-std=c++11 -O2 -Wall -Wextra -Werror
          -Wno-error=misleading-indentation -Wno-unknown-pragmas -I"$ROOT/include")
mkdir -p "$BUILD" "$REPORT/logs"

g++ "${CXXFLAGS[@]}" "$ROOT/tb/differential/predecode_tests.cpp" \
  "$ROOT/src/predecode.cpp" "$ROOT/src/rvc.cpp" "$ROOT/src/decode.cpp" \
  "$ROOT/src/divider.cpp" -o "$BUILD/predecode_tests" \
  2>"$REPORT/logs/predecode_directed_compile.log"
"$BUILD/predecode_tests" | tee "$REPORT/logs/predecode_directed.log"
grep -q 'GATE5_4_P1_PREDECODE_DIRECTED_PASS' "$REPORT/logs/predecode_directed.log"

g++ "${CXXFLAGS[@]}" "$ROOT/tb/differential/predecode_packet_tests.cpp" \
  "$ROOT/src/predecode.cpp" -o "$BUILD/predecode_packet_tests" \
  2>"$REPORT/logs/predecode_packet_compile.log"
"$BUILD/predecode_packet_tests" | tee "$REPORT/logs/predecode_packet.log"
grep -q 'GATE5_4_P1_PREDECODE_PACKET_PASS' "$REPORT/logs/predecode_packet.log"

printf '%s\n' 'GATE5_4_P1_PREDECODE_NATIVE_PASS'
