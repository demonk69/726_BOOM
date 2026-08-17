#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
BUILD="$ROOT/build/gate5_3_fetch_buffer/b2"
REPORT="$ROOT/reports/gate5_3_fetch_buffer/b2"
RTL="$BUILD/rtl_hls/solution_b2_integration/syn/verilog"
XVLOG=${XVLOG:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xvlog}
XELAB=${XELAB:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xelab}
XSIM=${XSIM:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xsim}
mkdir -p "$BUILD/xsim" "$REPORT/logs"
bash "$ROOT/scripts/generate_merged.sh" >"$REPORT/logs/integration_generate_merged.log"
/home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls \
  -f "$ROOT/scripts/gate5_3/b2_integration_csynth.tcl" \
  >"$REPORT/logs/integration_csynth.log" 2>&1
rm -rf "$BUILD/xsim"/*
cp "$RTL"/*.dat "$BUILD/xsim"/ 2>/dev/null || true
mapfile -t RTL_FILES < <(printf '%s\n' "$RTL"/*.v | sort)
(
  cd "$BUILD/xsim"
  "$XVLOG" "${RTL_FILES[@]}"
  "$XVLOG" --sv "$ROOT/rtl_tb/fetch_buffer_integration_rtl_tb.sv"
  "$XELAB" fetch_buffer_integration_rtl_tb -s b2_integration_snapshot -timescale 1ns/1ps
) >"$REPORT/logs/integration_rtl_build.log" 2>&1
(
  cd "$BUILD/xsim"
  "$XSIM" b2_integration_snapshot --runall --onerror quit \
    --log "$REPORT/logs/integration_rtl.log"
) >"$REPORT/logs/integration_rtl.stdout.log" 2>&1
grep -q 'GATE5_3_B2_FETCH_BUFFER_INTEGRATION_RTL_PASS cases=' "$REPORT/logs/integration_rtl.log"
python3 - "$REPORT/logs/integration_rtl.log" "$REPORT/rtl_test_matrix.csv" <<'PY'
import csv, sys
from pathlib import Path
log, matrix = map(Path, sys.argv[1:])
rows=[]
mandatory = {
    'upper_half_rvc_production',
    'rvc_pc_plus_2_start',
    'cross_word_carry_creation',
    'partial_cross_word_excluded_from_fetch_buffer',
    'faulted_cross_word_lower_half_start_pc',
}
for line in log.read_text(errors='replace').splitlines():
    if 'CASE_FAIL,' in line: raise SystemExit(line)
    if 'BLOCKED' in line: raise SystemExit(f'blocked RTL case is not permitted: {line}')
    if 'CASE_PASS,' in line:
        name, req = line.split('CASE_PASS,',1)[1].split(',',1)
        rows.append((name, req, 'PASS', str(log)))
if len(rows) < 30: raise SystemExit(f'insufficient RTL cases: {len(rows)}')
names = [row[0] for row in rows]
if len(names) != len(set(names)): raise SystemExit('RTL matrix case names are not unique')
for name in sorted(mandatory):
    if names.count(name) != 1: raise SystemExit(f'mandatory RTL case count for {name}: {names.count(name)}')
    marker = f'SEMANTIC_PASS,{name}'
    if log.read_text(errors='replace').count(marker) != 1:
        raise SystemExit(f'mandatory RTL marker count for {name} is not one')
with matrix.open('w',newline='') as f:
    w=csv.writer(f); w.writerow(('case','asserted_requirement','status','evidence')); w.writerows(rows)
print(f'GATE5_3_B2_INTEGRATION_RTL_PASS cases={len(rows)}')
PY
