#!/usr/bin/env bash
set -euo pipefail
ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
REPORT="$ROOT/reports/gate5_2_rvc/r2"
BUILD="$ROOT/tb/differential/gate5_2_r2_rtl_build"
TAG=${BOOM_R2_CSYNTH_TAG:-gate5_2_rvc_r2_repair}
RTL=${GATE5_2_R2_CORE_RTL_DIR:-"$ROOT/boom_hls_${TAG}_core_boom_core_top/solution_module/syn/verilog"}
XSIM_BUILD="$BUILD/xsim"
VITIS_HLS_BIN=${VITIS_HLS:-/home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls}
XVLOG_BIN=${XVLOG:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xvlog}
XELAB_BIN=${XELAB:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xelab}
XSIM_BIN=${XSIM:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xsim}
PROGRAMS=(rvc_addi rvc_load_store rvc_branch rvc_jump rvc_word_ops rvc_mixed_16_32 rvc_cross_boundary rvc_rv64m_mix rvc_redirect_halfword rvc_tohost rvc_decode_gaps)

mkdir -p "$REPORT/logs/r2_rtl" "$REPORT/r2_rtl_traces" "$BUILD" "$XSIM_BUILD"
bash "$ROOT/scripts/gate5_2/build_rvc_programs.sh" > "$REPORT/logs/r2_rtl/program_build.log" 2>&1
(
  if [[ ! -s "$RTL/boom_core_top.v" ]]; then
    BOOM_R2_CSYNTH_TAG="$TAG" VITIS_HLS="$VITIS_HLS_BIN" \
      "$ROOT/scripts/gate5_2/run_r2_canonical_csynth.sh"
  else
    printf 'Using canonical R2 boom_core_top RTL: %s\n' "$RTL"
  fi
) > "$REPORT/logs/r2_rtl/csynth.log" 2>&1
[[ -s "$RTL/boom_core_top.v" ]] || { printf 'canonical boom_core_top RTL unavailable at %s\n' "$RTL" >&2; exit 3; }
for source in "$ROOT"/src/*.cpp "$ROOT"/include/*.hpp \
              "$ROOT/scripts/generate_merged.sh" "$ROOT/scripts/create_project.tcl" \
              "$ROOT/scripts/module_csynth.tcl"; do
  case "$source" in
    */src/boom_core_merged.cpp|*/src/boom_all.cpp) continue ;;
  esac
  [[ ! "$source" -nt "$RTL/boom_core_top.v" ]] || {
    printf 'canonical boom_core_top RTL is older than source input: %s\n' "$source" >&2
    exit 3
  }
done
cp "$RTL"/*.dat "$XSIM_BUILD"/ 2>/dev/null || true
mapfile -t RTL_FILES < <(printf '%s\n' "$RTL"/*.v | sort)
(
  cd "$XSIM_BUILD"
  "$XVLOG_BIN" "${RTL_FILES[@]}"
  "$XVLOG_BIN" --sv "$ROOT/rtl_tb/axis_imem_model.sv" "$ROOT/rtl_tb/axis_dmem_model.sv" \
    "$ROOT/rtl_tb/commit_trace_monitor.sv" "$ROOT/tb/differential/gate5_2_r2_rtl_harness.sv" \
    "$ROOT/rtl_tb/boom_core_rtl_tb.sv"
  "$XELAB_BIN" boom_core_rtl_tb -s gate5_2_r2_snapshot -timescale 1ns/1ps
) > "$REPORT/logs/r2_rtl/xsim_build.log" 2>&1

for name in "${PROGRAMS[@]}"; do
  (
    cd "$XSIM_BUILD"
    "$XSIM_BIN" gate5_2_r2_snapshot --runall --onerror quit \
      --testplusarg "PROGRAM=$ROOT/tb/programs/rvc_fetch/build/$name.hex" \
      --testplusarg "PROGRAM_NAME=$name" --testplusarg "SCENARIO=R0_POWER_ON_RESET" \
      --testplusarg "MAX_CYCLES=1000000" \
      --testplusarg "TRACE=$REPORT/r2_rtl_traces/$name.jsonl" \
      --log "$REPORT/logs/r2_rtl/$name.log"
  ) > "$REPORT/logs/r2_rtl/$name.stdout.log" 2>&1
  grep -q 'GATE3_8_PASS scenario=R0_POWER_ON_RESET' "$REPORT/logs/r2_rtl/$name.log"
done

python3 - "$REPORT" "${PROGRAMS[@]}" <<'PY'
import csv, json, sys
from pathlib import Path
report = Path(sys.argv[1])
programs = sys.argv[2:]
expected = {
 "rvc_addi": {8:12,9:15}, "rvc_load_store": {9:46,10:46,11:47},
 "rvc_branch": {9:9,10:13}, "rvc_jump": {8:7,9:18},
 "rvc_word_ops": {8:12,10:0xfffffffffffffff8},
 "rvc_mixed_16_32": {8:19,9:13,10:30}, "rvc_cross_boundary": {8:10,9:21,10:31},
 "rvc_rv64m_mix": {10:42,11:8}, "rvc_redirect_halfword": {8:23,9:27},
 "rvc_tohost": {8:15,9:31}, "rvc_decode_gaps": {8:0xffffffff,9:2}}
rows=[]
for name in programs:
    records=[json.loads(x) for x in (report/'r2_rtl_traces'/f'{name}.jsonl').read_text().splitlines() if x]
    commits=[x for x in records if x.get('event')=='commit']
    checks=[]
    ok=True
    final={}
    for commit in commits:
        if commit.get('rd_valid'):
            final[commit.get('rd')]=int(commit['rd_value'],16)
    for rd,value in expected[name].items():
        hit=final.get(rd)==value
        checks.append(f'x{rd}=0x{value:016x}')
        ok &= hit
    tohost=any(x.get('event')=='tohost' and x.get('committed') and
               int(x['address'],16)==0x80000080 and int(x['value'],16)==1 for x in records)
    ok &= tohost
    rows.append((name,'PASS' if ok else 'FAIL',len(commits),';'.join(checks),'PASS' if tohost else 'FAIL',f'r2_rtl_traces/{name}.jsonl'))
with (report/'r2_full_core_rtl_matrix.csv').open('w',newline='') as f:
    w=csv.writer(f); w.writerow(('program','status','commits','signature','tohost','trace')); w.writerows(rows)
if sum(x[1]=='PASS' for x in rows)!=11: raise SystemExit('R3 full-core RTL signature check failed')
print('Gate 5.2 R3 XSim full-core mixed RVC: 11/11 PASS')
PY
