#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
BUILD=${BOOM_M3C_BUILD_DIR:-"$ROOT/build/gate4_1/m3c_tests"}
REPORT=${BOOM_M3C_REPORT_DIR:-"$ROOT/reports/gate4_1/m3/m3c"}
CXX_BIN=${CXX:-g++}

mkdir -p "$BUILD" "$REPORT/logs"
COMMON=(
  "$ROOT/src/boom_core_step.cpp" "$ROOT/src/frontend.cpp" "$ROOT/src/rvc.cpp" "$ROOT/src/decode.cpp"
  "$ROOT/src/rename.cpp" "$ROOT/src/rob.cpp" "$ROOT/src/issue.cpp"
  "$ROOT/src/mul.cpp" "$ROOT/src/divider.cpp" "$ROOT/src/execute.cpp"
  "$ROOT/src/branch.cpp" "$ROOT/src/lsu.cpp" "$ROOT/src/completion.cpp"
  "$ROOT/src/commit.cpp" "$ROOT/src/csr.cpp" "$ROOT/src/reset.cpp"
)

"$ROOT/scripts/gate4_1/generate_m3c_programs.py" > "$REPORT/logs/generate_programs.log" 2>&1

for test in rv64m_full_tests rv64m_full_random_tests rv64m_full_core_tests; do
  "$CXX_BIN" -std=c++11 -O2 -I"$ROOT/include" "${COMMON[@]}" \
    "$ROOT/tb/differential/$test.cpp" -o "$BUILD/$test" \
    > "$REPORT/logs/${test}_compile.log" 2>&1
  "$BUILD/$test" > "$REPORT/logs/$test.log" 2>&1
done

grep -q 'M3C_RV64M_DIRECTED .*failures=0 status=PASS' \
  "$REPORT/logs/rv64m_full_tests.log"
grep -q 'M3C_RV64M_RANDOM status=PASS' \
  "$REPORT/logs/rv64m_full_random_tests.log"
grep -q 'seeds=256' "$REPORT/logs/rv64m_full_random_tests.log"
grep -q 'cycles_per_seed=2048' "$REPORT/logs/rv64m_full_random_tests.log"
grep -q 'arithmetic_mismatch=0' "$REPORT/logs/rv64m_full_random_tests.log"
grep -q 'protocol_mismatch=0' "$REPORT/logs/rv64m_full_random_tests.log"
grep -q 'M3C native full-core RV64M programs: 15/15 PASS' \
  "$REPORT/logs/rv64m_full_core_tests.log"

printf '%s\n' 'Gate 4.1 M3C joint directed and random tests PASS.'
