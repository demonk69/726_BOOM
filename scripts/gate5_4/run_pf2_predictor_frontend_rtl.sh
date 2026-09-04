#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)"}
source "$ROOT/scripts/common/gate_workspace.sh"
gate_begin gate5_4 pf2
WORK=$(gate_build_dir focused_rtl)
trap 'gate_cleanup_success "$WORK"' EXIT

RTL=${PF2_PREDICTOR_FRONTEND_RTL_DIR:-"/tmp/boom_hls/gate5_4/pf2/csynth_focused/boom_hls_gate5_4_pf2_focused_synth_pf2_predictor_frontend_top/solution_module/syn/verilog"}
XVLOG=${XVLOG:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xvlog}
XELAB=${XELAB:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xelab}
XSIM=${XSIM:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xsim}

[[ -s "$RTL/synth_pf2_predictor_frontend_top.v" ]]
mapfile -t RTL_FILES < <(printf '%s\n' "$RTL"/*.v | sort)
cp "$RTL"/*.dat "$WORK"/ 2>/dev/null || true
(
  cd "$WORK"
  "$XVLOG" "${RTL_FILES[@]}"
  "$XVLOG" --sv "$ROOT/rtl_tb/pf2_predictor_frontend_rtl_tb.sv"
  "$XELAB" pf2_predictor_frontend_rtl_tb -s pf2_predictor_frontend_snapshot \
    -timescale 1ns/1ps
  "$XSIM" pf2_predictor_frontend_snapshot --runall --onerror quit \
    --log "$WORK/xsim.log"
)
grep -Fxq 'PF2_PREDICTOR_FRONTEND_RTL_PASS cases=116' "$WORK/xsim.log"
