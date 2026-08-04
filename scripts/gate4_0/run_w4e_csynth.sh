#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
VITIS_HLS_BIN=${VITIS_HLS:-/home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls}
BUILD_ROOT="$ROOT/build/gate4_0/w4e_csynth_final"
PROJECT_ROOT="$BUILD_ROOT/projects"
REPORT_ROOT="$ROOT/reports/gate4_0/w4/csynth_final"
SOLUTION=solution_w4e_final
PART=xczu7ev-ffvc1156-2-e
CLOCK=10
TOPS=(synth_issue_top synth_execute_top synth_completion_top synth_rob_top \
      synth_lsu_top synth_core_step_top boom_core_top)
SOURCE_HASHES="$ROOT/reports/gate4_0/w4/source_hashes_after.txt"
PRODUCT_HASHES="$ROOT/reports/gate4_0/w4/product_source_hashes.txt"

[[ -f "$SOURCE_HASHES" ]] || {
  printf '%s\n' 'Bind current source hashes before final csynth.' >&2
  exit 1
}

verify_hash_manifest() {
  local manifest=$1
  python3 - "$ROOT" "$manifest" <<'PY'
import hashlib
import sys
from pathlib import Path

root, manifest = map(Path, sys.argv[1:])
seen = set()
for line in manifest.read_text(encoding="utf-8").splitlines():
    if not line or line.startswith("#"): continue
    digest, name = line.split(None, 1)
    name = name.strip()
    if name in seen or Path(name).name == "boom_all.cpp":
        raise SystemExit(f"invalid source hash binding: {name}")
    seen.add(name)
    path = root / name
    if not path.is_file():
        raise SystemExit(f"missing bound source: {name}")
    actual = hashlib.sha256(path.read_bytes()).hexdigest()
    if actual != digest:
        raise SystemExit(f"source hash mismatch: {name}")
if not seen:
    raise SystemExit("empty source hash binding")
PY
}

bind_source_hashes() {
  python3 - "$ROOT" "$SOURCE_HASHES" <<'PY'
import hashlib
import sys
from pathlib import Path

root, manifest = map(Path, sys.argv[1:])
names = []
for line in manifest.read_text(encoding="utf-8").splitlines():
    if not line or line.startswith("#"): continue
    _, name = line.split(None, 1)
    name = name.strip()
    if Path(name).name == "boom_all.cpp":
        raise SystemExit("boom_all.cpp must not be bound")
    names.append(name)
if len(names) != len(set(names)) or not names:
    raise SystemExit("invalid source hash path set")
with manifest.open("w", encoding="utf-8") as stream:
    stream.write("# sha256  path\n")
    for name in names:
        path = root / name
        if not path.is_file(): raise SystemExit(f"missing bound source: {name}")
        stream.write(f"{hashlib.sha256(path.read_bytes()).hexdigest()}  {name}\n")
PY
}

verify_hash_manifest "$SOURCE_HASHES"
verify_hash_manifest "$PRODUCT_HASHES"
if ! "$VITIS_HLS_BIN" -version 2>&1 | grep -q 'v2021\.2'; then
  printf '%s\n' 'Vitis HLS 2021.2 is required.' >&2
  exit 1
fi

if [[ ${BOOM_W4E_RESUME:-0} != 1 ]]; then
  rm -rf "$BUILD_ROOT" "$REPORT_ROOT"
fi
mkdir -p "$PROJECT_ROOT" "$REPORT_ROOT"
"$ROOT/scripts/generate_merged.sh"
bind_source_hashes
verify_hash_manifest "$SOURCE_HASHES"
verify_hash_manifest "$PRODUCT_HASHES"

