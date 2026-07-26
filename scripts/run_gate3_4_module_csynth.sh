#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"}
VITIS_HLS_BIN=${VITIS_HLS:-vitis_hls}
GATE_TAG=${BOOM_HLS_GATE:-gate3_4}

if ! command -v "$VITIS_HLS_BIN" >/dev/null 2>&1; then
  if [ -x /home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls ]; then
    VITIS_HLS_BIN=/home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls
  else
    echo "Vitis HLS not found" >&2
    exit 127
  fi
fi

OUT_DIR="$ROOT/reports/$GATE_TAG/module_csynth"
SUMMARY="$ROOT/reports/$GATE_TAG/module_baseline.csv"
mkdir -p "$OUT_DIR"

MODULES=(
  synth_branch_tag_top
  synth_branch_mask_top
  synth_map_snapshot_top
  synth_free_list_rollback_top
  synth_busy_recovery_top
  synth_branch_kill_top
  synth_rename_top
  synth_rob_top
  synth_issue_top
  synth_lsu_top
  synth_core_step_top
  boom_core_top
)

if [ "$#" -gt 0 ]; then
  MODULES=("$@")
fi

printf 'module,status,runtime_seconds,peak_memory_kb,estimated_period_ns,lut,ff,bram_18k,dsp,latency,interval,automatic_partition_count,warning_count,report_path\n' > "$SUMMARY"

for module in "${MODULES[@]}"; do
  log="$OUT_DIR/${module}.log"
  time_log="$OUT_DIR/${module}.time"
  if [ "$module" = "boom_core_top" ]; then
    report="$ROOT/boom_hls_${GATE_TAG}_${module}/solution_module/syn/report/boom_core_top_csynth.rpt"
  else
    report="$ROOT/boom_hls_${GATE_TAG}_${module}/solution_module/syn/report/${module}_csynth.rpt"
  fi
  status=FAIL
  set +e
  FPGA_PART=${FPGA_PART:-xczu7ev-ffvc1156-2-e} CLOCK_PERIOD=${CLOCK_PERIOD:-10} BOOM_HLS_GATE="$GATE_TAG" BOOM_HLS_TOP="$module" \
    /usr/bin/time -f 'runtime_seconds=%e\npeak_memory_kb=%M' -o "$time_log" \
    "$VITIS_HLS_BIN" -f "$ROOT/scripts/gate3_4_module_csynth.tcl" > "$log" 2>&1
  rc=$?
  set -e
  if [ -f "$report" ] && [ "$rc" -eq 0 ]; then
    status=PASS
  elif [ -f "$report" ]; then
    status=REPORT_WITH_NONZERO_EXIT
  fi
  python3 "$ROOT/scripts/analyze_gate3_4_resources.py" --root "$ROOT" --append-module-row "$SUMMARY" --module "$module" --status "$status" --report "$report" --log "$log" --time-log "$time_log"
done

echo "Gate 3.4 module baseline: $SUMMARY"
