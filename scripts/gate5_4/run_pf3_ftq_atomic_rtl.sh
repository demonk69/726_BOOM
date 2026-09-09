#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)"}
source "$ROOT/scripts/common/gate_workspace.sh"
gate_begin gate5_4_product_integration pf3
WORK=$(gate_build_dir focused_rtl)
trap 'gate_cleanup_success "$WORK"' EXIT

RTL=${PF3_FTQ_ATOMIC_RTL_DIR:-"/tmp/boom_hls/boom_hls_gate5_4_pf3_synth_pf3_ftq_atomic_top/solution_module/syn/verilog"}
XVLOG=${XVLOG:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xvlog}
XELAB=${XELAB:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xelab}
XSIM=${XSIM:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xsim}

[[ -s "$RTL/synth_pf3_ftq_atomic_top.v" ]]
mapfile -t RTL_FILES < <(printf '%s\n' "$RTL"/*.v | sort)
cp "$RTL"/*.dat "$WORK"/ 2>/dev/null || true
(
  cd "$WORK"
  "$XVLOG" "${RTL_FILES[@]}"
  "$XVLOG" --sv "$ROOT/rtl_tb/pf3_ftq_atomic_rtl_tb.sv"
  "$XELAB" pf3_ftq_atomic_rtl_tb -s pf3_ftq_atomic_snapshot -timescale 1ns/1ps
  "$XSIM" pf3_ftq_atomic_snapshot --runall --onerror quit --log "$WORK/xsim.log"
)
grep -q 'PF3_FTQ_ATOMIC_RTL_PASS cases=100' "$WORK/xsim.log"
