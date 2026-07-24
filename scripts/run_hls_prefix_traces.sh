#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"}
OUT_DIR=${HLS_TRACE_OUT_DIR:-"$ROOT/reference/hls_traces"}
REPORT_DIR="$ROOT/reports/equivalence/provisional_gate3"
BUILD_BIN="$REPORT_DIR/hls_prefix_trace_tb"
CXX_BIN=${CXX:-g++}
VITIS_HLS_BIN=${VITIS_HLS:-vitis_hls}
TRACE_MODE=${HLS_TRACE_MODE:-prefix}

mkdir -p "$OUT_DIR" "$REPORT_DIR"

"$CXX_BIN" -std=c++11 -I"$ROOT/include" \
  "$ROOT/src/boom_core_step.cpp" \
  "$ROOT/src/frontend.cpp" \
  "$ROOT/src/decode.cpp" \
  "$ROOT/src/rename.cpp" \
  "$ROOT/src/rob.cpp" \
  "$ROOT/src/issue.cpp" \
  "$ROOT/src/execute.cpp" \
  "$ROOT/src/branch.cpp" \
  "$ROOT/src/lsu.cpp" \
  "$ROOT/src/commit.cpp" \
  "$ROOT/src/csr.cpp" \
  "$ROOT/tb/differential/hls_prefix_trace_tb.cpp" \
  -o "$BUILD_BIN"

HLS_PROJECT_ROOT="$ROOT" HLS_TRACE_OUT_DIR="$OUT_DIR" HLS_TRACE_SOURCE=hls_cpp HLS_TRACE_MODE="$TRACE_MODE" \
  "$BUILD_BIN" > "$REPORT_DIR/hls_cpp_trace.log" 2>&1

if ! command -v "$VITIS_HLS_BIN" >/dev/null 2>&1; then
  if [ -x /home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls ]; then
    VITIS_HLS_BIN=/home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls
  else
    echo "Vitis HLS not found" >&2
    exit 127
  fi
fi

FPGA_PART=${FPGA_PART:-xczu7ev-ffvc1156-2-e} \
CLOCK_PERIOD=${CLOCK_PERIOD:-10} \
HLS_PROJECT_ROOT="$ROOT" \
HLS_TRACE_OUT_DIR="$OUT_DIR" \
HLS_TRACE_SOURCE=hls_csim \
HLS_TRACE_MODE="$TRACE_MODE" \
  "$VITIS_HLS_BIN" -f "$ROOT/scripts/run_hls_prefix_trace_csim.tcl" \
  > "$REPORT_DIR/hls_csim_trace.log" 2>&1

echo "HLS C++ trace log: $REPORT_DIR/hls_cpp_trace.log"
echo "HLS csim trace log: $REPORT_DIR/hls_csim_trace.log"
echo "HLS traces: $OUT_DIR"
echo "HLS trace mode: $TRACE_MODE"
