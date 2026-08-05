#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
BUILD_ROOT=${BOOM_M3B_RTL_BUILD_DIR:-"$ROOT/build/gate4_1/m3b_divider_rtl"}
REPORT_DIR=${BOOM_M3B_RTL_REPORT_DIR:-"$ROOT/reports/gate4_1/m3/m3b/rtl"}
VITIS_HLS_BIN=${VITIS_HLS:-/home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls}
XVLOG=${XVLOG:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xvlog}
XELAB=${XELAB:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xelab}
XSIM=${XSIM:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xsim}
TOP=synth_m3b_divider_core_top
SOLUTION=solution_gate4_1_m3b_rtl
START_SECONDS=$SECONDS

for tool in "$VITIS_HLS_BIN" "$XVLOG" "$XELAB" "$XSIM"; do
    [[ -x "$tool" ]] || { printf 'ERROR: required tool is not executable: %s\n' "$tool" >&2; exit 2; }
done
if ! "$VITIS_HLS_BIN" -version 2>&1 | grep -q 'v2021\.2'; then
    printf '%s\n' 'ERROR: Vitis HLS 2021.2 is required.' >&2
    exit 2
fi
if ! "$XVLOG" -version 2>&1 | grep -q 'v2021\.2'; then
    printf '%s\n' 'ERROR: Vivado/XSim 2021.2 is required.' >&2
    exit 2
fi

rm -rf "$BUILD_ROOT" "$REPORT_DIR"
mkdir -p "$BUILD_ROOT/projects" "$BUILD_ROOT/xsim" "$REPORT_DIR/logs" \
    "$REPORT_DIR/csynth_reports"

"$ROOT/scripts/generate_merged.sh" > "$REPORT_DIR/logs/generate_merged.log" 2>&1
(
    cd "$BUILD_ROOT/projects"
    BOOM_HLS_GATE=gate4_1_m3b_rtl BOOM_HLS_TOP="$TOP" \
        BOOM_HLS_PROJECT="$TOP" BOOM_HLS_SOLUTION="$SOLUTION" \
        BOOM_HLS_CFLAGS_EXTRA= \
        FPGA_PART=xczu7ev-ffvc1156-2-e CLOCK_PERIOD=10 \
        "$VITIS_HLS_BIN" -f "$ROOT/scripts/run_top_csynth.tcl"
) > "$REPORT_DIR/logs/csynth.log" 2>&1

PROJECT="$BUILD_ROOT/projects/$TOP/$SOLUTION"
RTL_DIR="$PROJECT/syn/verilog"
[[ -s "$RTL_DIR/$TOP.v" ]] || {
    printf 'ERROR: canonical top RTL was not generated: %s\n' "$RTL_DIR/$TOP.v" >&2
    exit 1
}
cp "$PROJECT/syn/report"/* "$REPORT_DIR/csynth_reports/"
cp "$RTL_DIR"/*.dat "$BUILD_ROOT/xsim/" 2>/dev/null || true
mapfile -t RTL_FILES < <(printf '%s\n' "$RTL_DIR"/*.v | sort)
(
    cd "$BUILD_ROOT/xsim"
    "$XVLOG" "${RTL_FILES[@]}"
    "$XVLOG" --sv "$ROOT/rtl_tb/divider_core_rtl_tb.sv"
    "$XELAB" divider_core_rtl_tb -s divider_core_rtl_snapshot -timescale 1ns/1ps
) > "$REPORT_DIR/logs/xsim_build.log" 2>&1
(
    cd "$BUILD_ROOT/xsim"
    "$XSIM" divider_core_rtl_snapshot --runall --onerror quit \
        --log "$REPORT_DIR/logs/xsim.log"
) > "$REPORT_DIR/logs/xsim.stdout.log" 2>&1

grep -q 'M3B_DIVIDER_RTL_PASS cases=26' "$REPORT_DIR/logs/xsim.log"
python3 - "$REPORT_DIR/logs/xsim.log" "$REPORT_DIR/rtl_test_matrix.csv" <<'PY'
import csv
import sys
from pathlib import Path

log, matrix = map(Path, sys.argv[1:])
rows = []
for line in log.read_text(errors="replace").splitlines():
    marker = "CASE_PASS,"
    if marker not in line:
        continue
    payload = line.split(marker, 1)[1]
    name, requirement = payload.split(",", 1)
    rows.append((name.strip(), requirement.strip(), "PASS", str(log)))
if len(rows) != 26 or len({row[0] for row in rows}) != 26:
    raise SystemExit(f"expected 26 unique asserted CASE_PASS records, found {len(rows)}")
with matrix.open("w", newline="") as stream:
    writer = csv.writer(stream)
    writer.writerow(("case", "asserted_requirement", "status", "log"))
    writer.writerows(rows)
PY

runtime=$((SECONDS - START_SECONDS))
printf 'runtime_seconds,%s\n' "$runtime" > "$REPORT_DIR/runtime.csv"
printf 'Gate 4.1 M3B generated divider RTL: 26/26 PASS in %ss.\n' "$runtime"
