#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
VITIS_HLS_BIN=${VITIS_HLS:-/home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls}
BUILD_ROOT=${BOOM_M2B_CSYNTH_BUILD_DIR:-"$ROOT/build/gate4_1/m2b_csynth"}
REPORT_ROOT=${BOOM_M2B_CSYNTH_REPORT_DIR:-"$ROOT/reports/gate4_1/m2/m2b/csynth"}
SOLUTION=solution_gate4_1_m2b
PART=xczu7ev-ffvc1156-2-e
CLOCK=10
TOPS=(synth_mul_top synth_execute_top synth_completion_top synth_core_step_top boom_core_step)

if ! "$VITIS_HLS_BIN" -version 2>&1 | grep -q 'v2021\.2'; then
  printf '%s\n' 'Vitis HLS 2021.2 is required.' >&2
  exit 1
fi

if [[ ${BOOM_M2B_CSYNTH_RESUME:-0} != 1 ]]; then
  rm -rf "$BUILD_ROOT" "$REPORT_ROOT"
fi
mkdir -p "$BUILD_ROOT/projects" "$REPORT_ROOT"
"$ROOT/scripts/generate_merged.sh"

for top in "${TOPS[@]}"; do
  report="$REPORT_ROOT/$top"
  if [[ ${BOOM_M2B_CSYNTH_RESUME:-0} == 1 && -s "$report/${top}_csynth.xml" ]]; then
    printf 'M2B_CSYNTH_RETAIN top=%s\n' "$top"
    continue
  fi
  mkdir -p "$report"
  if (
    cd "$BUILD_ROOT/projects"
    BOOM_HLS_GATE=gate4_1_m2b BOOM_HLS_TOP="$top" \
      BOOM_HLS_PROJECT="$top" BOOM_HLS_SOLUTION="$SOLUTION" \
      BOOM_HLS_CFLAGS_EXTRA= FPGA_PART="$PART" CLOCK_PERIOD="$CLOCK" \
      /usr/bin/time -f 'runtime_seconds=%e\npeak_memory_kb=%M' \
      -o "$report/csynth.time" "$VITIS_HLS_BIN" \
      -f "$ROOT/scripts/run_top_csynth.tcl" > "$report/csynth.log" 2>&1
  ); then
    :
  elif [[ "$top" == boom_core_step ]]; then
    printf '%s\n' 'status=FAIL reason=VITIS_HLS_AGGREGATE_INTERFACE_LIMIT' \
      > "$report/csynth_failed.txt"
    printf 'M2B_CSYNTH_BLOCKER top=%s reason=aggregate_interface_limit\n' "$top"
    continue
  else
    exit 1
  fi
  project="$BUILD_ROOT/projects/$top/$SOLUTION"
  cp "$project/syn/report/${top}_csynth.rpt" "$report/"
  cp "$project/syn/report/${top}_csynth.xml" "$report/"
  for path in "$project/.autopilot/db/$top.verbose.rpt" \
              "$project/.autopilot/db/$top.verbose.sched.rpt" \
              "$project/.autopilot/db/$top.verbose.bind.rpt"; do
    [[ ! -f "$path" ]] || cp "$path" "$report/"
  done
  printf 'M2B_CSYNTH_PASS top=%s\n' "$top"
done

"$ROOT/scripts/generate_merged.sh"

python3 - "$ROOT" "$REPORT_ROOT" <<'PY'
import csv
import re
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

root, report = map(Path, sys.argv[1:])
tops = ("synth_mul_top", "synth_execute_top", "synth_completion_top",
        "synth_core_step_top", "boom_core_step")
