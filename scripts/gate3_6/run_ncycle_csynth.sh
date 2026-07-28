#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
VITIS_HLS_BIN=${VITIS_HLS:-vitis_hls}
OUT_DIR="$ROOT/reports/gate3_6/ncycle_csynth"
SUMMARY="$ROOT/reports/gate3_6/ncycle_resource_scaling.csv"

if ! command -v "$VITIS_HLS_BIN" >/dev/null 2>&1; then
  if [ -x /home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls ]; then
    VITIS_HLS_BIN=/home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls
  else
    echo "Vitis HLS not found" >&2
    exit 127
  fi
fi

mkdir -p "$OUT_DIR"
if [ "${GATE36_APPEND:-0}" != "1" ]; then
  printf 'top,status,runtime_seconds,peak_memory_kb,estimated_period_ns,lut,ff,bram_18k,dsp,automatic_partition_count,warning_count,report_path\n' > "$SUMMARY"
fi

TOPS=(boom_core_ncycle_n1_top boom_core_ncycle_n2_top boom_core_ncycle_n4_top boom_core_ncycle_n8_top boom_core_top)
if [ "$#" -gt 0 ]; then TOPS=("$@"); fi

for top in "${TOPS[@]}"; do
  log="$OUT_DIR/${top}.log"
  time_log="$OUT_DIR/${top}.time"
  report="$ROOT/boom_hls_gate3_6_${top}/solution_ncycle/syn/report/${top}_csynth.rpt"
  set +e
  HLS_BOOM_ROOT="$ROOT" BOOM_HLS_TOP="$top" FPGA_PART=${FPGA_PART:-xczu7ev-ffvc1156-2-e} CLOCK_PERIOD=${CLOCK_PERIOD:-10} \
    /usr/bin/time -f 'runtime_seconds=%e\npeak_memory_kb=%M' -o "$time_log" \
    "$VITIS_HLS_BIN" -f "$ROOT/scripts/gate3_6/run_ncycle_csynth.tcl" > "$log" 2>&1
  rc=$?
  set -e
  status=FAIL
  if [ "$rc" -eq 0 ] && [ -f "$report" ]; then status=PASS; elif [ -f "$report" ]; then status=REPORT_WITH_NONZERO_EXIT; fi
  python3 "$ROOT/scripts/gate3_6/summarize_ncycle.py" --append "$SUMMARY" --top "$top" --status "$status" --report "$report" --log "$log" --time-log "$time_log"
done

python3 "$ROOT/scripts/gate3_6/summarize_ncycle.py" --markdown "$SUMMARY" "$ROOT/reports/gate3_6/ncycle_resource_scaling.md"
echo "Gate 3.6 N-cycle scaling: $SUMMARY"
