#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)"}
: "${BOOM_BUILD_ROOT:=/tmp/boom_hls}"
RTL=${PF1_FULL_CORE_RTL_DIR:?set PF1_FULL_CORE_RTL_DIR to generated boom_core_top Verilog directory}
BUILD="$BOOM_BUILD_ROOT/gate5_4_product_integration/pf1/full_core_rtl"
REPORT="$ROOT/reports/gate5_4_product_integration/pf1"
mkdir -p -- "$BUILD" "$REPORT/logs"
rm -rf -- "$BUILD/xsim"
mkdir -p -- "$BUILD/xsim"
mapfile -t rtl_files < <(printf '%s\n' "$RTL"/*.v | sort)
(
    cd -- "$BUILD/xsim"
    cp "$RTL"/*.dat . 2>/dev/null || true
    /home/lab_726/Xilinx/Vivado/2021.2/bin/xvlog "${rtl_files[@]}"
    /home/lab_726/Xilinx/Vivado/2021.2/bin/xvlog --sv "$ROOT/rtl_tb/exception_recovery_full_core_rtl_tb.sv"
    /home/lab_726/Xilinx/Vivado/2021.2/bin/xelab exception_recovery_full_core_rtl_tb \
        -s pf1_full_core_snapshot -timescale 1ns/1ps
) >"$REPORT/logs/pf1_full_core_rtl_build.log" 2>&1
for scenario in {0..7}; do
    (
        cd -- "$BUILD/xsim"
        /home/lab_726/Xilinx/Vivado/2021.2/bin/xsim pf1_full_core_snapshot --runall \
            --onerror quit --testplusarg "SCENARIO=$scenario" \
            --log "$REPORT/logs/pf1_full_core_rtl_${scenario}.log"
    ) >"$REPORT/logs/pf1_full_core_rtl_${scenario}.stdout.log" 2>&1
    grep -q "PF1_FULL_CORE_RTL_PASS scenario=$scenario" \
        "$REPORT/logs/pf1_full_core_rtl_${scenario}.log"
done
printf '%s\n' 'PF1_FULL_CORE_RTL_PASS cases=8'
