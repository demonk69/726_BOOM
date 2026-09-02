#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
REPORT="$ROOT/reports/gate5_4_predictor/p2"
BUILD="$ROOT/build/gate5_4_predictor/p2/rtl"
RTL="$ROOT/boom_hls_gate5_4_p2_lutram_synth_predictor_foundation_top/solution_module/syn/verilog"
XVLOG=${XVLOG:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xvlog}
XELAB=${XELAB:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xelab}
XSIM=${XSIM:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xsim}

if [ ! -f "$RTL/synth_predictor_foundation_top.v" ]; then
  "$ROOT/scripts/gate5_4/run_p2_predictor_csynth.sh"
fi
mkdir -p "$BUILD" "$REPORT/logs/rtl"
cp "$RTL"/*.dat "$BUILD"/

mapfile -t RTL_FILES < <(printf '%s\n' "$RTL"/*.v)
pushd "$BUILD" >/dev/null
"$XVLOG" --sv "$ROOT/rtl_tb/predictor_foundation_rtl_tb.sv" \
  "${RTL_FILES[@]}" >"$REPORT/logs/rtl/xvlog.log" 2>&1
"$XELAB" predictor_foundation_rtl_tb -s predictor_foundation_rtl_sim \
  >"$REPORT/logs/rtl/xelab.log" 2>&1
"$XSIM" predictor_foundation_rtl_sim -R \
  >"$REPORT/logs/rtl/xsim.log" 2>&1
popd >/dev/null
grep -q GATE5_4_P2_PREDICTOR_RTL_PASS "$REPORT/logs/rtl/xsim.log"
printf '%s\n' GATE5_4_P2_PREDICTOR_RTL_PASS
