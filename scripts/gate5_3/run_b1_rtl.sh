#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
BUILD="$ROOT/build/gate5_3_fetch_buffer/b1"
REPORT="$ROOT/reports/gate5_3_fetch_buffer/b1"
RTL="$BUILD/csynth/d8_auto/solution/syn/verilog"
TB="$ROOT/rtl_tb/fetch_buffer_rtl_tb.sv"
XVLOG=${XVLOG:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xvlog}
XELAB=${XELAB:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xelab}
XSIM=${XSIM:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xsim}
for tool in "$XVLOG" "$XELAB" "$XSIM"; do
    [[ -x "$tool" ]] || { printf 'ERROR: missing %s\n' "$tool" >&2; exit 2; }
done
[[ -s "$RTL/synth_fetch_buffer_top.v" ]] || {
    printf 'ERROR: run scripts/gate5_3/run_b1_csynth_sweep.sh first\n' >&2; exit 2;
}
mkdir -p "$BUILD/xsim" "$REPORT/logs"
rm -rf "$BUILD/xsim"/*
mapfile -t RTL_FILES < <(printf '%s\n' "$RTL"/*.v | sort)
(
    cd "$BUILD/xsim"
    "$XVLOG" "${RTL_FILES[@]}"
    "$XVLOG" --sv "$TB"
    "$XELAB" fetch_buffer_rtl_tb -s fetch_buffer_rtl_snapshot -timescale 1ns/1ps
) >"$REPORT/logs/rtl_build.log" 2>&1
(
    cd "$BUILD/xsim"
    "$XSIM" fetch_buffer_rtl_snapshot --runall --onerror quit \
        --log "$REPORT/logs/rtl_xsim.log"
) >"$REPORT/logs/rtl_xsim.stdout.log" 2>&1
grep -q 'GATE5_3_B1_FETCH_BUFFER_RTL_PASS cases=' "$REPORT/logs/rtl_xsim.log"
python3 - "$REPORT/logs/rtl_xsim.log" "$REPORT/rtl_test_matrix.csv" <<'PY'
import csv
import sys
from pathlib import Path

log, matrix = map(Path, sys.argv[1:])
rows = []
for line in log.read_text(errors="replace").splitlines():
    if "CASE_FAIL," in line:
        raise SystemExit(line)
    if "CASE_PASS," in line:
        name, requirement = line.split("CASE_PASS,", 1)[1].split(",", 1)
        rows.append((name.strip(), requirement.strip(), "PASS", str(log)))
if len(rows) < 9 or len({row[0] for row in rows}) != len(rows):
    raise SystemExit(f"insufficient unique RTL cases: {len(rows)}")
with matrix.open("w", newline="") as stream:
    writer = csv.writer(stream)
    writer.writerow(("case", "asserted_requirement", "status", "evidence"))
    writer.writerows(rows)
print(f"GATE5_3_B1_RTL_PASS cases={len(rows)}")
PY
