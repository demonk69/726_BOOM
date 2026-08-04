#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
BUILD_DIR="$ROOT/build/gate4_0/w4a_regression"
REPORT_DIR="$ROOT/reports/gate4_0/w4/regression"
LOG_DIR="$REPORT_DIR/logs"
mkdir -p "$BUILD_DIR" "$LOG_DIR"

BOOM_REGRESSION_BUILD_DIR="$BUILD_DIR/w3_preservation" \
BOOM_REGRESSION_REPORT_DIR="$REPORT_DIR/w3_preservation" \
  "$ROOT/scripts/gate4_0/run_w3_regressions.sh"

COMMON_SRCS=(
  "$ROOT/src/boom_core_step.cpp" "$ROOT/src/frontend.cpp" "$ROOT/src/decode.cpp"
  "$ROOT/src/rename.cpp" "$ROOT/src/rob.cpp" "$ROOT/src/issue.cpp"
  "$ROOT/src/execute.cpp" "$ROOT/src/branch.cpp" "$ROOT/src/lsu.cpp"
  "$ROOT/src/completion.cpp" "$ROOT/src/commit.cpp" "$ROOT/src/csr.cpp"
  "$ROOT/src/reset.cpp"
)
${CXX:-g++} -std=c++11 -I"$ROOT/include" "${COMMON_SRCS[@]}" \
  "$ROOT/tb/differential/w4a_completion_interface_tests.cpp" \
  -o "$BUILD_DIR/w4a_completion_interface_tests" \
  > "$LOG_DIR/w4a_completion_interface_tests_compile.log" 2>&1
"$BUILD_DIR/w4a_completion_interface_tests" \
  > "$LOG_DIR/w4a_completion_interface_tests.log" 2>&1

grep -q 'W4A completion interfaces: 19 passed, 0 failed' \
  "$LOG_DIR/w4a_completion_interface_tests.log"
grep -q 'Software suites:.*400 passed, 0 failed' \
  "$REPORT_DIR/w3_preservation/regression_after.md"
printf '%s\n' 'Gate 4.0 W4A regressions complete: W3 400/400; W4A 19/19.'
