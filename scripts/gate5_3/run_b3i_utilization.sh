#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
BUILD="$ROOT/build/gate5_3_fetch_buffer/b3i"
REPORT="$ROOT/reports/gate5_3_fetch_buffer/b3i"
mkdir -p "$BUILD" "$REPORT/logs"
CXXFLAGS=(-std=c++11 -O2 -Wall -Wextra -Werror
          -Wno-error=misleading-indentation -Wno-error=unused-label
          -Wno-unknown-pragmas -I"$ROOT/include")
SOURCES=("$ROOT/src/frontend.cpp" "$ROOT/src/fetch_buffer.cpp"
         "$ROOT/src/fetch_packet.cpp" "$ROOT/src/rvc.cpp"
         "$ROOT/src/decode.cpp" "$ROOT/src/divider.cpp")

g++ "${CXXFLAGS[@]}" "$ROOT/tb/differential/fetch_packet_2lane_utilization.cpp" \
  "${SOURCES[@]}" -o "$BUILD/utilization" 2>"$REPORT/logs/utilization_compile.log"
"$BUILD/utilization" "$REPORT/packet_utilization.csv" \
  "$REPORT/throughput_comparison.csv" "$REPORT/throughput_analysis.md" \
  | tee "$REPORT/logs/utilization.log"
grep -q 'GATE5_3_B3I_UTILIZATION_PASS scenarios=6 native_call_qualified=true' \
  "$REPORT/logs/utilization.log"
