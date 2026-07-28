#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
VARIANT=${1:?variant required}
REQUESTED_II=${2:?requested II required; use auto or none where applicable}
TIMEOUT_SECONDS=${3:?timeout seconds required}
REPORT_DIR="$ROOT/reports/gate3_7/variants/$VARIANT"
PROJECT_DIR="$ROOT/build/gate3_7/hls_projects/$VARIANT"
DIRECTIVE="$REPORT_DIR/directive.tcl"
LOG="$REPORT_DIR/csynth.log"
TIME_LOG="$REPORT_DIR/csynth.time"
STATUS_FILE="$REPORT_DIR/run_status.txt"
RAW_REPORT="$PROJECT_DIR/solution_pipeline/syn/report/boom_core_top_csynth.rpt"
RAW_CYCLE_REPORT="$PROJECT_DIR/solution_pipeline/syn/report/boom_core_cycle_io_csynth.rpt"
SOLUTION_LOG="$PROJECT_DIR/solution_pipeline/solution_pipeline.log"
FLOW_LOG="$PROJECT_DIR/solution_pipeline/.autopilot/db/autopilot.flow.log"
PRAGMA_DUMP="$PROJECT_DIR/solution_pipeline/.autopilot/db/fe_pragma_dump.reflow.0.xml"
DIRECTIVE_DB="$PROJECT_DIR/solution_pipeline/solution_pipeline.directive"
VITIS_HLS_BIN=${VITIS_HLS:-vitis_hls}

if ! command -v "$VITIS_HLS_BIN" >/dev/null 2>&1; then
  VITIS_HLS_BIN=/home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls
fi
if [ ! -f "$DIRECTIVE" ]; then
  printf 'missing directive: %s\n' "$DIRECTIVE" >&2
  exit 2
fi

mkdir -p "$REPORT_DIR" "$(dirname "$PROJECT_DIR")"
set +e
HLS_BOOM_ROOT="$ROOT" GATE37_VARIANT="$VARIANT" GATE37_PROJECT="$PROJECT_DIR" GATE37_DIRECTIVE="$DIRECTIVE" \
  FPGA_PART=${FPGA_PART:-xczu7ev-ffvc1156-2-e} CLOCK_PERIOD=${CLOCK_PERIOD:-10} \
  /usr/bin/time -f 'runtime_seconds=%e\npeak_memory_kb=%M' -o "$TIME_LOG" \
  timeout --signal=TERM --kill-after=30s "$TIMEOUT_SECONDS" \
  "$VITIS_HLS_BIN" -f "$ROOT/scripts/gate3_7/pipeline_csynth.tcl" > "$LOG" 2>&1
rc=$?
set -e

status=FAIL
if [ -f "$RAW_REPORT" ] && [ "$rc" -eq 0 ]; then
  status=PASS
elif [ -f "$RAW_REPORT" ]; then
  status=REPORT_WITH_NONZERO_EXIT
elif [ "$rc" -eq 124 ] || [ "$rc" -eq 137 ]; then
  status=TIMEOUT
fi

printf 'variant=%s\nrequested_ii=%s\ntimeout_seconds=%s\nexit_code=%s\nstatus=%s\nproject=%s\n' \
  "$VARIANT" "$REQUESTED_II" "$TIMEOUT_SECONDS" "$rc" "$status" "$PROJECT_DIR" > "$STATUS_FILE"

if [ -f "$RAW_REPORT" ]; then cp "$RAW_REPORT" "$REPORT_DIR/boom_core_top_csynth.rpt"; fi
if [ -f "$RAW_CYCLE_REPORT" ]; then cp "$RAW_CYCLE_REPORT" "$REPORT_DIR/boom_core_cycle_io_csynth.rpt"; fi
if [ -f "$SOLUTION_LOG" ]; then cp "$SOLUTION_LOG" "$REPORT_DIR/solution_pipeline.log"; fi
if [ -f "$FLOW_LOG" ]; then cp "$FLOW_LOG" "$REPORT_DIR/autopilot.flow.log"; fi
if [ -f "$PRAGMA_DUMP" ]; then cp "$PRAGMA_DUMP" "$REPORT_DIR/fe_pragma_dump.reflow.0.xml"; fi
if [ -f "$DIRECTIVE_DB" ]; then cp "$DIRECTIVE_DB" "$REPORT_DIR/solution_pipeline.directive"; fi

python3 "$ROOT/scripts/gate3_7/analyze_pipeline_log.py" \
  --root "$ROOT" --variant "$VARIANT" --requested-ii "$REQUESTED_II" \
  --status "$status" --exit-code "$rc" --timeout-seconds "$TIMEOUT_SECONDS"

printf '%s: %s\n' "$VARIANT" "$status"
