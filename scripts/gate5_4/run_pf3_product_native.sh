#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
BUILD_ROOT=${BOOM_BUILD_ROOT:-/tmp/boom_hls/pf3a}
BUILD="$BUILD_ROOT/native"
PROGRAM_BUILD=${PF3_PROGRAM_BUILD:-$BUILD_ROOT/programs}
mkdir -p "$BUILD"
PF3_PROGRAM_BUILD="$PROGRAM_BUILD" bash "$ROOT/scripts/gate5_4/build_pf3_product_programs.sh"
CXXFLAGS=(-std=c++11 -O2 -Wall -Wextra -Werror
          -Wno-error=misleading-indentation -Wno-error=unused-label
          -Wno-unknown-pragmas -DBOOM_FTQ_STORAGE_LUTRAM -I"$ROOT/include")
SOURCES=("$ROOT/src/boom_core_step.cpp" "$ROOT/src/frontend.cpp"
         "$ROOT/src/fetch_buffer.cpp" "$ROOT/src/fetch_packet.cpp"
         "$ROOT/src/predecode.cpp" "$ROOT/src/predictor.cpp"
         "$ROOT/src/rvc.cpp" "$ROOT/src/decode.cpp" "$ROOT/src/rename.cpp"
         "$ROOT/src/rob.cpp" "$ROOT/src/issue.cpp" "$ROOT/src/mul.cpp"
         "$ROOT/src/divider.cpp" "$ROOT/src/execute.cpp" "$ROOT/src/branch.cpp"
         "$ROOT/src/lsu.cpp" "$ROOT/src/completion.cpp" "$ROOT/src/commit.cpp"
         "$ROOT/src/csr.cpp" "$ROOT/src/reset.cpp")
g++ "${CXXFLAGS[@]}" "${SOURCES[@]}" \
  "$ROOT/tb/differential/pf3_product_programs.cpp" -o "$BUILD/pf3_product_programs"
PF3_PROGRAM_BUILD="$PROGRAM_BUILD" "$BUILD/pf3_product_programs" | tee "$BUILD_ROOT/native.log"