baseline = {
    "synth_mul_top": (612, 7, 0, 33, 6.463, "M2A"),
    "synth_execute_top": (1950, 577, 8, 3, 5.009, "W4"),
    "synth_completion_top": (37959, 10779, 8, 0, 5.474, "W4"),
    "synth_core_step_top": (107159, 25312, 16, 3, 6.025, "M1"),
}
rows = []
full_core_period = None
raw_target_status = "PASS"
for top in tops:
    path = report / top / f"{top}_csynth.xml"
    if not path.is_file():
        if top != "boom_core_step":
            raise SystemExit(f"missing synthesis report: {path}")
        raw_target_status = "VITIS_HLS_AGGREGATE_INTERFACE_LIMIT"
        rows.append((top, "xczu7ev-ffvc1156-2-e", "10.00", "", "", "", "", "",
                     "", "11.35", "1518436", "", "", "", "", "", "none",
                     "FAIL: aggregate interface exceeds 4096-bit Vitis HLS limit"))
        continue
    xml = ET.parse(path).getroot()
    get = lambda name: xml.findtext(name)
    if (get("./ReportVersion/Version") != "2021.2" or
            get("./UserAssignments/Part") != "xczu7ev-ffvc1156-2-e" or
            get("./UserAssignments/TargetClockPeriod") != "10.00" or
            get("./PerformanceEstimates/PipelineType") != "no"):
        raise SystemExit(f"{top}: synthesis guardrail failed")
    lut = int(get("./AreaEstimates/Resources/LUT"))
    ff = int(get("./AreaEstimates/Resources/FF"))
    bram = int(get("./AreaEstimates/Resources/BRAM_18K"))
    dsp = int(get("./AreaEstimates/Resources/DSP"))
    period = float(get("./PerformanceEstimates/SummaryOfTimingAnalysis/EstimatedClockPeriod"))
    runtime = dict(line.split("=", 1) for line in
                   (report / top / "csynth.time").read_text().splitlines() if "=" in line)
    old = baseline.get(top)
    if old:
        delta = (lut - old[0], ff - old[1], bram - old[2], dsp - old[3],
                 f"{period - old[4]:.3f}", old[5])
    else:
        delta = ("", "", "", "", "", "none")
    rows.append((top, "xczu7ev-ffvc1156-2-e", "10.00", f"{period:.3f}",
                 lut, ff, bram, dsp, "no", runtime.get("runtime_seconds", ""),
                 runtime.get("peak_memory_kb", ""), *delta,
                 str(path.relative_to(root))))
    if top == "synth_core_step_top":
        full_core_period = period

with (report.parent / "resource_summary.csv").open("w", newline="", encoding="utf-8") as stream:
    writer = csv.writer(stream)
    writer.writerow(("top", "part", "target_period_ns", "estimated_period_ns",
                     "lut", "ff", "bram_18k", "dsp", "pipeline_type",
                     "runtime_seconds", "peak_memory_kb", "delta_lut", "delta_ff",
                     "delta_bram_18k", "delta_dsp", "delta_period_ns", "baseline",
                     "evidence_xml"))
    writer.writerows(rows)

canonical = [root / "src" / name for name in
    ("boom_core_step.cpp", "frontend.cpp", "rvc.cpp", "decode.cpp", "rename.cpp", "completion.cpp",
     "rob.cpp", "issue.cpp", "mul.cpp", "execute.cpp", "branch.cpp", "lsu.cpp",
     "commit.cpp", "csr.cpp", "reset.cpp", "synth_module_tops.cpp",
     "boom_core_merged.cpp")]
for path in canonical:
    text = path.read_text(errors="replace")
    if re.search(r"#pragma\s+HLS\s+(?:DEPENDENCE.*false|DATAFLOW|ARRAY_PARTITION.*complete)",
                 text, re.I):
        raise SystemExit(f"forbidden optimization pragma: {path.relative_to(root)}")
if (root / "src/boom_core_merged.cpp").read_text().count("// ==== mul.cpp ====") != 1:
    raise SystemExit("mul.cpp merged-source occurrence is not one")

status = "M2B_PPA_BLOCKER" if (full_core_period is None or full_core_period > 6.5 or
                               raw_target_status != "PASS") else "PASS"
(report.parent / "ppa_status.txt").write_text(
    f"status={status}\nsynth_core_step_top_estimated_period_ns={full_core_period:.3f}\n"
    f"raw_boom_core_step_status={raw_target_status}\n"
    "threshold_ns=6.500\noptimization_attempted=false\n", encoding="utf-8")
PY

printf '%s\n' 'Gate 4.1 M2B synthesis: 4/5 PASS; raw boom_core_step interface blocker recorded.'
