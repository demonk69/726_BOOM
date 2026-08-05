#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
BUILD=${BOOM_M3B_BUILD_DIR:-"$ROOT/build/gate4_1/m3b_tests"}
REPORT=${BOOM_M3B_REPORT_DIR:-"$ROOT/reports/gate4_1/m3/m3b"}
CXX_BIN=${CXX:-g++}

mkdir -p "$BUILD" "$REPORT/logs"

COMMON=(
  "$ROOT/src/boom_core_step.cpp" "$ROOT/src/frontend.cpp" "$ROOT/src/decode.cpp"
  "$ROOT/src/rename.cpp" "$ROOT/src/rob.cpp" "$ROOT/src/issue.cpp"
  "$ROOT/src/mul.cpp" "$ROOT/src/divider.cpp" "$ROOT/src/execute.cpp"
  "$ROOT/src/branch.cpp" "$ROOT/src/lsu.cpp" "$ROOT/src/completion.cpp"
  "$ROOT/src/commit.cpp" "$ROOT/src/csr.cpp" "$ROOT/src/reset.cpp"
)

for test in divider_integration_tests divider_integration_random_tests divider_full_core_tests; do
  "$CXX_BIN" -std=c++11 -O2 -I"$ROOT/include" "${COMMON[@]}" \
    "$ROOT/tb/differential/$test.cpp" -o "$BUILD/$test" \
    > "$REPORT/logs/${test}_compile.log" 2>&1
  "$BUILD/$test" > "$REPORT/logs/$test.log" 2>&1
done

grep -q 'divider_integration_checks=167' "$REPORT/logs/divider_integration_tests.log"
grep -q 'failures=0' "$REPORT/logs/divider_integration_tests.log"
grep -q 'Gate 4.1 M3B divider integration random: PASS' "$REPORT/logs/divider_integration_random_tests.log"
grep -q 'dropped=0' "$REPORT/logs/divider_integration_random_tests.log"
grep -q 'stale_side_effect=0' "$REPORT/logs/divider_integration_random_tests.log"
grep -q 'M3B native full-core divider programs: 10/10 PASS' "$REPORT/logs/divider_full_core_tests.log"

printf '%s\n' 'Gate 4.1 M3B directed, random, and native full-core tests PASS.'
