#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
VITIS_HLS_BIN=${VITIS_HLS:-vitis_hls}
PROJECT="$ROOT/boom_hls_gate3_9_baseline"
SOLUTION="$PROJECT/solution_baseline"
REPORT_DIR="$ROOT/reports/gate3_9/variants/F1_FINE_GRAIN_RESET"
RTL_DIR="$REPORT_DIR/conservative_rtl"

if ! command -v "$VITIS_HLS_BIN" >/dev/null 2>&1; then
  VITIS_HLS_BIN=/home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls
fi
mkdir -p "$REPORT_DIR" "$RTL_DIR"

BOOM_HLS_GATE=gate3_9 FPGA_PART=${FPGA_PART:-xczu7ev-ffvc1156-2-e} CLOCK_PERIOD=${CLOCK_PERIOD:-10} \
  /usr/bin/time -f 'runtime_seconds=%e\npeak_memory_kb=%M' -o "$REPORT_DIR/csynth.time" \
  "$VITIS_HLS_BIN" -f "$ROOT/scripts/run_baseline_csynth.tcl" > "$REPORT_DIR/csynth.log" 2>&1

cp "$SOLUTION/syn/report/boom_core_top_csynth.rpt" "$REPORT_DIR/boom_core_top_csynth.rpt"
cp "$SOLUTION/syn/report/boom_core_top_csynth.xml" "$REPORT_DIR/boom_core_top_csynth.xml"
cp "$SOLUTION/syn/verilog"/* "$RTL_DIR"/
cp "$SOLUTION/solution_baseline.log" "$REPORT_DIR/solution_baseline.log"

printf '%s\n' "GATE3_9_FINE_GRAIN_CSYNTH_PASS report=$REPORT_DIR/boom_core_top_csynth.rpt"
