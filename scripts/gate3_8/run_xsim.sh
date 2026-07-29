#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
BUILD_DIR=${GATE3_8_XSIM_BUILD:-"$ROOT/build/gate3_8/xsim"}
XSIM=${XSIM:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xsim}
SCENARIO=${1:-R0_POWER_ON_RESET}
PROGRAM_NAME=${2:-independent_alu}
MAX_CYCLES=${MAX_CYCLES:-40000}
PROGRAM=${PROGRAM:-"$ROOT/tb/programs/boom_reference/build/$PROGRAM_NAME.hex"}
TRACE=${TRACE:-"$ROOT/reference/rtl_traces/${PROGRAM_NAME}_${SCENARIO}.jsonl"}
LOG=${LOG:-"$ROOT/reports/gate3_8/logs/${PROGRAM_NAME}_${SCENARIO}.log"}

if [[ ! -d "$BUILD_DIR/xsim.dir/gate3_8_snapshot" ]]; then
  "$ROOT/scripts/gate3_8/build_xsim.sh"
fi
if [[ ! -f "$PROGRAM" ]]; then
  printf 'missing program image: %s\n' "$PROGRAM" >&2
  exit 2
fi

mkdir -p "$(dirname "$TRACE")" "$(dirname "$LOG")"
CLEAN_PROGRAM="$BUILD_DIR/${PROGRAM_NAME}.readmemh"
: > "$CLEAN_PROGRAM"
while IFS= read -r line; do
  line=${line%%#*}
  line=${line//[[:space:]]/}
  if [[ -n "$line" ]]; then
    printf '%s\n' "$line" >> "$CLEAN_PROGRAM"
  fi
done < "$PROGRAM"
if (
  cd "$BUILD_DIR"
  "$XSIM" gate3_8_snapshot --runall --onerror quit \
    --testplusarg "PROGRAM=$CLEAN_PROGRAM" \
    --testplusarg "PROGRAM_NAME=$PROGRAM_NAME" \
    --testplusarg "SCENARIO=$SCENARIO" \
    --testplusarg "MAX_CYCLES=$MAX_CYCLES" \
    --testplusarg "TRACE=$TRACE" \
    --log "$LOG"
); then
  xsim_status=0
else
  xsim_status=$?
fi

if ((xsim_status != 0)); then
  exit "$xsim_status"
fi
if ! grep -Fq "GATE3_8_PASS scenario=$SCENARIO " "$LOG"; then
  printf 'xsim completed without a Gate 3.8 PASS marker: %s\n' "$LOG" >&2
  exit 1
fi
