#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
VARIANT=${1:?usage: run_local_variant.sh VARIANT [CLOCK_PERIOD]}
PERIOD=${2:-10}
REPORT_DIR="$ROOT/reports/gate3_10/variants/$VARIANT"
PROJECT="$ROOT/build/gate3_10/$VARIANT/hls_project/solution_local"
VITIS_HLS_BIN=${VITIS_HLS:-/home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls}
case "$VARIANT" in
  R1_RESET_INIT_PIPELINE|SWEEP_R1_*) CFLAGS="-DBOOM_HLS_GATE3_10_R1_RESET_ROB_PIPELINE" ;;
  P0_GATE3_9_BASELINE|P0_*|SWEEP_P0_*) CFLAGS="" ;;
  *) printf 'unsupported Gate 3.10 variant: %s\n' "$VARIANT" >&2; exit 2 ;;
esac
mkdir -p "$REPORT_DIR/logs" "$REPORT_DIR/conservative_rtl" "$ROOT/build/gate3_10/$VARIANT"
(
  cd "$ROOT/build/gate3_10/$VARIANT"
  GATE3_10_VARIANT="$VARIANT" BOOM_HLS_CFLAGS_EXTRA="$CFLAGS" \
    FPGA_PART=${FPGA_PART:-xczu7ev-ffvc1156-2-e} CLOCK_PERIOD="$PERIOD" \
    /usr/bin/time -f 'runtime_seconds=%e\npeak_memory_kb=%M' -o "$REPORT_DIR/csynth.time" \
    "$VITIS_HLS_BIN" -f "$ROOT/scripts/gate3_10/local_pipeline_csynth.tcl" \
    > "$REPORT_DIR/logs/csynth.log" 2>&1
)
cp "$PROJECT/syn/report/boom_core_top_csynth.rpt" "$REPORT_DIR/boom_core_top_csynth.rpt"
cp "$PROJECT/syn/report/boom_core_top_csynth.xml" "$REPORT_DIR/boom_core_top_csynth.xml"
cp "$PROJECT/syn/report"/boom_core_reset_step*_csynth.rpt "$REPORT_DIR/"
cp "$PROJECT/syn/verilog"/* "$REPORT_DIR/conservative_rtl/"
cp "$PROJECT/.autopilot/db/boom_core_reset_step.verbose.sched.rpt" "$REPORT_DIR/"
printf 'GATE3_10_CSYNTH_PASS variant=%s period=%s\n' "$VARIANT" "$PERIOD"
