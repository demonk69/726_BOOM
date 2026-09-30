#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)"}
BUILD_ROOT=${G6_L2_SYNTH_ROOT:-/home/lab_726/opencode_tmp/g6_l2/full_core}
REPORT=${G6_L2_SYNTH_REPORT:-$ROOT/reports/gate6_0_full_lsu/l2/synthesis}
VITIS_HLS=${VITIS_HLS:-/home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls}
TIMEOUT=${G6_L2_SYNTH_TIMEOUT:-3600}
HARD_KB=${G6_L2_SYNTH_HARD_KB:-16777216}
CONFIGS=${G6_L2_SYNTH_CONFIGS:-8_8}

[[ "$BUILD_ROOT" == /home/lab_726/opencode_tmp/* ]] || exit 2
mkdir -p "$BUILD_ROOT" "$REPORT"
printf 'config,status,runtime_seconds,peak_workspace_kb,LUT,FF,BRAM,DSP,estimated_period,report_path\n' >"$REPORT/parameter_csynth.csv"

for config in $CONFIGS; do
    lq=${config%_*}; sq=${config#*_}
    work=$BUILD_ROOT/$config
    mkdir -p "$work"
    log=$REPORT/${config}.log
    start=$(date +%s); peak=0
    setsid timeout --signal=TERM --kill-after=30s "$TIMEOUT" env \
        HLS_BOOM_ROOT="$ROOT" G6_L2_SYNTH_WORK="$work" \
        LQ_DEPTH="$lq" SQ_DEPTH="$sq" FPGA_PART=xczu7ev-ffvc1156-2-e \
        CLOCK_PERIOD=10 "$VITIS_HLS" \
        -f "$ROOT/scripts/gate6_0/l2_full_core_csynth.tcl" >"$log" 2>&1 &
    pid=$!
    while kill -0 "$pid" 2>/dev/null; do
        current=$(du -sk "$work" 2>/dev/null | awk '{print $1+0}')
        ((current > peak)) && peak=$current
        if ((current >= HARD_KB)); then
            kill -TERM -- "-$pid" 2>/dev/null || true
            sleep 5
            kill -KILL -- "-$pid" 2>/dev/null || true
            wait "$pid" || true
            printf '%s,FAIL_RESOURCE,%s,%s,,,,,,\n' "$config" "$(( $(date +%s)-start ))" "$peak" >>"$REPORT/parameter_csynth.csv"
            exit 1
        fi
        sleep 10
    done
    wait "$pid"
    runtime=$(( $(date +%s)-start ))
    report=$work/hls_project/solution/syn/report/boom_core_top_csynth.rpt
    xml=$work/hls_project/solution/syn/report/boom_core_top_csynth.xml
    [[ -s "$report" && -s "$xml" ]]
    read -r lut ff bram dsp period < <(python3 - "$xml" <<'PY'
import sys, xml.etree.ElementTree as ET
root=ET.parse(sys.argv[1]).getroot()
def text(path): return root.findtext(path, default="")
print(text('.//AreaEstimates/Resources/LUT'), text('.//AreaEstimates/Resources/FF'),
      text('.//AreaEstimates/Resources/BRAM_18K'), text('.//AreaEstimates/Resources/DSP'),
      text('.//PerformanceEstimates/SummaryOfTimingAnalysis/EstimatedClockPeriod'))
PY
)
    printf '%s,PASS,%s,%s,%s,%s,%s,%s,%s,%s\n' "$config" "$runtime" "$peak" \
        "$lut" "$ff" "$bram" "$dsp" "$period" \
        "/home/lab_726/opencode_tmp/g6_l2/full_core/$config/hls_project/solution/syn/report/boom_core_top_csynth.rpt" \
        >>"$REPORT/parameter_csynth.csv"
    printf 'G6_L2_CSYNTH_%s=PASS LUT=%s FF=%s BRAM=%s DSP=%s PERIOD=%s\n' \
        "$config" "$lut" "$ff" "$bram" "$dsp" "$period"
done
