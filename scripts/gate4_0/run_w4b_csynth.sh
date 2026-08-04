#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
VITIS_HLS_BIN=${VITIS_HLS:-/home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls}
STAGE=${BOOM_W4_STAGE:-w4b}
GATE=${BOOM_W4_GATE:-gate4_0_w4b}
VARIANT=${BOOM_W4_VARIANT:-W4B_MULTI_ROB_COMPLETE}
BUILD_ROOT="$ROOT/build/gate4_0/${STAGE}_csynth"
PROJECT_ROOT="$BUILD_ROOT/projects"
REPORT_ROOT="$ROOT/reports/gate4_0/w4/csynth/$STAGE"
SOLUTION="solution_${STAGE}_csynth"
PART=xczu7ev-ffvc1156-2-e
CLOCK=10
if [[ -n "${BOOM_W4_TOPS:-}" ]]; then
  read -r -a TOPS <<< "$BOOM_W4_TOPS"
else
  TOPS=(synth_issue_top synth_execute_top synth_completion_top synth_rob_top \
        synth_lsu_top synth_core_step_top boom_core_top)
fi

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
    BOOM_HLS_GATE="$GATE" BOOM_HLS_TOP="$top" BOOM_HLS_PROJECT="$top" \
      BOOM_HLS_SOLUTION="$SOLUTION" BOOM_HLS_CFLAGS_EXTRA= \
      FPGA_PART="$PART" CLOCK_PERIOD="$CLOCK" \
      /usr/bin/time -f 'runtime_seconds=%e\npeak_memory_kb=%M' \
      -o "$report/csynth.time" "$VITIS_HLS_BIN" \
      -f "$ROOT/scripts/run_top_csynth.tcl" > "$report/csynth.log" 2>&1
  )
  synth_report="$project/$SOLUTION/syn/report"
  database="$project/$SOLUTION/.autopilot/db"
  cp "$synth_report/${top}_csynth.rpt" "$report/${top}_csynth.rpt"
  cp "$synth_report/${top}_csynth.xml" "$report/${top}_csynth.xml"
  mkdir -p "$report/verbose"
  for bind_report in "$database"/completion*.verbose.bind.rpt \
                     "$database"/rob_complete*.verbose.bind.rpt \
                     "$database"/service_pending*.verbose.bind.rpt \
                     "$database"/begin_completion_cycle*.verbose.bind.rpt \
                     "$database"/build_wakeup_bypass_ports*.verbose.bind.rpt \
                     "$database"/wakeup_lookup*.verbose.bind.rpt \
                     "$database"/bypass_lookup*.verbose.bind.rpt; do
    if [[ -f "$bind_report" ]]; then cp "$bind_report" "$report/verbose/"; fi
  done
  printf '%s_CSYNTH_PASS top=%s\n' "${STAGE^^}" "$top"
done

python3 - "$ROOT" "$REPORT_ROOT" "$STAGE" "$VARIANT" <<'PY'
import csv
import re
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

root, report = map(Path, sys.argv[1:3])
stage, variant = sys.argv[3:]
canonical = ("synth_issue_top", "synth_execute_top", "synth_completion_top",
             "synth_rob_top", "synth_lsu_top", "synth_core_step_top", "boom_core_top",
             "synth_w4d_oracle_top")
tops = tuple(top for top in canonical if
             (report / top / f"{top}_csynth.xml").is_file())
