#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
VITIS_HLS_BIN=${VITIS_HLS:-/home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls}
BUILD_ROOT=${BOOM_M2C_CSYNTH_BUILD_DIR:-"$ROOT/build/gate4_1/m2c_final/csynth"}
REPORT_ROOT=${BOOM_M2C_CSYNTH_REPORT_DIR:-"$ROOT/reports/gate4_1/m2/m2c/final/csynth"}
SOLUTION=solution_gate4_1_m2c
PART=xczu7ev-ffvc1156-2-e
CLOCK=10
TOPS=(synth_mul_top synth_issue_top synth_execute_top synth_completion_top
      synth_rob_top synth_lsu_top synth_core_step_top boom_core_top)

if ! "$VITIS_HLS_BIN" -version 2>&1 | grep -q 'v2021\.2'; then
  printf '%s\n' 'Vitis HLS 2021.2 is required.' >&2
  exit 1
fi

if [[ ${BOOM_M2C_CSYNTH_RESUME:-0} != 1 ]]; then
  rm -rf "$BUILD_ROOT" "$REPORT_ROOT"
fi
mkdir -p "$BUILD_ROOT/projects" "$REPORT_ROOT"
"$ROOT/scripts/generate_merged.sh"

for top in "${TOPS[@]}"; do
  report="$REPORT_ROOT/$top"
  if [[ ${BOOM_M2C_CSYNTH_RESUME:-0} == 1 && -s "$report/${top}_csynth.xml" ]]; then
    printf 'M2C_CSYNTH_RETAIN top=%s\n' "$top"
    continue
  fi
  mkdir -p "$report"
  (
    cd "$BUILD_ROOT/projects"
    BOOM_HLS_GATE=gate4_1_m2c BOOM_HLS_TOP="$top" \
      BOOM_HLS_PROJECT="$top" BOOM_HLS_SOLUTION="$SOLUTION" \
      BOOM_HLS_CFLAGS_EXTRA= FPGA_PART="$PART" CLOCK_PERIOD="$CLOCK" \
      /usr/bin/time -f 'runtime_seconds=%e\npeak_memory_kb=%M' \
      -o "$report/csynth.time" "$VITIS_HLS_BIN" \
      -f "$ROOT/scripts/run_top_csynth.tcl" > "$report/csynth.log" 2>&1
  )
  project="$BUILD_ROOT/projects/$top/$SOLUTION"
  cp "$project/syn/report/${top}_csynth.rpt" "$report/"
  cp "$project/syn/report/${top}_csynth.xml" "$report/"
  for path in "$project/.autopilot/db/$top.verbose.rpt" \
              "$project/.autopilot/db/$top.verbose.sched.rpt" \
              "$project/.autopilot/db/$top.verbose.bind.rpt"; do
    [[ ! -f "$path" ]] || cp "$path" "$report/"
  done
  printf 'M2C_CSYNTH_PASS top=%s\n' "$top"
done

python3 - "$ROOT" "$REPORT_ROOT" "${TOPS[@]}" <<'PY'
import csv
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

root = Path(sys.argv[1])
report = Path(sys.argv[2])
tops = sys.argv[3:]
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
    period = float(get("./PerformanceEstimates/SummaryOfTimingAnalysis/EstimatedClockPeriod"))
    resources = [int(get(f"./AreaEstimates/Resources/{name}"))
                 for name in ("LUT", "FF", "BRAM_18K", "DSP")]
    runtime = dict(line.split("=", 1) for line in
                   (report / top / "csynth.time").read_text().splitlines() if "=" in line)
    rows.append((top, f"{period:.3f}", *resources, "no",
                 runtime.get("runtime_seconds", ""), runtime.get("peak_memory_kb", ""),
                 str(path.relative_to(root))))

with (report.parent / "resource_summary.csv").open("w", newline="", encoding="utf-8") as stream:
    writer = csv.writer(stream)
    writer.writerow(("top", "estimated_period_ns", "lut", "ff", "bram_18k", "dsp",
                     "pipeline_type", "runtime_seconds", "peak_memory_kb", "evidence_xml"))
    writer.writerows(rows)

by_top = {row[0]: row for row in rows}
for top in ("synth_core_step_top", "boom_core_top"):
    if float(by_top[top][1]) > 6.5:
        raise SystemExit(f"{top}: estimated period exceeds 6.5 ns")
    if int(by_top[top][5]) > 3:
        raise SystemExit(f"{top}: multiply DSP target not met")
if int(by_top["synth_mul_top"][5]) > 3:
    raise SystemExit("synth_mul_top: multiply DSP target not met")
PY

printf '%s\n' 'Gate 4.1 M2C canonical synthesis: 8/8 PASS.'
