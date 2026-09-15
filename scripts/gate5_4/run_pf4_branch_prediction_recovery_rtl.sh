#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)"}
WORK=${PF4_WORK:-/tmp/boom_hls/pf4}
TOP=synth_pf4_branch_prediction_recovery_top
VITIS_HLS=${VITIS_HLS:-/home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls}
XVLOG=${XVLOG:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xvlog}
XELAB=${XELAB:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xelab}
XSIM=${XSIM:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xsim}
SENTINEL='PF4_BRANCH_PREDICTION_RECOVERY_RTL_PASS cases=140'

mkdir -p -- "$(dirname -- "$WORK")"
rm -rf -- "$WORK"
mkdir -p -- "$WORK/sim"

if [[ ! -x "$VITIS_HLS" ]]; then
    printf 'Vitis HLS not found: %s\n' "$VITIS_HLS" >&2
    exit 127
fi

HLS_BOOM_ROOT="$ROOT" PF4_WORK="$WORK" \
    "$VITIS_HLS" -f "$ROOT/scripts/gate5_4/pf4_branch_prediction_recovery_csynth.tcl" \
    >"$WORK/vitis_hls.log" 2>&1

RTL="$WORK/hls_project/solution/syn/verilog"
[[ -s "$RTL/$TOP.v" ]] || {
    printf 'missing current-source generated RTL: %s\n' "$RTL/$TOP.v" >&2
    exit 1
}
mapfile -t RTL_FILES < <(printf '%s\n' "$RTL"/*.v | sort)
cp -- "$RTL"/*.dat "$WORK/sim/" 2>/dev/null || true

(
    cd -- "$WORK/sim"
    "$XVLOG" "${RTL_FILES[@]}" >"$WORK/xvlog_rtl.log" 2>&1
    "$XVLOG" --sv "$ROOT/rtl_tb/pf4_branch_prediction_recovery_rtl_tb.sv" \
        >"$WORK/xvlog_tb.log" 2>&1
    "$XELAB" pf4_branch_prediction_recovery_rtl_tb \
        -s pf4_branch_prediction_recovery_snapshot -timescale 1ns/1ps \
        >"$WORK/xelab.log" 2>&1
    "$XSIM" pf4_branch_prediction_recovery_snapshot --runall --onerror quit \
        --log "$WORK/xsim.log" >"$WORK/xsim.stdout.log" 2>&1
)

grep -Fxq "$SENTINEL" "$WORK/xsim.log"
printf '%s\n' "$SENTINEL"
