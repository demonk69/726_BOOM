#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)"}
WORK=${PF5_WORK:-/tmp/boom_hls/pf5/focused_rtl}
TOP=synth_pf5_commit_bim_training_top
VITIS_HLS=${VITIS_HLS:-/home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls}
XVLOG=${XVLOG:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xvlog}
XELAB=${XELAB:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xelab}
XSIM=${XSIM:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xsim}
SENTINEL='PF5_COMMIT_BIM_TRAINING_RTL_PASS cases=160'

mkdir -p -- "$(dirname -- "$WORK")"
rm -rf -- "$WORK"
mkdir -p -- "$WORK/sim"
if [[ ${GATE5_4_VERIFY_MERGED_ONLY:-0} != 1 ]]; then
    "$ROOT/scripts/generate_merged.sh"
fi
HLS_BOOM_ROOT="$ROOT" PF5_WORK="$WORK" \
    "$VITIS_HLS" -f "$ROOT/scripts/gate5_4/pf5_commit_bim_training_csynth.tcl" \
    >"$WORK/vitis_hls.log" 2>&1

RTL="$WORK/hls_project/solution/syn/verilog"
[[ -s "$RTL/$TOP.v" ]]
mapfile -t RTL_FILES < <(printf '%s\n' "$RTL"/*.v | sort)
cp -- "$RTL"/*.dat "$WORK/sim/" 2>/dev/null || true
(
    cd -- "$WORK/sim"
    "$XVLOG" "${RTL_FILES[@]}" >"$WORK/xvlog_rtl.log" 2>&1
    "$XVLOG" --sv "$ROOT/rtl_tb/pf5_commit_bim_training_rtl_tb.sv" \
        >"$WORK/xvlog_tb.log" 2>&1
    "$XELAB" pf5_commit_bim_training_rtl_tb \
        -s pf5_commit_bim_training_snapshot -timescale 1ns/1ps \
        >"$WORK/xelab.log" 2>&1
    "$XSIM" pf5_commit_bim_training_snapshot --runall --onerror quit \
        --log "$WORK/xsim.log" >"$WORK/xsim.stdout.log" 2>&1
)
[[ $(grep -Fxc "$SENTINEL" "$WORK/xsim.log") -eq 1 ]]
printf '%s\n' "$SENTINEL"
