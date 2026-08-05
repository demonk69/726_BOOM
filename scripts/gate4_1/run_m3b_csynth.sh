#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
VITIS_HLS_BIN=${VITIS_HLS:-/home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls}
BUILD_ROOT=${BOOM_M3B_CSYNTH_BUILD_DIR:-"$ROOT/build/gate4_1/m3b_csynth"}
REPORT_ROOT=${BOOM_M3B_CSYNTH_REPORT_DIR:-"$ROOT/reports/gate4_1/m3/m3b/csynth"}
SOLUTION=solution_gate4_1_m3b
TOPS=(synth_divider_top synth_mul_top synth_execute_top synth_completion_top
      synth_rob_top synth_lsu_top synth_core_step_top boom_core_top)

if ! "$VITIS_HLS_BIN" -version 2>&1 | grep -q 'v2021\.2'; then
  printf '%s\n' 'Vitis HLS 2021.2 is required.' >&2
  exit 1
fi
if [[ ${BOOM_M3B_CSYNTH_RESUME:-0} != 1 ]]; then
  rm -rf "$BUILD_ROOT" "$REPORT_ROOT"
fi
mkdir -p "$BUILD_ROOT/projects" "$REPORT_ROOT"
"$ROOT/scripts/generate_merged.sh"

for top in "${TOPS[@]}"; do
  report="$REPORT_ROOT/$top"
  if [[ ${BOOM_M3B_CSYNTH_RESUME:-0} == 1 && -s "$report/${top}_csynth.xml" ]]; then
    printf 'M3B_CSYNTH_RETAIN top=%s\n' "$top"
    continue
  fi
  mkdir -p "$report"
  (
    cd "$BUILD_ROOT/projects"
    BOOM_HLS_GATE=gate4_1_m3b BOOM_HLS_TOP="$top" \
      BOOM_HLS_PROJECT="$top" BOOM_HLS_SOLUTION="$SOLUTION" \
      BOOM_HLS_CFLAGS_EXTRA= FPGA_PART=xczu7ev-ffvc1156-2-e CLOCK_PERIOD=10 \
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
  printf 'M3B_CSYNTH_PASS top=%s\n' "$top"
done

python3 - "$ROOT" "$REPORT_ROOT" "${TOPS[@]}" <<'PY'
import csv
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

root = Path(sys.argv[1])
report = Path(sys.argv[2])
rows = []
for top in sys.argv[3:]:
    path = report / top / f"{top}_csynth.xml"
    xml = ET.parse(path).getroot()
    get = lambda name: xml.findtext(name)
    guards = (get("./ReportVersion/Version") == "2021.2" and
              get("./UserAssignments/Part") == "xczu7ev-ffvc1156-2-e" and
              get("./UserAssignments/TargetClockPeriod") == "10.00" and
              get("./PerformanceEstimates/PipelineType") == "no")
    if not guards:
        raise SystemExit(f"{top}: synthesis guardrail failed")
    period = float(get("./PerformanceEstimates/SummaryOfTimingAnalysis/EstimatedClockPeriod"))
    resources = [int(get(f"./AreaEstimates/Resources/{name}"))
                 for name in ("LUT", "FF", "BRAM_18K", "DSP")]
    timing = dict(line.split("=", 1) for line in
                  (report / top / "csynth.time").read_text().splitlines() if "=" in line)
    rows.append((top, *resources, f"{period:.3f}",
                 timing.get("runtime_seconds", ""), "no",
                 "32_or_64_iterations" if top in ("synth_divider_top", "synth_execute_top",
                                                   "synth_core_step_top", "boom_core_top") else "n/a",
                 str(path.relative_to(root))))
summary = report.parent / "resource_summary.csv"
with summary.open("w", newline="", encoding="utf-8") as stream:
    writer = csv.writer(stream)
    writer.writerow(("top", "lut", "ff", "bram", "dsp", "estimated_period",
                     "runtime", "pipeline_status", "divider_latency", "evidence_xml"))
    writer.writerows(rows)
by_top = {row[0]: row for row in rows}
if any(row[4] > 3 for row in rows) or by_top["synth_divider_top"][4] != 0:
    raise SystemExit("DSP guardrail failed")
if any(row[3] > 16 for row in rows):
    raise SystemExit("BRAM guardrail failed")
blocker = any(float(by_top[top][5]) > 6.5
              for top in ("synth_core_step_top", "boom_core_top"))
(report.parent / "ppa_status.txt").write_text(
    f"M3B_PPA_BLOCKER={'true' if blocker else 'false'}\n", encoding="utf-8")
PY

printf '%s\n' 'Gate 4.1 M3B canonical synthesis: 8/8 complete.'
