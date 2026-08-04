#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
VITIS_HLS_BIN=${VITIS_HLS:-/home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls}
BUILD_ROOT="$ROOT/build/gate4_0/w4a_csynth"
PROJECT_ROOT="$BUILD_ROOT/projects"
REPORT_ROOT="$ROOT/reports/gate4_0/w4/csynth"
SOLUTION=solution_w4a_csynth
PART=xczu7ev-ffvc1156-2-e
CLOCK=10
TOPS=(synth_issue_top synth_execute_top synth_completion_top synth_rob_top \
      synth_lsu_top synth_core_step_top boom_core_top)

mkdir -p "$PROJECT_ROOT" "$REPORT_ROOT"
if ! "$VITIS_HLS_BIN" -version 2>&1 | tee "$ROOT/reports/gate4_0/w4/vitis_hls_version.txt" | grep -q 'v2021\.2'; then
  printf '%s\n' 'Vitis HLS 2021.2 is required' >&2
  exit 1
fi
"$ROOT/scripts/generate_merged.sh"

for top in "${TOPS[@]}"; do
  project="$PROJECT_ROOT/$top"
  report="$REPORT_ROOT/$top"
  mkdir -p "$report"
  (
    cd "$PROJECT_ROOT"
    BOOM_HLS_GATE=gate4_0_w4a BOOM_HLS_TOP="$top" BOOM_HLS_PROJECT="$top" \
      BOOM_HLS_SOLUTION="$SOLUTION" BOOM_HLS_CFLAGS_EXTRA= FPGA_PART="$PART" \
      CLOCK_PERIOD="$CLOCK" /usr/bin/time -f 'runtime_seconds=%e\npeak_memory_kb=%M' \
      -o "$report/csynth.time" "$VITIS_HLS_BIN" \
      -f "$ROOT/scripts/run_top_csynth.tcl" > "$report/csynth.log" 2>&1
  )
  synth_report="$project/$SOLUTION/syn/report"
  database="$project/$SOLUTION/.autopilot/db"
  cp "$synth_report/${top}_csynth.rpt" "$report/${top}_csynth.rpt"
  cp "$synth_report/${top}_csynth.xml" "$report/${top}_csynth.xml"
  mkdir -p "$report/verbose"
  for bind_report in "$database"/completion*.verbose.bind.rpt \
                     "$database"/rob_complete*.verbose.bind.rpt; do
    if [[ -f "$bind_report" ]]; then cp "$bind_report" "$report/verbose/"; fi
  done
  printf 'W4A_CSYNTH_PASS top=%s\n' "$top"
done

python3 - "$ROOT" "$REPORT_ROOT" <<'PY'
import csv
import hashlib
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

root, report = map(Path, sys.argv[1:])
tops = ("synth_issue_top", "synth_execute_top", "synth_completion_top",
        "synth_rob_top", "synth_lsu_top", "synth_core_step_top", "boom_core_top")
rows = []
for top in tops:
    path = report / top / f"{top}_csynth.xml"
    xml = ET.parse(path).getroot()
    get = lambda name: xml.findtext(name)
    version = get("./ReportVersion/Version")
    part = get("./UserAssignments/Part")
    target = get("./UserAssignments/TargetClockPeriod")
    estimate = get("./PerformanceEstimates/SummaryOfTimingAnalysis/EstimatedClockPeriod")
    pipeline = get("./PerformanceEstimates/PipelineType")
    if version != "2021.2" or part != "xczu7ev-ffvc1156-2-e" or pipeline != "no":
        raise SystemExit(f"{top}: synthesis guardrail failed")
    if top == "boom_core_top":
        core = xml.find("./PerformanceEstimates/SummaryOfLoopLatency/CORE_CYCLE")
        if core is None or core.find("PipelineII") is not None:
            raise SystemExit("boom_core_top: CORE_CYCLE is absent or pipelined")
    resources = {name.lower(): get(f"./AreaEstimates/Resources/{name}")
                 for name in ("LUT", "FF", "BRAM_18K", "DSP", "URAM")}
    timing = float(target) - float(estimate)
    runtime = {}
    for line in (report / top / "csynth.time").read_text().splitlines():
        if "=" in line:
            key, value = line.split("=", 1)
            runtime[key] = value
    rows.append({"variant": "GATE4_0_W4A", "top": top, "part": part,
                 "target_period_ns": target, "estimated_period_ns": estimate,
                 "timing_margin_ns": f"{timing:.3f}", **resources,
                 "pipeline_type": pipeline, "runtime_seconds": runtime.get("runtime_seconds", ""),
                 "peak_memory_kb": runtime.get("peak_memory_kb", ""),
                 "evidence_xml": str(path.relative_to(root))})
out = report.parent / "w4a_resource_summary.csv"
with out.open("w", newline="", encoding="utf-8") as stream:
    writer = csv.DictWriter(stream, fieldnames=list(rows[0]))
    writer.writeheader()
    writer.writerows(rows)

warning_rows = []
bind_rows = []
for top in tops:
    log = report / top / "csynth.log"
    warnings = [line.strip() for line in log.read_text(errors="replace").splitlines()
                if "WARNING: [HLS 200-805]" in line]
    warning_rows.append({"top": top, "hls_200_805_internal_stream_warnings": len(warnings),
                         "status": "PRESENT" if warnings else "NONE"})
    database = root / "build/gate4_0/w4a_csynth/projects" / top / \
        "solution_w4a_csynth/.autopilot/db"
    bind_paths = sorted(database.glob("*.verbose.bind.rpt"))
    operation_lines = []
    completion_lines = []
    for bind_path in bind_paths:
        lines = bind_path.read_text(errors="replace").splitlines()
        operation_lines.extend(line for line in lines if "Operation " in line)
        if bind_path.name.startswith(("completion", "rob_complete")):
            completion_lines.extend(line for line in lines if "Operation " in line)
    bind_rows.append({
        "top": top,
        "i129_operations": sum("i129" in line for line in operation_lines),
        "reciprocal_operations": sum("recip" in line.lower() for line in operation_lines),
        "completion_multiply_operations": sum(" mul " in line for line in completion_lines),
        "completion_address_multiply_operations": sum(
            " mul " in line and (" 52" in line or " 416" in line)
            for line in completion_lines),
    })
with (report.parent / "w4a_hls_warnings.csv").open("w", newline="", encoding="utf-8") as stream:
    writer = csv.DictWriter(stream, fieldnames=list(warning_rows[0]))
    writer.writeheader()
    writer.writerows(warning_rows)
with (report.parent / "w4a_bind_audit.csv").open("w", newline="", encoding="utf-8") as stream:
    writer = csv.DictWriter(stream, fieldnames=list(bind_rows[0]))
    writer.writeheader()
    writer.writerows(bind_rows)

hash_path = report.parent / "w4a_csynth_hashes.sha256"
paths = sorted(path for path in report.rglob("*") if path.is_file())
with hash_path.open("w", encoding="utf-8") as stream:
    for path in paths:
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        stream.write(f"{digest}  {path.relative_to(root)}\n")
PY

printf '%s\n' 'Gate 4.0 W4A synthesis complete: 7/7 targets.'
