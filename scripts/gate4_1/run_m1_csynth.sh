#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
VITIS_HLS_BIN=${VITIS_HLS:-/home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls}
BUILD_ROOT=${BOOM_M1_CSYNTH_BUILD_DIR:-"$ROOT/build/gate4_1/m1_csynth"}
REPORT_ROOT=${BOOM_M1_CSYNTH_REPORT_DIR:-"$ROOT/reports/gate4_1/m1/csynth"}
SOLUTION=solution_gate4_1_m1
PART=xczu7ev-ffvc1156-2-e
CLOCK=10
TOPS=(synth_decode_top synth_core_step_top boom_core_top)

if ! "$VITIS_HLS_BIN" -version 2>&1 | grep -q 'v2021\.2'; then
  printf '%s\n' 'Vitis HLS 2021.2 is required.' >&2
  exit 1
fi

rm -rf "$BUILD_ROOT" "$REPORT_ROOT"
mkdir -p "$BUILD_ROOT/projects" "$REPORT_ROOT"
"$ROOT/scripts/generate_merged.sh"

for top in "${TOPS[@]}"; do
  report="$REPORT_ROOT/$top"
  mkdir -p "$report"
  (
    cd "$BUILD_ROOT/projects"
    BOOM_HLS_GATE=gate4_1_m1 BOOM_HLS_TOP="$top" \
      BOOM_HLS_PROJECT="$top" BOOM_HLS_SOLUTION="$SOLUTION" \
      BOOM_HLS_CFLAGS_EXTRA= FPGA_PART="$PART" CLOCK_PERIOD="$CLOCK" \
      /usr/bin/time -f 'runtime_seconds=%e\npeak_memory_kb=%M' \
      -o "$report/csynth.time" "$VITIS_HLS_BIN" \
      -f "$ROOT/scripts/run_top_csynth.tcl" > "$report/csynth.log" 2>&1
  )
  project="$BUILD_ROOT/projects/$top/$SOLUTION"
  cp "$project/syn/report/${top}_csynth.rpt" "$report/"
  cp "$project/syn/report/${top}_csynth.xml" "$report/"
  printf 'M1_CSYNTH_PASS top=%s\n' "$top"
done

"$ROOT/scripts/generate_merged.sh"

python3 - "$ROOT" "$REPORT_ROOT" <<'PY'
import csv
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

root, report = map(Path, sys.argv[1:])
tops = ("synth_decode_top", "synth_core_step_top", "boom_core_top")
baseline = {
    "synth_core_step_top": (105121, 24687, 16, 3, 6.025),
    "boom_core_top": (111869, 25094, 16, 3, 6.025),
}
rows = []
for top in tops:
    path = report / top / f"{top}_csynth.xml"
    xml = ET.parse(path).getroot()
    get = lambda name: xml.findtext(name)
    if (get("./ReportVersion/Version") != "2021.2" or
            get("./UserAssignments/Part") != "xczu7ev-ffvc1156-2-e" or
            get("./UserAssignments/TargetClockPeriod") != "10.00" or
            get("./PerformanceEstimates/PipelineType") != "no"):
        raise SystemExit(f"{top}: synthesis guardrail failed")
    if top == "boom_core_top":
        cycle = xml.find("./PerformanceEstimates/SummaryOfLoopLatency/CORE_CYCLE")
        if cycle is None or cycle.find("PipelineII") is not None:
            raise SystemExit("boom_core_top: CORE_CYCLE absent or pipelined")
    lut = int(get("./AreaEstimates/Resources/LUT"))
    ff = int(get("./AreaEstimates/Resources/FF"))
    bram = int(get("./AreaEstimates/Resources/BRAM_18K"))
    dsp = int(get("./AreaEstimates/Resources/DSP"))
    period = float(get("./PerformanceEstimates/SummaryOfTimingAnalysis/EstimatedClockPeriod"))
    old = baseline.get(top)
    deltas = ("", "", "", "", "") if old is None else (
        lut - old[0], ff - old[1], bram - old[2], dsp - old[3],
        f"{period - old[4]:.3f}")
    rows.append(("M1_DECODE", top, "xczu7ev-ffvc1156-2-e", "10.00",
                 f"{period:.3f}", lut, ff, bram, dsp, "no", *deltas,
                 str(path.relative_to(root))))

with (report.parent / "resource_summary.csv").open("w", newline="", encoding="utf-8") as stream:
    writer = csv.writer(stream)
    writer.writerow(("variant", "top", "part", "target_period_ns", "estimated_period_ns",
                     "lut", "ff", "bram_18k", "dsp", "pipeline_type", "delta_lut",
                     "delta_ff", "delta_bram_18k", "delta_dsp", "delta_period_ns",
                     "evidence_xml"))
    writer.writerows(rows)
PY

printf '%s\n' 'Gate 4.1 M1 synthesis complete: 3/3 targets.'
