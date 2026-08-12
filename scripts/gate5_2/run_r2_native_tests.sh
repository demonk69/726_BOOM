#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
BUILD=${BOOM_R2_BUILD_DIR:-"$ROOT/build/gate5_2_rvc/r2/native_final"}
REPORT=${BOOM_R2_REPORT_DIR:-"$ROOT/reports/gate5_2_rvc/r2"}
CXXFLAGS=(-std=c++11 -O2 -Wall -Wextra -Werror -Wno-error=misleading-indentation
          -Wno-unknown-pragmas -I"$ROOT/include")

mkdir -p "$BUILD" "$REPORT/logs"

g++ "${CXXFLAGS[@]}" "$ROOT/tb/differential/rvc_fetch_tests.cpp" \
  "$ROOT/src/frontend.cpp" "$ROOT/src/rvc.cpp" "$ROOT/src/decode.cpp" \
  "$ROOT/src/branch.cpp" "$ROOT/src/divider.cpp" \
  -o "$BUILD/rvc_fetch_tests" \
  2>"$REPORT/logs/rvc_fetch_tests_compile.log"
"$BUILD/rvc_fetch_tests" | tee "$REPORT/logs/rvc_fetch_tests.log"

g++ "${CXXFLAGS[@]}" "$ROOT/tb/differential/rvc_fetch_random_tests.cpp" \
  "$ROOT/src/frontend.cpp" "$ROOT/src/rvc.cpp" "$ROOT/src/divider.cpp" \
  -o "$BUILD/rvc_fetch_random_tests" \
  2>"$REPORT/logs/rvc_fetch_random_tests_compile.log"
"$BUILD/rvc_fetch_random_tests" | tee "$REPORT/logs/rvc_fetch_random_tests.log"

g++ "${CXXFLAGS[@]}" "$ROOT/tb/differential/gate5_2_r2_native_throughput_audit.cpp" \
  "$ROOT/src/frontend.cpp" "$ROOT/src/rvc.cpp" "$ROOT/src/decode.cpp" \
  -o "$BUILD/rvc_throughput_audit" \
  2>"$REPORT/logs/rvc_throughput_compile.log"
"$BUILD/rvc_throughput_audit" "$REPORT/cycle_trace.csv" \
  "$REPORT/throughput_analysis.md" | tee "$REPORT/logs/rvc_throughput.log"
