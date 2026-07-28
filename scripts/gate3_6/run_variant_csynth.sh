#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
VARIANT=${1:?variant required}
shift
TOPS=("$@")
if [ "${#TOPS[@]}" -eq 0 ]; then TOPS=(boom_core_top); fi
REPORT_DIR="$ROOT/reports/gate3_6/variants/$VARIANT"
LOG_DIR="$REPORT_DIR/logs"
SUMMARY="$REPORT_DIR/csynth_summary.csv"
VITIS_HLS_BIN=${VITIS_HLS:-vitis_hls}

if ! command -v "$VITIS_HLS_BIN" >/dev/null 2>&1; then VITIS_HLS_BIN=/home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls; fi
mkdir -p "$LOG_DIR"
if [ "${GATE36_APPEND:-0}" != "1" ]; then
  printf 'top,status,runtime_seconds,peak_memory_kb,estimated_period_ns,lut,ff,bram_18k,dsp,automatic_partition_count,warning_count,report_path\n' > "$SUMMARY"
fi

for top in "${TOPS[@]}"; do
  log="$LOG_DIR/${top}_csynth.log"
  time_log="$LOG_DIR/${top}_csynth.time"
  report="$ROOT/boom_hls_gate3_6_${VARIANT}_${top}/solution_variant/syn/report/${top}_csynth.rpt"
  set +e
  HLS_BOOM_ROOT="$ROOT" GATE36_VARIANT="$VARIANT" BOOM_HLS_TOP="$top" FPGA_PART=${FPGA_PART:-xczu7ev-ffvc1156-2-e} CLOCK_PERIOD=${CLOCK_PERIOD:-10} \
    /usr/bin/time -f 'runtime_seconds=%e\npeak_memory_kb=%M' -o "$time_log" "$VITIS_HLS_BIN" -f "$ROOT/scripts/gate3_6/run_variant_csynth.tcl" > "$log" 2>&1
  rc=$?
  set -e
  status=FAIL
  if [ "$rc" -eq 0 ] && [ -f "$report" ]; then status=PASS; elif [ -f "$report" ]; then status=REPORT_WITH_NONZERO_EXIT; fi
  python3 "$ROOT/scripts/gate3_6/summarize_ncycle.py" --append "$SUMMARY" --top "$top" --status "$status" --report "$report" --log "$log" --time-log "$time_log"
done
