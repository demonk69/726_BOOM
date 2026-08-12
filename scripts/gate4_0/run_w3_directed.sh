#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
BUILD_DIR="$ROOT/build/gate4_0/w3_regression"
LOG_DIR="$ROOT/reports/gate4_0/w3/regression/logs"
CXX_BIN=${CXX:-g++}

mkdir -p "$BUILD_DIR" "$LOG_DIR"

COMMON=(
  "$ROOT/src/boom_core_step.cpp" "$ROOT/src/frontend.cpp" "$ROOT/src/rvc.cpp" "$ROOT/src/decode.cpp"
  "$ROOT/src/rename.cpp" "$ROOT/src/rob.cpp" "$ROOT/src/issue.cpp"
  "$ROOT/src/execute.cpp" "$ROOT/src/branch.cpp" "$ROOT/src/lsu.cpp"
  "$ROOT/src/commit.cpp" "$ROOT/src/csr.cpp" "$ROOT/src/reset.cpp"
)

for test in dispatch_retry_tests w3_dual_execute_tests w3_completion_buffer_tests \
  w3_dual_execute_random_tests; do
  "$CXX_BIN" -std=c++11 -I"$ROOT/include" "${COMMON[@]}" \
    "$ROOT/tb/differential/$test.cpp" -o "$BUILD_DIR/$test"
  "$BUILD_DIR/$test" > "$LOG_DIR/$test.log" 2>&1
done

printf '%s\n' 'Gate 4.0 W3 directed/random regressions complete.'
