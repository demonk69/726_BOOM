#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
RTL=${GATE5_1_THROUGHPUT_RTL_DIR:?set GATE5_1_THROUGHPUT_RTL_DIR}
BUILD=${GATE5_1_THROUGHPUT_BUILD_DIR:-"$ROOT/build/gate5_1_frontend/repair/r3/throughput"}
REPORT="$ROOT/reports/gate5_1_frontend/repair/r3/throughput_rtl"
XVLOG=${XVLOG:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xvlog}
XELAB=${XELAB:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xelab}
XSIM=${XSIM:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xsim}

mkdir -p "$BUILD/xsim" "$REPORT"
rm -rf "$BUILD/xsim"/*
mapfile -t RTL_FILES < <(printf '%s\n' "$RTL"/*.v | sort)
(
  cd "$BUILD/xsim"
  "$XVLOG" "${RTL_FILES[@]}"
  "$XVLOG" --sv "$ROOT/rtl_tb/frontend_throughput_rtl_tb.sv"
  "$XELAB" frontend_throughput_rtl_tb -s frontend_throughput_rtl_snapshot -timescale 1ns/1ps
) > "$REPORT/xsim_build.log" 2>&1

for latency in 0 1 3 4; do
  (
    cd "$BUILD/xsim"
    "$XSIM" frontend_throughput_rtl_snapshot --runall --onerror quit \
      --testplusarg "LATENCY=$latency" --log "$REPORT/latency_${latency}.log"
  ) > "$REPORT/latency_${latency}.stdout.log" 2>&1
  grep -q 'THROUGHPUT_PASS' "$REPORT/latency_${latency}.log"
done

printf '%s\n' 'Frontend generated-RTL throughput sweep passed.'
