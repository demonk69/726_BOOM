#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
BUILD="$ROOT/build/gate5_3_fetch_buffer/b3i"
REPORT="$ROOT/reports/gate5_3_fetch_buffer/b3i"
mkdir -p "$BUILD" "$REPORT/logs"
CXXFLAGS=(-std=c++11 -O2 -Wall -Wextra -Werror
          -Wno-error=misleading-indentation -Wno-error=unused-label
          -Wno-unknown-pragmas -I"$ROOT/include")
SOURCES=("$ROOT/src/boom_core_step.cpp" "$ROOT/src/frontend.cpp"
         "$ROOT/src/fetch_buffer.cpp" "$ROOT/src/fetch_packet.cpp"
         "$ROOT/src/rvc.cpp" "$ROOT/src/decode.cpp" "$ROOT/src/rename.cpp"
         "$ROOT/src/rob.cpp" "$ROOT/src/issue.cpp" "$ROOT/src/mul.cpp"
         "$ROOT/src/divider.cpp" "$ROOT/src/execute.cpp" "$ROOT/src/branch.cpp"
         "$ROOT/src/lsu.cpp" "$ROOT/src/completion.cpp" "$ROOT/src/commit.cpp"
         "$ROOT/src/csr.cpp" "$ROOT/src/reset.cpp")

bash "$ROOT/scripts/gate5_3/build_b3i_programs.sh" \
  >"$REPORT/logs/full_core_native_program_build.log" 2>&1
g++ "${CXXFLAGS[@]}" "${SOURCES[@]}" \
  "$ROOT/tb/differential/gate5_3_b3i_full_core.cpp" \
  -o "$BUILD/full_core_native" 2>"$REPORT/logs/full_core_native_compile.log"
HLS_PROJECT_ROOT="$ROOT" "$BUILD/full_core_native" \
  | tee "$REPORT/logs/full_core_native.log"
grep -q 'GATE5_3_B3I_FULL_CORE 6/6 PASS' "$REPORT/logs/full_core_native.log"
printf '%s\n' 'GATE5_3_B3I_FULL_CORE_NATIVE_PASS 6/6'
