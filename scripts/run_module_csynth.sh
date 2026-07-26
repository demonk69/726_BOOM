#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"}
VITIS_HLS_BIN=${VITIS_HLS:-vitis_hls}
GATE_TAG=${BOOM_HLS_GATE:-gate3_3}
if ! command -v "$VITIS_HLS_BIN" >/dev/null 2>&1; then
  if [ -x /home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls ]; then
    VITIS_HLS_BIN=/home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls
  else
    echo "Vitis HLS not found" >&2
    exit 127
  fi
fi

OUT_DIR="$ROOT/reports/$GATE_TAG/module_csynth"
SUMMARY="$ROOT/reports/$GATE_TAG/module_csynth_summary.csv"
mkdir -p "$OUT_DIR"

MODULES=(
  synth_frontend_top
  synth_decode_top
  synth_rename_top
  synth_rob_top
  synth_issue_top
  synth_execute_top
  synth_lsu_top
  synth_commit_top
  synth_core_step_top
)

if [ "$#" -gt 0 ]; then
  MODULES=("$@")
fi

printf 'module,status,runtime,peak_memory,LUT,FF,BRAM,DSP,estimated_period,last_pass,warnings,report_path,log_path\n' > "$SUMMARY"

for module in "${MODULES[@]}"; do
  log="$OUT_DIR/${module}.log"
  time_log="$OUT_DIR/${module}.time"
  report="$ROOT/boom_hls_${GATE_TAG}_${module}/solution_module/syn/report/${module}_csynth.rpt"
  status=FAIL
  runtime=""
  peak_memory=""
  set +e
  FPGA_PART=${FPGA_PART:-xczu7ev-ffvc1156-2-e} CLOCK_PERIOD=${CLOCK_PERIOD:-10} BOOM_HLS_GATE="$GATE_TAG" BOOM_HLS_TOP="$module" \
    /usr/bin/time -f 'runtime_seconds=%e\npeak_memory_kb=%M' -o "$time_log" \
    "$VITIS_HLS_BIN" -f "$ROOT/scripts/module_csynth.tcl" > "$log" 2>&1
  rc=$?
  set -e
  if [ -f "$report" ] && [ "$rc" -eq 0 ]; then
    status=PASS
  elif [ -f "$report" ]; then
    status=REPORT_WITH_NONZERO_EXIT
  fi
  if [ -f "$time_log" ]; then
    runtime=$(python3 - "$time_log" <<'PY'
import sys
data={}
for line in open(sys.argv[1], encoding='utf-8', errors='ignore'):
    if '=' in line:
        k,v=line.strip().split('=',1); data[k]=v
print(data.get('runtime_seconds',''))
PY
)
    peak_memory=$(python3 - "$time_log" <<'PY'
import sys
data={}
for line in open(sys.argv[1], encoding='utf-8', errors='ignore'):
    if '=' in line:
        k,v=line.strip().split('=',1); data[k]=v
print(data.get('peak_memory_kb',''))
PY
)
  fi
  last_pass=$(python3 - "$log" <<'PY'
import re, sys
last=''
for line in open(sys.argv[1], encoding='utf-8', errors='ignore'):
    m=re.search(r'Finished ([^:]+):', line)
    if m: last=m.group(1)
print(last)
PY
)
  warnings=$(python3 - "$log" <<'PY'
import sys
count=0
for line in open(sys.argv[1], encoding='utf-8', errors='ignore'):
    if 'WARNING:' in line: count += 1
print(count)
PY
)
  printf '%s,%s,%s,%s,,,,,,%s,%s,%s,%s\n' "$module" "$status" "$runtime" "$peak_memory" "$last_pass" "$warnings" "$report" "$log" >> "$SUMMARY"
done

echo "Module csynth summary: $SUMMARY"
