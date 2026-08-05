#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
BUILD_DIR=${BOOM_M2B_BUILD_DIR:-"$ROOT/build/gate4_1/m2b_tests"}
REPORT_DIR=${BOOM_M2B_REPORT_DIR:-"$ROOT/reports/gate4_1/m2/m2b"}
LOG_DIR="$REPORT_DIR/logs"
CXX_BIN=${CXX:-g++}

mkdir -p "$BUILD_DIR" "$LOG_DIR"

COMMON=(
  "$ROOT/src/boom_core_step.cpp" "$ROOT/src/frontend.cpp" "$ROOT/src/decode.cpp"
  "$ROOT/src/rename.cpp" "$ROOT/src/rob.cpp" "$ROOT/src/issue.cpp"
  "$ROOT/src/mul.cpp" "$ROOT/src/execute.cpp" "$ROOT/src/branch.cpp"
  "$ROOT/src/lsu.cpp" "$ROOT/src/completion.cpp" "$ROOT/src/commit.cpp"
  "$ROOT/src/csr.cpp" "$ROOT/src/reset.cpp"
)

for test in m2b_execute_tests m2b_execute_random_tests m2b_full_core_tests; do
  "$CXX_BIN" -std=c++11 -I"$ROOT/include" "${COMMON[@]}" \
    "$ROOT/tb/differential/$test.cpp" -o "$BUILD_DIR/$test" \
    > "$LOG_DIR/${test}_compile.log" 2>&1
  "$BUILD_DIR/$test" > "$LOG_DIR/$test.log" 2>&1
done

grep -q 'M2B directed execute integration: 30 passed, 0 failed' \
  "$LOG_DIR/m2b_execute_tests.log"
grep -q 'M2B persistent randomized execute integration: PASS' \
  "$LOG_DIR/m2b_execute_random_tests.log"
grep -q 'M2B full-core multiply program: 10/10 architectural checks PASS' \
  "$LOG_DIR/m2b_full_core_tests.log"

"$ROOT/scripts/gate4_1/run_m2a_mul_tests.sh" \
  > "$LOG_DIR/m2a_arithmetic_regression.log" 2>&1

if [[ ${BOOM_M2B_SKIP_LEGACY:-0} != 1 ]]; then
  BOOM_W4E_BUILD_DIR="$BUILD_DIR/legacy" \
  BOOM_W4E_REPORT_DIR="$REPORT_DIR/legacy" \
    "$ROOT/scripts/gate4_0/run_w4e_regressions.sh" \
    > "$LOG_DIR/legacy_regressions.log" 2>&1
fi

"$ROOT/scripts/generate_merged.sh" > "$LOG_DIR/generate_merged.log" 2>&1
"$CXX_BIN" -std=c++11 -I"$ROOT/include" -c "$ROOT/src/boom_core_merged.cpp" \
  -o "$BUILD_DIR/boom_core_merged.o" > "$LOG_DIR/merged_compile.log" 2>&1
[[ $(grep -c '^// ==== mul.cpp ====$' "$ROOT/src/boom_core_merged.cpp") -eq 1 ]]

python3 - "$LOG_DIR/m2b_execute_random_tests.log" "$REPORT_DIR/random_metrics.csv" <<'PY'
import csv
import re
import sys
from pathlib import Path

source, output = map(Path, sys.argv[1:])
pairs = re.findall(r"^METRIC,([^,]+),(\d+)$", source.read_text(), re.M)
metrics = dict(pairs)
if metrics.get("total_vectors") != "131072" or metrics.get("mismatches") != "0":
    raise SystemExit("M2B random accounting failed")
if any(int(metrics.get(f"uopc_{uopc}_vectors", 0)) == 0 for uopc in range(16, 21)):
    raise SystemExit("M2B operation coverage incomplete")
with output.open("w", newline="", encoding="utf-8") as stream:
    writer = csv.writer(stream)
    writer.writerow(("metric", "value"))
    writer.writerows(pairs)
PY

printf '%s\n' 'Gate 4.1 M2B C++ integration and full-core tests complete.'
