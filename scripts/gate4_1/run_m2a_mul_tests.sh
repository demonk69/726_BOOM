#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
BUILD_DIR=${BOOM_M2A_BUILD_DIR:-"$ROOT/build/gate4_1/m2a_tests"}
REPORT_DIR=${BOOM_M2A_REPORT_DIR:-"$ROOT/reports/gate4_1/m2/m2a"}
LOG_DIR="$REPORT_DIR/logs"
CXX_BIN=${CXX:-g++}
VITIS_INCLUDE=${VITIS_HLS_INCLUDE:-/home/lab_726/Xilinx/Vitis_HLS/2021.2/include}

mkdir -p "$BUILD_DIR" "$LOG_DIR"

COMMON_FLAGS=(-std=c++11 -I"$ROOT/include" -I"$VITIS_INCLUDE" -DBOOM_USE_AP_INT)

"$CXX_BIN" "${COMMON_FLAGS[@]}" "$ROOT/src/mul.cpp" \
  "$ROOT/tb/differential/multiply_tests.cpp" -o "$BUILD_DIR/multiply_tests" \
  > "$LOG_DIR/multiply_tests_compile.log" 2>&1
"$BUILD_DIR/multiply_tests" > "$LOG_DIR/multiply_tests.log" 2>&1

"$CXX_BIN" "${COMMON_FLAGS[@]}" "$ROOT/src/mul.cpp" \
  "$ROOT/tb/differential/multiply_random_tests.cpp" -o "$BUILD_DIR/multiply_random_tests" \
  > "$LOG_DIR/multiply_random_tests_compile.log" 2>&1
"$BUILD_DIR/multiply_random_tests" > "$LOG_DIR/multiply_random_tests.log" 2>&1

grep -q 'M2A directed multiply: 51 passed, 0 failed' "$LOG_DIR/multiply_tests.log"
grep -q 'M2A arithmetic random: 256 seeds, 131072 vectors, 0 mismatches' \
  "$LOG_DIR/multiply_random_tests.log"

python3 - "$LOG_DIR/multiply_random_tests.log" "$REPORT_DIR/arithmetic_random_metrics.csv" <<'PY'
import csv
import re
import sys
from pathlib import Path

source, output = map(Path, sys.argv[1:])
pairs = re.findall(r"^METRIC,([^,]+),(\d+)$", source.read_text(), re.M)
metrics = dict(pairs)
required = {"total_vectors": 131072, "mismatches": 0}
for name, expected in required.items():
    if int(metrics.get(name, -1)) != expected:
        raise SystemExit(f"metric mismatch: {name}")
if sum(int(metrics[f"vectors_operation_{index}"]) for index in range(5)) != 131072:
    raise SystemExit("operation coverage does not conserve vectors")
with output.open("w", newline="", encoding="utf-8") as stream:
    writer = csv.writer(stream)
    writer.writerow(("metric", "value"))
    for name, value in pairs:
        writer.writerow((name, value))
PY

printf '%s\n' 'Gate 4.1 M2A multiply tests complete.'