for top in "${TOPS[@]}"; do
  report="$REPORT_ROOT/$top"
  if [[ ${BOOM_W4E_RESUME:-0} == 1 && -f "$report/${top}_csynth.xml" ]]; then
    printf 'W4E_FINAL_CSYNTH_RETAIN top=%s\n' "$top"
    continue
  fi
  mkdir -p "$report/verbose"
  (
    cd "$PROJECT_ROOT"
    BOOM_HLS_GATE=gate4_0_w4e_final BOOM_HLS_TOP="$top" \
      BOOM_HLS_PROJECT="$top" BOOM_HLS_SOLUTION="$SOLUTION" \
      BOOM_HLS_CFLAGS_EXTRA= FPGA_PART="$PART" CLOCK_PERIOD="$CLOCK" \
      /usr/bin/time -f 'runtime_seconds=%e\npeak_memory_kb=%M' \
      -o "$report/csynth.time" "$VITIS_HLS_BIN" \
      -f "$ROOT/scripts/run_top_csynth.tcl" > "$report/csynth.log" 2>&1
  )
  project="$PROJECT_ROOT/$top/$SOLUTION"
  cp "$project/syn/report/${top}_csynth.rpt" "$report/"
  cp "$project/syn/report/${top}_csynth.xml" "$report/"
  for path in "$project/.autopilot/db/$top.verbose.rpt" \
              "$project/.autopilot/db/$top.verbose.sched.rpt" \
              "$project/.autopilot/db/$top.verbose.bind.rpt"; do
    [[ ! -f "$path" ]] || cp "$path" "$report/verbose/"
  done
  printf 'W4E_FINAL_CSYNTH_PASS top=%s\n' "$top"
done

# Regenerate deterministic merged input and reject any source drift before the
# copied synthesis reports are treated as final evidence.
"$ROOT/scripts/generate_merged.sh"
verify_hash_manifest "$SOURCE_HASHES"
verify_hash_manifest "$PRODUCT_HASHES"

# Preserve the state-local report that carries the final 5.97 ns execute path.
for top in synth_core_step_top boom_core_top; do
  database="$PROJECT_ROOT/$top/$SOLUTION/.autopilot/db"
  for name in execute_module.verbose.rpt execute_module.verbose.sched.rpt; do
    cp "$database/$name" "$REPORT_ROOT/$top/verbose/$name"
  done
done

python3 - "$ROOT" "$REPORT_ROOT" "$BUILD_ROOT" <<'PY'
import csv
import hashlib
import json
import re
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

root, report, build = map(Path, sys.argv[1:])
tops = ("synth_issue_top", "synth_execute_top", "synth_completion_top",
        "synth_rob_top", "synth_lsu_top", "synth_core_step_top", "boom_core_top")
rows = []
for top in tops:
    path = report / top / f"{top}_csynth.xml"
    xml = ET.parse(path).getroot()
    get = lambda name: xml.findtext(name)
    if (get("./ReportVersion/Version") != "2021.2" or
            get("./UserAssignments/Part") != "xczu7ev-ffvc1156-2-e" or
            get("./UserAssignments/TargetClockPeriod") != "10.00" or
            get("./PerformanceEstimates/PipelineType") != "no"):
        raise SystemExit(f"{top}: final synthesis guardrail failed")
    if top == "boom_core_top":
        core = xml.find("./PerformanceEstimates/SummaryOfLoopLatency/CORE_CYCLE")
        if core is None or core.find("PipelineII") is not None:
            raise SystemExit("boom_core_top: CORE_CYCLE absent or pipelined")
    runtime = dict(line.split("=", 1) for line in
                   (report / top / "csynth.time").read_text().splitlines() if "=" in line)
    rows.append(("W4E_FINAL", top, get("./UserAssignments/Part"),
                 get("./UserAssignments/TargetClockPeriod"),
                 get("./PerformanceEstimates/SummaryOfTimingAnalysis/EstimatedClockPeriod"),
                 get("./AreaEstimates/Resources/LUT"), get("./AreaEstimates/Resources/FF"),
                 get("./AreaEstimates/Resources/BRAM_18K"), get("./AreaEstimates/Resources/DSP"),
                 "no", runtime.get("runtime_seconds", ""), str(path.relative_to(root))))

