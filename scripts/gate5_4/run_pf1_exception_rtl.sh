#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)"}
: "${BOOM_BUILD_ROOT:=/tmp/boom_hls}"
RTL=${PF1_EXCEPTION_RTL_DIR:?set PF1_EXCEPTION_RTL_DIR to generated Verilog directory}
BUILD="$BOOM_BUILD_ROOT/gate5_4_product_integration/pf1/rtl"
REPORT="$ROOT/reports/gate5_4_product_integration/pf1"
XVLOG=${XVLOG:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xvlog}
XELAB=${XELAB:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xelab}
XSIM=${XSIM:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xsim}

mkdir -p -- "$BUILD" "$REPORT/logs"
rm -rf -- "$BUILD/xsim"
mkdir -p -- "$BUILD/xsim"
mapfile -t rtl_files < <(printf '%s\n' "$RTL"/*.v | sort)
(
    cd -- "$BUILD/xsim"
    "$XVLOG" "${rtl_files[@]}"
    "$XVLOG" --sv "$ROOT/rtl_tb/exception_recovery_rtl_tb.sv"
    "$XELAB" exception_recovery_rtl_tb -s pf1_exception_snapshot -timescale 1ns/1ps
) >"$REPORT/logs/pf1_exception_rtl_build.log" 2>&1
(
    cd -- "$BUILD/xsim"
    "$XSIM" pf1_exception_snapshot --runall --onerror quit \
        --log "$REPORT/logs/pf1_exception_rtl.log"
) >"$REPORT/logs/pf1_exception_rtl.stdout.log" 2>&1
grep -q 'PF1_EXCEPTION_RTL_PASS cases=64' "$REPORT/logs/pf1_exception_rtl.log"
printf '%s\n' 'PF1_EXCEPTION_RTL_PASS cases=64'
