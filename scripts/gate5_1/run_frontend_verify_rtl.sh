#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
REPORT="$ROOT/reports/gate5_1_frontend/repair/r2"
BUILD=${GATE5_1_R2_RTL_BUILD_DIR:-"$ROOT/build/gate5_1_frontend/repair/r2/rtl"}
RTL=${GATE5_1_R2_RTL_DIR:-"$ROOT/boom_hls_gate5_1_frontend_repair_r2_synth_frontend_verify_top/solution_module/syn/verilog"}
XVLOG=${XVLOG:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xvlog}
XELAB=${XELAB:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xelab}
XSIM=${XSIM:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xsim}

mkdir -p "$REPORT/logs" "$BUILD/xsim"
rm -rf "$BUILD/xsim"/*
mapfile -t RTL_FILES < <(printf '%s\n' "$RTL"/*.v | sort)
mapfile -t RTL_DATA < <(printf '%s\n' "$RTL"/*.dat | sort)
cp "${RTL_DATA[@]}" "$BUILD/xsim/"
(
  cd "$BUILD/xsim"
  "$XVLOG" "${RTL_FILES[@]}"
  "$XVLOG" --sv "$ROOT/rtl_tb/frontend_verify_rtl_tb.sv"
  "$XELAB" frontend_verify_rtl_tb -s frontend_verify_rtl_snapshot -timescale 1ns/1ps
) > "$REPORT/logs/xsim_build.log" 2>&1
(
  cd "$BUILD/xsim"
  "$XSIM" frontend_verify_rtl_snapshot --runall --onerror quit --log "$REPORT/logs/xsim.log"
) > "$REPORT/logs/xsim.stdout.log" 2>&1

grep -q 'FRONTEND_VERIFY_RTL_PASS cases=33' "$REPORT/logs/xsim.log"
python3 - "$REPORT/logs/xsim.log" "$REPORT/rtl_test_matrix.csv" <<'PY'
import csv
import sys
from pathlib import Path

log, output = map(Path, sys.argv[1:])
rows = []
for line in log.read_text(errors="replace").splitlines():
    if "CASE_PASS," in line:
        name, requirement = line.split("CASE_PASS,", 1)[1].split(",", 1)
        rows.append((name.strip(), requirement.strip(), "PASS", str(log)))
if len(rows) != 33 or len({row[0] for row in rows}) != 33:
    raise SystemExit(f"expected 33 unique PASS cases, got {len(rows)} rows")
with output.open("w", newline="") as stream:
    writer = csv.writer(stream)
    writer.writerow(("case", "asserted_requirement", "status", "evidence"))
    writer.writerows(rows)
PY

printf '%s\n' 'Gate 5.1R R2 focused generated RTL: 33/33 PASS.'
