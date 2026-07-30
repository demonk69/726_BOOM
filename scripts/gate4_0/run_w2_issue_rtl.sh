#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
RTL_DIR="$ROOT/reports/gate4_0/w2/csynth/synth_issue_top/rtl"
BUILD_DIR="$ROOT/build/gate4_0/w2_issue_xsim"
REPORT_DIR="$ROOT/reports/gate4_0/w2/issue_rtl"
XVLOG=${XVLOG:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xvlog}
XELAB=${XELAB:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xelab}
XSIM=${XSIM:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xsim}

mkdir -p "$BUILD_DIR" "$REPORT_DIR/logs"
cp "$RTL_DIR"/*.dat "$BUILD_DIR"/
mapfile -t RTL_FILES < <(printf '%s\n' "$RTL_DIR"/*.v | sort)
(
  cd "$BUILD_DIR"
  "$XVLOG" "${RTL_FILES[@]}"
  "$XVLOG" --sv "$ROOT/rtl_tb/gate4_0/w2_issue_select_tb.sv"
  "$XELAB" w2_issue_select_tb -s w2_issue_snapshot -timescale 1ns/1ps
) > "$REPORT_DIR/build.log" 2>&1

CASES=(
  "dual_ready:15:1:2:1:1:1:1:1:0:2:1"
  "mem_busy_int_accept:11:1:2:1:1:1:0:1:1:1:2"
  "int_busy_mem_accept:7:1:2:1:1:1:1:1:0:2:1"
  "both_busy_retain:3:2:2:0:2:1:0:1:0:1:0"
  "branch_kill_mem:31:0:1:1:0:0:0:1:1:0:2"
)

printf '%s\n' 'case,status,observable' > "$REPORT_DIR/results.csv"
for item in "${CASES[@]}"; do
  IFS=: read -r name seed count generated accepted retained mem_valid mem_accepted \
    int_valid int_accepted survivor issued <<< "$item"
  log="$REPORT_DIR/logs/$name.log"
  (
    cd "$BUILD_DIR"
    "$XSIM" w2_issue_snapshot --runall --onerror quit \
      --testplusarg "SEED=$seed" --testplusarg "COUNT=$count" \
      --testplusarg "GENERATED=$generated" --testplusarg "ACCEPTED=$accepted" \
      --testplusarg "RETAINED=$retained" --testplusarg "MEM_VALID=$mem_valid" \
      --testplusarg "MEM_ACCEPTED=$mem_accepted" --testplusarg "INT_VALID=$int_valid" \
      --testplusarg "INT_ACCEPTED=$int_accepted" --testplusarg "SURVIVOR_ROB=$survivor" \
      --testplusarg "ISSUED_ROB=$issued" --log "$log"
  ) > "$REPORT_DIR/logs/$name.stdout.log" 2>&1
  observable=$(python3 - "$log" <<'PY'
import re
import sys
from pathlib import Path
text = Path(sys.argv[1]).read_text(errors="replace")
match = re.search(r"W2_RTL_PASS observable=([0-9a-fA-F]+)", text)
if not match:
    raise SystemExit(1)
print(match.group(1))
PY
  )
  printf '%s,PASS,%s\n' "$name" "$observable" >> "$REPORT_DIR/results.csv"
done

printf '%s\n' 'Gate 4.0 W2 issue-selection RTL 5/5 PASS.'
