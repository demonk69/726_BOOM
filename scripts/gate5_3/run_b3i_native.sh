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
         "$ROOT/src/predecode.cpp" "$ROOT/src/predictor.cpp"
         "$ROOT/src/decode.cpp" "$ROOT/src/divider.cpp")

g++ "${CXXFLAGS[@]}" "$ROOT/tb/differential/fetch_packet_2lane_tests.cpp" \
  "${SOURCES[@]}" -o "$BUILD/directed" 2>"$REPORT/logs/directed_compile.log"
"$BUILD/directed" | tee "$REPORT/logs/directed.log"
grep -q 'GATE5_3_B3I_FETCH_PACKET_2LANE_PASS' "$REPORT/logs/directed.log"
bash "$ROOT/scripts/gate5_3/run_b3i_random.sh"
bash "$ROOT/scripts/gate5_3/run_b3i_utilization.sh"
printf '%s\n' 'GATE5_3_B3I_NATIVE_PASS directed random=256x4096 utilization=6'
