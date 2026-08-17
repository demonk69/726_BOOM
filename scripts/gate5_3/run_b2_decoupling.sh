#!/usr/bin/env bash
set -euo pipefail
ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
BUILD="$ROOT/build/gate5_3_fetch_buffer/b2/native"
REPORT="$ROOT/reports/gate5_3_fetch_buffer/b2"
mkdir -p "$BUILD" "$REPORT/logs"
g++ -std=c++11 -O2 -Wall -Wextra -Werror -Wno-unknown-pragmas -I"$ROOT/include" \
  "$ROOT/tb/differential/fetch_buffer_decoupling_audit.cpp" \
  "$ROOT/src/frontend.cpp" "$ROOT/src/fetch_buffer.cpp" "$ROOT/src/rvc.cpp" \
  "$ROOT/src/divider.cpp" \
  -o "$BUILD/decoupling_audit" 2>"$REPORT/logs/decoupling_compile.log"
"$BUILD/decoupling_audit" "$REPORT/decoupling_metrics.csv" \
  "$REPORT/throughput_analysis.md" | tee "$REPORT/logs/decoupling.log"
