#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
RTL_DIR=${GATE3_9_RTL_DIR:-"$ROOT/reports/gate3_9/variants/F1_FINE_GRAIN_RESET/conservative_rtl"}
BUILD_DIR=${GATE3_9_XSIM_BUILD:-"$ROOT/build/gate3_9/xsim"}
XVLOG=${XVLOG:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xvlog}
XELAB=${XELAB:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xelab}

mkdir -p "$BUILD_DIR"
cp "$RTL_DIR"/*.dat "$BUILD_DIR"/

mapfile -t RTL_FILES < <(printf '%s\n' "$RTL_DIR"/*.v | sort)
SV_FILES=(
  "$ROOT/rtl_tb/axis_imem_model.sv"
  "$ROOT/rtl_tb/axis_dmem_model.sv"
  "$ROOT/rtl_tb/commit_trace_monitor.sv"
  "$ROOT/rtl_tb/gate3_9/boom_core_rtl_harness.sv"
  "$ROOT/rtl_tb/boom_core_rtl_tb.sv"
)

(
  cd "$BUILD_DIR"
  "$XVLOG" "${RTL_FILES[@]}"
  "$XVLOG" --sv "${SV_FILES[@]}"
  "$XELAB" boom_core_rtl_tb -s gate3_9_snapshot -timescale 1ns/1ps
)

printf '%s\n' "GATE3_9_XSIM_BUILD_PASS snapshot=$BUILD_DIR/gate3_9_snapshot"
