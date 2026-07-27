#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
VARIANT=${1:?variant name required}
REPORT_DIR="$ROOT/reports/gate3_5/variants/$VARIANT"
LOG_DIR="$REPORT_DIR/logs"
SUMMARY="$REPORT_DIR/csynth_summary.csv"
VITIS_HLS_BIN=${VITIS_HLS:-vitis_hls}

mkdir -p "$REPORT_DIR" "$LOG_DIR"

"$ROOT/scripts/gate3_5/run_all_regressions.sh" "$VARIANT"

if ! command -v "$VITIS_HLS_BIN" >/dev/null 2>&1; then
  if [ -x /home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls ]; then
    VITIS_HLS_BIN=/home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls
  else
    echo "Vitis HLS not found" >&2
    exit 127
  fi
fi

printf 'module,status,runtime_seconds,peak_memory_kb,estimated_period_ns,lut,ff,bram_18k,dsp,latency,interval,automatic_partition_count,warning_count,report_path\n' > "$SUMMARY"
for module in synth_free_list_rollback_top synth_core_step_top boom_core_top; do
  log="$LOG_DIR/${module}_csynth.log"
  time_log="$LOG_DIR/${module}_csynth.time"
  report="$ROOT/boom_hls_gate3_5_${VARIANT}_${module}/solution_module/syn/report/${module}_csynth.rpt"
  status=FAIL
  set +e
  HLS_BOOM_ROOT="$ROOT" GATE35_VARIANT="$VARIANT" BOOM_HLS_TOP="$module" \
  FPGA_PART=${FPGA_PART:-xczu7ev-ffvc1156-2-e} CLOCK_PERIOD=${CLOCK_PERIOD:-10} \
    /usr/bin/time -f 'runtime_seconds=%e\npeak_memory_kb=%M' -o "$time_log" \
    "$VITIS_HLS_BIN" -f "$ROOT/scripts/gate3_5/run_variant_csynth.tcl" > "$log" 2>&1
  rc=$?
  set -e
  if [ -f "$report" ] && [ "$rc" -eq 0 ]; then
    status=PASS
  elif [ -f "$report" ]; then
    status=REPORT_WITH_NONZERO_EXIT
  fi
  python3 "$ROOT/scripts/analyze_gate3_4_resources.py" --root "$ROOT" --append-module-row "$SUMMARY" --module "$module" --status "$status" --report "$report" --log "$log" --time-log "$time_log"
done

python3 "$ROOT/scripts/gate3_5/compare_variant.py" --root "$ROOT" --variant "$VARIANT"
echo "Gate 3.5 variant complete: $VARIANT"
