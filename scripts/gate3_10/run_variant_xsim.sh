#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
VARIANT=${1:?usage: run_variant_xsim.sh VARIANT SCENARIO PROGRAM}
SCENARIO=${2:?missing scenario}
PROGRAM_NAME=${3:?missing program}
BUILD_DIR="$ROOT/build/gate3_10/$VARIANT/xsim"
OUT="$ROOT/reports/gate3_10/variants/$VARIANT"
PROGRAM="$ROOT/tb/programs/boom_reference/build/$PROGRAM_NAME.hex"
TRACE="$OUT/rtl_traces/${PROGRAM_NAME}_${SCENARIO}.jsonl"
LOG="$OUT/logs/${PROGRAM_NAME}_${SCENARIO}.log"
XSIM=${XSIM:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xsim}
mkdir -p "$OUT/rtl_traces" "$OUT/logs"
if [[ ! -d "$BUILD_DIR/xsim.dir/gate3_10_snapshot" ]]; then
  "$ROOT/scripts/gate3_10/build_variant_xsim.sh" "$VARIANT"
fi
CLEAN_PROGRAM="$BUILD_DIR/${PROGRAM_NAME}.readmemh"
: > "$CLEAN_PROGRAM"
while IFS= read -r line; do
  line=${line%%#*}; line=${line//[[:space:]]/}
  [[ -n "$line" ]] && printf '%s\n' "$line" >> "$CLEAN_PROGRAM"
done < "$PROGRAM"
(
  cd "$BUILD_DIR"
  "$XSIM" gate3_10_snapshot --runall --onerror quit \
    --testplusarg "PROGRAM=$CLEAN_PROGRAM" --testplusarg "PROGRAM_NAME=$PROGRAM_NAME" \
    --testplusarg "SCENARIO=$SCENARIO" --testplusarg "MAX_CYCLES=50000" \
    --testplusarg "TRACE=$TRACE" --log "$LOG"
)
if ! grep -Fq "GATE3_8_PASS scenario=$SCENARIO " "$LOG"; then
  exit 1
fi