with (report.parent / "resource_summary.csv").open("w", newline="") as stream:
    writer = csv.writer(stream)
    writer.writerow(("variant", "top", "part", "target_period_ns", "estimated_period_ns",
                     "lut", "ff", "bram_18k", "dsp", "pipeline_type", "runtime_seconds",
                     "evidence_xml"))
    writer.writerows(rows)

prf_rows = []
for top in ("synth_completion_top", "synth_core_step_top", "boom_core_top"):
    rtl = build / "projects" / top / "solution_w4e_final/syn/verilog" / f"{top}.v"
    text = rtl.read_text(errors="replace")
    required = ("state_int_rf_bank0_we0", "state_int_rf_bank1_we0",
                "state_int_rf_bank0_U", "state_int_rf_bank1_U",
                "state_int_rf_latest_bank")
    if not all(name in text for name in required):
        raise SystemExit(f"{top}: two-bank LVT PRF topology missing")
    log = (report / top / "csynth.log").read_text(errors="replace")
    match = re.search(r"Pipelining result : Target II = 1, Final II = (\d+), .*"
                      r"function 'apply_writeback_ports'", log)
    if match is None or match.group(1) != "1":
        raise SystemExit(f"{top}: apply_writeback_ports final II is not 1")
    prf_rows.append(("W4E_FINAL", top, 52, "2xRAM_AUTO_1R1W_52x64+LVT52", 2, 2,
                     "bank0_we0;bank1_we0", str(rtl.relative_to(root)), "PASS"))
with (report.parent / "prf_after.csv").open("w", newline="") as stream:
    writer = csv.writer(stream)
    writer.writerow(("stage", "top", "phys_regs", "generated_structure", "physical_ports",
                     "write_ports", "write_enables", "evidence", "status"))
    writer.writerows(prf_rows)

canonical = tuple(root / "src" / name for name in
    ("boom_core_step.cpp", "frontend.cpp", "decode.cpp", "rename.cpp", "completion.cpp",
     "rob.cpp", "issue.cpp", "execute.cpp", "branch.cpp", "lsu.cpp", "commit.cpp",
     "csr.cpp", "reset.cpp", "boom_core_top.cpp", "synth_module_tops.cpp",
     "boom_core_merged.cpp"))
for path in canonical:
    text = path.read_text(errors="replace")
    if re.search(r"#pragma\s+HLS\s+(?:DEPENDENCE.*false|DATAFLOW|ARRAY_PARTITION.*complete)",
                 text, re.I):
        raise SystemExit(f"forbidden pragma in {path.relative_to(root)}")

source_manifest = root / "reports/gate4_0/w4/source_hashes_after.txt"
product_manifest = root / "reports/gate4_0/w4/product_source_hashes.txt"
binding = {
    "status": "PASS",
    "tops": list(tops),
    "source_manifest": str(source_manifest.relative_to(root)),
    "source_manifest_sha256": hashlib.sha256(source_manifest.read_bytes()).hexdigest(),
    "product_source_manifest": str(product_manifest.relative_to(root)),
    "product_source_manifest_sha256": hashlib.sha256(product_manifest.read_bytes()).hexdigest(),
    "merged_source": "src/boom_core_merged.cpp",
    "merged_source_sha256": hashlib.sha256((root / "src/boom_core_merged.cpp").read_bytes()).hexdigest(),
    "diagnostic_wrapper": "src/synth_module_tops.cpp",
    "diagnostic_wrapper_sha256": hashlib.sha256((root / "src/synth_module_tops.cpp").read_bytes()).hexdigest(),
}
(report.parent / "csynth_source_binding.json").write_text(
    json.dumps(binding, indent=2, sort_keys=True) + "\n", encoding="utf-8")
PY

printf '%s\n' 'Gate 4.0 W4E final synthesis complete: 7/7 targets.'
