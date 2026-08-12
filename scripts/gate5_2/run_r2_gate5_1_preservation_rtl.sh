#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
REPORT="$ROOT/reports/gate5_2_rvc/r2"
BUILD="$ROOT/build/gate5_2_rvc/r2/gate5_1_preservation"
TAG=${BOOM_R2_VERIFY_TAG:-gate5_2_rvc_r2_repair_verify}
RTL=${GATE5_2_R2_GATE5_1_RTL_DIR:-"$ROOT/boom_hls_${TAG}_synth_frontend_verify_top/solution_module/syn/verilog"}
XVLOG=${XVLOG:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xvlog}
XELAB=${XELAB:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xelab}
XSIM=${XSIM:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xsim}

mkdir -p "$REPORT/logs/gate5_1_preservation" "$BUILD/xsim"
rm -rf "$BUILD/xsim"/*
TOP_RTL="$RTL/synth_frontend_verify_top.v"
generate=0
[[ -s "$TOP_RTL" ]] || generate=1
if (( ! generate )); then
  for source in "$ROOT"/src/*.cpp "$ROOT"/include/*.hpp \
                "$ROOT/scripts/generate_merged.sh" "$ROOT/scripts/create_project.tcl"; do
    case "$source" in
      */src/boom_core_merged.cpp|*/src/boom_all.cpp) continue ;;
    esac
    [[ "$source" -nt "$TOP_RTL" ]] && generate=1
  done
fi
if (( generate )); then
  BOOM_HLS_GATE="$TAG" "$ROOT/scripts/run_module_csynth.sh" synth_frontend_verify_top \
    > "$REPORT/logs/gate5_1_preservation/csynth.log" 2>&1
  python3 - "$ROOT/reports/$TAG/module_csynth_summary.csv" <<'PY'
import csv
import sys

with open(sys.argv[1], newline='') as stream:
    rows = list(csv.DictReader(stream))
if len(rows) != 1 or rows[0]['module'] != 'synth_frontend_verify_top' or rows[0]['status'] != 'PASS':
    raise SystemExit('frontend verification csynth did not pass')
PY
fi
[[ -s "$TOP_RTL" ]] || { printf 'missing current frontend verify RTL: %s\n' "$TOP_RTL" >&2; exit 2; }
for source in "$ROOT"/src/*.cpp "$ROOT"/include/*.hpp; do
  case "$source" in
    */src/boom_core_merged.cpp|*/src/boom_all.cpp) continue ;;
  esac
  [[ ! "$source" -nt "$TOP_RTL" ]] || {
    printf 'frontend verification RTL is older than source input: %s\n' "$source" >&2
    exit 2
  }
done
mapfile -t RTL_FILES < <(printf '%s\n' "$RTL"/*.v | sort)
shopt -s nullglob
RTL_DATA=("$RTL"/*.dat)
if (( ${#RTL_DATA[@]} )); then cp "${RTL_DATA[@]}" "$BUILD/xsim/"; fi
(
  cd "$BUILD/xsim"
  "$XVLOG" "${RTL_FILES[@]}"
  "$XVLOG" --sv "$ROOT/rtl_tb/frontend_verify_rtl_tb.sv"
  "$XELAB" frontend_verify_rtl_tb -s gate5_1_r2_preservation -timescale 1ns/1ps
) > "$REPORT/logs/gate5_1_preservation/xsim_build.log" 2>&1
(
  cd "$BUILD/xsim"
  "$XSIM" gate5_1_r2_preservation --runall --onerror quit \
    --log "$REPORT/logs/gate5_1_preservation/xsim.log"
) > "$REPORT/logs/gate5_1_preservation/xsim.stdout.log" 2>&1

grep -q 'FRONTEND_VERIFY_RTL_PASS cases=33' "$REPORT/logs/gate5_1_preservation/xsim.log"
printf '%s\n' 'Gate 5.1 focused generated-RTL preservation: 33/33 PASS.'
