#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
BUILD="$ROOT/build/gate5_3_fetch_buffer/b1/csynth"
REPORT="$ROOT/reports/gate5_3_fetch_buffer/b1"
VITIS_HLS=${VITIS_HLS:-/home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls}
TCL="$ROOT/scripts/gate5_3/b1_fetch_buffer_csynth.tcl"
mkdir -p "$BUILD" "$REPORT/logs/csynth"
[[ -x "$VITIS_HLS" ]] || { printf 'ERROR: missing %s\n' "$VITIS_HLS" >&2; exit 2; }

variants=(d2_auto d4_auto d8_auto d16_auto d8_lutram d8_bram d8_payload_reset)
for variant in "${variants[@]}"; do
    depth=${variant#d}; depth=${depth%%_*}
    defines=
    case "$variant" in
        *_lutram) defines=-DFETCH_BUFFER_STORAGE_LUTRAM ;;
        *_bram) defines=-DFETCH_BUFFER_STORAGE_BRAM ;;
        *_payload_reset) defines=-DFETCH_BUFFER_RESET_PAYLOAD ;;
    esac
    project="$BUILD/$variant"
    HLS_BOOM_ROOT="$ROOT" GATE5_3_B1_HLS_PROJECT="$project" \
        GATE5_3_B1_SOLUTION=solution GATE5_3_B1_DEPTH="$depth" \
        GATE5_3_B1_DEFINES="$defines" "$VITIS_HLS" -f "$TCL" \
        >"$REPORT/logs/csynth/$variant.log" 2>&1
done

python3 - "$BUILD" "$REPORT/csynth_sweep.csv" <<'PY'
import csv
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

build, output = map(Path, sys.argv[1:])
rows = []
for project in sorted(build.iterdir()):
    report = project / "solution/syn/report/synth_fetch_buffer_top_csynth.xml"
    if not report.exists():
        raise SystemExit(f"missing csynth report: {report}")
    root = ET.parse(report).getroot()
    def text(path, default="0"):
        node = root.find(path)
        return node.text if node is not None else default
    rows.append((project.name, project.name.split("_")[0][1:],
                 text("./AreaEstimates/Resources/LUT"),
                 text("./AreaEstimates/Resources/FF"),
                 text("./AreaEstimates/Resources/BRAM_18K"),
                 text("./AreaEstimates/Resources/DSP"),
                 text("./PerformanceEstimates/SummaryOfTimingAnalysis/EstimatedClockPeriod"),
                 text("./PerformanceEstimates/SummaryOfOverallLatency/Best-caseLatency", "?"),
                 text("./PerformanceEstimates/SummaryOfOverallLatency/Worst-caseLatency", "?"),
                 "Pipelined=no", str(report)))
with output.open("w", newline="") as stream:
    writer = csv.writer(stream)
    writer.writerow(("variant", "depth", "lut", "ff", "bram_18k", "dsp",
                     "estimated_period_ns", "best_latency", "worst_latency",
                     "core_cycle", "report"))
    writer.writerows(rows)
print(f"GATE5_3_B1_CSYNTH_SWEEP_PASS variants={len(rows)}")
PY
