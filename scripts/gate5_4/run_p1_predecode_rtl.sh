#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
REPORT="$ROOT/reports/gate5_4_predictor/p1"
RTL_SINGLE="$ROOT/boom_hls_gate5_4_p1_synth_predecode_top/solution_module/syn/verilog"
RTL_PACKET="$ROOT/boom_hls_gate5_4_p1_synth_predecode_packet_top/solution_module/syn/verilog"
mkdir -p "$REPORT/logs/rtl"

if [ ! -f "$RTL_SINGLE/synth_predecode_top.v" ] || \
   [ ! -f "$RTL_PACKET/synth_predecode_packet_top.v" ]; then
  "$ROOT/scripts/gate5_4/run_p1_predecode_csynth.sh"
fi

xvlog --sv "$ROOT/rtl_tb/predecode_rtl_tb.sv" \
  "$RTL_SINGLE/synth_predecode_top.v" \
  "$RTL_SINGLE/synth_predecode_top_predecode_cfi.v" \
  "$RTL_PACKET/synth_predecode_packet_top.v" \
  "$RTL_PACKET/synth_predecode_packet_top_predecode_cfi.v" \
  >"$REPORT/logs/rtl/xvlog.log" 2>&1
xelab predecode_rtl_tb -s predecode_rtl_sim \
  >"$REPORT/logs/rtl/xelab.log" 2>&1
xsim predecode_rtl_sim -R >"$REPORT/logs/rtl/xsim.log" 2>&1
grep -q 'GATE5_4_P1_PREDECODE_RTL_PASS' "$REPORT/logs/rtl/xsim.log"
grep 'PREDECODE_RTL' "$REPORT/logs/rtl/xsim.log"
printf '%s\n' 'GATE5_4_P1_PREDECODE_RTL_PASS'