rows = []
audits = []
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
            raise SystemExit("boom_core_top: CORE_CYCLE absent or pipelined")
    resources = {name.lower(): get(f"./AreaEstimates/Resources/{name}")
                 for name in ("LUT", "FF", "BRAM_18K", "DSP", "URAM")}
    runtime = {}
    for line in (report / top / "csynth.time").read_text().splitlines():
        if "=" in line:
            key, value = line.split("=", 1)
            runtime[key] = value
    rows.append({"variant": variant, "top": top, "part": part,
                 "target_period_ns": target, "estimated_period_ns": estimate,
                 "timing_margin_ns": f"{float(target)-float(estimate):.3f}",
                 **resources, "pipeline_type": pipeline,
                 "runtime_seconds": runtime.get("runtime_seconds", ""),
                 "peak_memory_kb": runtime.get("peak_memory_kb", ""),
                 "evidence_xml": str(path.relative_to(root))})
    operation_lines = []
    for bind in (report / top / "verbose").glob("*.verbose.bind.rpt"):
        operation_lines.extend(line for line in bind.read_text(errors="replace").splitlines()
                               if "Operation " in line)
    multiply = sum("'mul'" in line for line in operation_lines)
    address_multiply = sum("'mul'" in line and (" 52" in line or " 416" in line)
                           for line in operation_lines)
    reciprocal = sum("recip" in line.lower() for line in operation_lines)
    if multiply or address_multiply or reciprocal:
        raise SystemExit(f"{top}: artificial completion arithmetic found")
    audits.append((top, multiply, address_multiply, reciprocal, "PASS"))

with (report.parents[1] / f"{stage}_resource_summary.csv").open("w", newline="", encoding="utf-8") as stream:
    writer = csv.DictWriter(stream, fieldnames=list(rows[0]))
    writer.writeheader()
    writer.writerows(rows)
with (report.parents[1] / f"{stage}_bind_audit.csv").open("w", newline="", encoding="utf-8") as stream:
    writer = csv.writer(stream)
    writer.writerow(("top", "completion_multiply_operations", "completion_address_multiply_operations", "reciprocal_operations", "status"))
    writer.writerows(audits)

prf_rows = []
for top in ("synth_completion_top", "synth_core_step_top", "boom_core_top"):
    if top not in tops:
        continue
    rtl_dir = root / "build/gate4_0" / f"{stage}_csynth/projects" / top / f"solution_{stage}_csynth/syn/verilog"
    rtl = rtl_dir / f"{top}.v"
    text = rtl.read_text(errors="replace")
    signals = ("state_int_rf_bank0_we0", "state_int_rf_bank1_we0",
               "state_int_rf_bank0_U", "state_int_rf_bank1_U")
    if not all(signal in text for signal in signals):
        raise SystemExit(f"{top}: generated LVT PRF does not have two independent bank write enables")
    prf_rows.append((stage.upper(), top, 52, "2xRAM_AUTO_1R1W_52x64+LVT52", 2, 2,
                     "bank0_we0;bank1_we0", str(rtl.relative_to(root)), "PASS"))
if prf_rows:
    with (report.parents[1] / "prf_after.csv").open("w", newline="", encoding="utf-8") as stream:
        writer = csv.writer(stream)
        writer.writerow(("stage", "top", "phys_regs", "generated_structure", "physical_ports",
                         "write_ports", "write_enables", "evidence", "status"))
        writer.writerows(prf_rows)

if "boom_core_top" in tops:
    top_rtl = root / "build/gate4_0" / f"{stage}_csynth/projects/boom_core_top" / f"solution_{stage}_csynth/syn/verilog/boom_core_top.v"
    text = top_rtl.read_text(errors="replace")
    status_ports = (("io_success", 1), ("io_halted", 1), ("io_trap", 1),
                    ("io_cycle_valid", 1), ("io_cycle", 64), ("io_instret", 64))
    status_rows = []
    for name, width in status_ports:
        pattern = rf"(?m)^output\s+(?:\[{width-1}:0\]\s+)?{name};$" if width > 1 else rf"(?m)^output\s+{name};$"
        if not re.search(pattern, text):
            raise SystemExit(f"boom_core_top: {name} is not a generated output")
        status_rows.append((name, "output", width, str(top_rtl.relative_to(root)), "PASS"))
    with (report.parents[1] / "status_ports_after.csv").open("w", newline="", encoding="utf-8") as stream:
        writer = csv.writer(stream)
        writer.writerow(("port", "direction", "width", "evidence", "status"))
        writer.writerows(status_rows)
PY

printf 'Gate 4.0 %s synthesis complete: %d/%d targets.\n' "${STAGE^^}" "${#TOPS[@]}" "${#TOPS[@]}"
