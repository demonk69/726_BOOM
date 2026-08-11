#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
BUILD="$ROOT/build/gate5_2_rvc/r1"
REPORT="$ROOT/reports/gate5_2_rvc/r1"
mkdir -p "$BUILD" "$REPORT/logs"

g++ -std=c++11 -O2 -Wall -Wextra -Werror -Wno-error=misleading-indentation \
  -Wno-unknown-pragmas -I"$ROOT/include" \
  "$ROOT/src/rvc.cpp" \
  "$ROOT/tb/differential/rvc_decompress_tests.cpp" \
  -o "$BUILD/rvc_decompress_tests" \
  2>"$REPORT/logs/rvc_decompress_compile.log"

"$BUILD/rvc_decompress_tests" | tee "$REPORT/logs/rvc_decompress_tests.log"
grep -qx 'GATE5_2_R1_RVC_DECOMPRESS_PASS' "$REPORT/logs/rvc_decompress_tests.log"

g++ -std=c++11 -O2 -Wall -Wextra -Werror -Wno-error=misleading-indentation \
  -Wno-unknown-pragmas -I"$ROOT/include" \
  "$ROOT/src/rvc.cpp" "$ROOT/src/decode.cpp" "$ROOT/src/divider.cpp" \
  "$ROOT/tb/differential/rvc_decode_cross_tests.cpp" \
  -o "$BUILD/rvc_decode_cross_tests" \
  2>"$REPORT/logs/rvc_decode_cross_compile.log"

"$BUILD/rvc_decode_cross_tests" | tee "$REPORT/logs/rvc_decode_cross_tests.log"
grep -qx 'GATE5_2_R1_RVC_DECODE_CROSS_PASS' "$REPORT/logs/rvc_decode_cross_tests.log"
