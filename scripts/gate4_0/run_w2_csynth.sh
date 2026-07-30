#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
VITIS_HLS_BIN=${VITIS_HLS:-vitis_hls}
BUILD_ROOT="$ROOT/build/gate4_0/w2_hls"
REPORT_ROOT="$ROOT/reports/gate4_0/w2/csynth"

if ! command -v "$VITIS_HLS_BIN" >/dev/null 2>&1; then
  VITIS_HLS_BIN=/home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls
fi
mkdir -p "$BUILD_ROOT" "$REPORT_ROOT"

run_top() {
  local top=$1
  local project_name="boom_hls_gate4_0_w2_$top"
  local project="$ROOT/$project_name"
  local solution=solution_baseline
  local report="$REPORT_ROOT/$top"
  mkdir -p "$report/rtl"
  BOOM_HLS_GATE=gate4_0_w2 BOOM_HLS_TOP="$top" BOOM_HLS_PROJECT="$project_name" \
    BOOM_HLS_SOLUTION="$solution" FPGA_PART=${FPGA_PART:-xczu7ev-ffvc1156-2-e} \
    CLOCK_PERIOD=${CLOCK_PERIOD:-10} \
    /usr/bin/time -f 'runtime_seconds=%e\npeak_memory_kb=%M' -o "$report/csynth.time" \
    "$VITIS_HLS_BIN" -f "$ROOT/scripts/run_top_csynth.tcl" > "$report/csynth.log" 2>&1
  cp "$project/$solution/syn/report/${top}_csynth.rpt" "$report/${top}_csynth.rpt"
  cp "$project/$solution/syn/report/${top}_csynth.xml" "$report/${top}_csynth.xml"
  cp "$project/$solution/syn/verilog"/* "$report/rtl"/
  cp "$project/$solution/${solution}.log" "$report/${solution}.log"
}

run_top synth_issue_top
run_top synth_core_step_top
run_top boom_core_top

printf '%s\n' 'Gate 4.0 W2 module and conservative synthesis complete.'
