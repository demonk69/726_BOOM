#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
VITIS_HLS_BIN=${VITIS_HLS:-/home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls}
BUILD_ROOT=${BOOM_M2A_CSYNTH_BUILD_DIR:-"$ROOT/build/gate4_1/m2a_csynth"}
REPORT_DIR=${BOOM_M2A_CSYNTH_REPORT_DIR:-"$ROOT/reports/gate4_1/m2/m2a"}
TOP=synth_mul_top
SOLUTION=solution_gate4_1_m2a
PART=xczu7ev-ffvc1156-2-e
CLOCK=10

if ! "$VITIS_HLS_BIN" -version 2>&1 | grep -q 'v2021\.2'; then
  printf '%s\n' 'Vitis HLS 2021.2 is required.' >&2
  exit 1
fi

rm -rf "$BUILD_ROOT"
mkdir -p "$BUILD_ROOT/projects" "$REPORT_DIR"
"$ROOT/scripts/generate_merged.sh"

(
  cd "$BUILD_ROOT/projects"
  BOOM_HLS_GATE=gate4_1_m2a BOOM_HLS_TOP="$TOP" \
    BOOM_HLS_PROJECT="$TOP" BOOM_HLS_SOLUTION="$SOLUTION" \
    BOOM_HLS_CFLAGS_EXTRA= FPGA_PART="$PART" CLOCK_PERIOD="$CLOCK" \
    /usr/bin/time -f 'runtime_seconds=%e\npeak_memory_kb=%M' \
    -o "$REPORT_DIR/synth_mul_top_csynth.time" "$VITIS_HLS_BIN" \
    -f "$ROOT/scripts/run_top_csynth.tcl" \
    > "$REPORT_DIR/synth_mul_top_csynth.log" 2>&1
)

project="$BUILD_ROOT/projects/$TOP/$SOLUTION"
cp "$project/syn/report/${TOP}_csynth.rpt" "$REPORT_DIR/synth_mul_top_csynth.rpt"
cp "$project/syn/report/${TOP}_csynth.xml" "$REPORT_DIR/synth_mul_top_csynth.xml"
cp "$project/.autopilot/db/${TOP}.verbose.bind.rpt" \
  "$REPORT_DIR/synth_mul_top_verbose_bind.rpt"

python3 - "$REPORT_DIR" <<'PY'
import csv
import re
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

report = Path(sys.argv[1])
xml = ET.parse(report / "synth_mul_top_csynth.xml").getroot()
get = lambda name: xml.findtext(name)
if (get("./ReportVersion/Version") != "2021.2" or
        get("./UserAssignments/Part") != "xczu7ev-ffvc1156-2-e" or
        get("./UserAssignments/TargetClockPeriod") != "10.00" or
        get("./PerformanceEstimates/PipelineType") != "no"):
    raise SystemExit("M2A synthesis guardrail failed")
timing = get("./PerformanceEstimates/SummaryOfTimingAnalysis/EstimatedClockPeriod")
latency = get("./PerformanceEstimates/SummaryOfOverallLatency/Best-caseLatency")
runtime = dict(line.split("=", 1) for line in
               (report / "synth_mul_top_csynth.time").read_text().splitlines() if "=" in line)
bind = (report / "synth_mul_top_verbose_bind.rpt").read_text(errors="replace")
multiply_operations = len(re.findall(r"Operation \d+ 'mul'", bind))
with (report / "synth_mul_summary.csv").open("w", newline="", encoding="utf-8") as stream:
    writer = csv.writer(stream)
    writer.writerow(("top", "part", "target_period_ns", "estimated_period_ns", "latency_cycles",
                     "lut", "ff", "bram_18k", "dsp", "pipeline_type", "multiply_operations",
                     "runtime_seconds", "peak_memory_kb"))
    writer.writerow(("synth_mul_top", "xczu7ev-ffvc1156-2-e", "10.00", timing, latency,
                     get("./AreaEstimates/Resources/LUT"), get("./AreaEstimates/Resources/FF"),
                     get("./AreaEstimates/Resources/BRAM_18K"),
                     get("./AreaEstimates/Resources/DSP"), "no", multiply_operations,
                     runtime.get("runtime_seconds", ""), runtime.get("peak_memory_kb", "")))
PY

printf '%s\n' 'Gate 4.1 M2A synth_mul_top synthesis complete.'
