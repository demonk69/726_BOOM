#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
BUILD=${GATE5_2_R2_BUILD_DIR:-"$ROOT/build/gate5_2_rvc/r2"}
REPORT=${GATE5_2_R2_REPORT_DIR:-"$ROOT/reports/gate5_2_rvc/r2"}
HLS_PROJECT=${GATE5_2_R2_HLS_PROJECT:-"$BUILD/hls_project"}
SOLUTION=solution_r2_rvc_frontend
RTL=${GATE5_2_R2_RTL_DIR:-"$HLS_PROJECT/$SOLUTION/syn/verilog"}
VITIS_HLS=${VITIS_HLS:-/home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls}
XVLOG=${XVLOG:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xvlog}
XELAB=${XELAB:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xelab}
XSIM=${XSIM:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xsim}
TCL="$ROOT/scripts/gate5_2/r2_rvc_frontend_csynth.tcl"
TB="$ROOT/rtl_tb/rvc_frontend_rtl_tb.sv"
TOP_RTL="$RTL/synth_r2_rvc_frontend_top.v"

for tool in "$VITIS_HLS" "$XVLOG" "$XELAB" "$XSIM"; do
    [[ -x "$tool" ]] || { printf 'ERROR: missing executable %s\n' "$tool" >&2; exit 2; }
done
for source in "$TCL" "$TB" "$ROOT/src/frontend.cpp" "$ROOT/src/rvc.cpp" \
              "$ROOT/src/decode.cpp" "$ROOT/src/divider.cpp" \
              "$ROOT/src/synth_module_tops.cpp"; do
    [[ -s "$source" ]] || { printf 'ERROR: missing R2 source %s\n' "$source" >&2; exit 2; }
done

mkdir -p "$BUILD" "$REPORT/logs" "$BUILD/xsim"
"$VITIS_HLS" -version > "$REPORT/logs/tool_versions.log" 2>&1
grep -q '2021.2' "$REPORT/logs/tool_versions.log" || {
    printf 'ERROR: R2 requires Vitis HLS 2021.2; see %s\n' "$REPORT/logs/tool_versions.log" >&2
    exit 2
}

generate=0
if [[ ! -s "$TOP_RTL" ]]; then
    generate=1
else
    for source in "$TCL" "$ROOT/src/frontend.cpp" "$ROOT/src/rvc.cpp" \
                   "$ROOT/src/decode.cpp" "$ROOT/src/divider.cpp" \
                   "$ROOT/src/synth_module_tops.cpp" "$ROOT"/include/*.hpp; do
        [[ "$source" -nt "$TOP_RTL" ]] && generate=1
    done
fi

if (( generate )); then
    HLS_BOOM_ROOT="$ROOT" GATE5_2_R2_HLS_PROJECT="$HLS_PROJECT" \
        "$VITIS_HLS" -f "$TCL" > "$REPORT/logs/vitis_hls.log" 2>&1
fi
[[ -s "$TOP_RTL" ]] || {
    printf 'ERROR: HLS did not generate %s\n' "$TOP_RTL" >&2
    exit 2
}

rm -rf "$BUILD/xsim"/*
mapfile -t RTL_FILES < <(printf '%s\n' "$RTL"/*.v | sort)
(( ${#RTL_FILES[@]} > 0 )) || { printf 'ERROR: no generated RTL in %s\n' "$RTL" >&2; exit 2; }
shopt -s nullglob
RTL_DATA=("$RTL"/*.dat)
if (( ${#RTL_DATA[@]} )); then
    cp "${RTL_DATA[@]}" "$BUILD/xsim/"
fi

(
    cd "$BUILD/xsim"
    "$XVLOG" "${RTL_FILES[@]}"
    "$XVLOG" --sv "$TB"
    "$XELAB" rvc_frontend_rtl_tb -s rvc_frontend_rtl_snapshot -timescale 1ns/1ps
) > "$REPORT/logs/xsim_build.log" 2>&1
(
    cd "$BUILD/xsim"
    "$XSIM" rvc_frontend_rtl_snapshot --runall --onerror quit \
        --log "$REPORT/logs/xsim.log"
) > "$REPORT/logs/xsim.stdout.log" 2>&1

grep -q 'GATE5_2_R2_RVC_FRONTEND_RTL_PASS cases=' "$REPORT/logs/xsim.log"
python3 - "$REPORT/logs/xsim.log" "$REPORT/rtl_test_matrix.csv" <<'PY'
import csv
import sys
from pathlib import Path

log, matrix = map(Path, sys.argv[1:])
rows = []
for line in log.read_text(errors="replace").splitlines():
    if "CASE_FAIL," in line:
        raise SystemExit(f"RTL case failure present: {line}")
    if "CASE_PASS," in line:
        name, requirement = line.split("CASE_PASS,", 1)[1].split(",", 1)
        rows.append((name.strip(), requirement.strip(), "PASS", str(log)))
names = [row[0] for row in rows]
if len(rows) < 40 or len(names) != len(set(names)):
    raise SystemExit(f"expected >=40 unique PASS cases, got rows={len(rows)} unique={len(set(names))}")
with matrix.open("w", newline="") as stream:
    writer = csv.writer(stream)
    writer.writerow(("case", "asserted_requirement", "status", "evidence"))
    writer.writerows(rows)
print(f"Gate 5.2 R2 focused generated RTL: {len(rows)}/{len(rows)} unique cases PASS.")
PY

printf 'RTL=%s\nLOG=%s\nMATRIX=%s\n' "$RTL" "$REPORT/logs/xsim.log" \
       "$REPORT/rtl_test_matrix.csv"
