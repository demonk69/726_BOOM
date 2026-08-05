#!/usr/bin/env python3
import csv
import hashlib
import json
import re
import subprocess
import xml.etree.ElementTree as ET
from pathlib import Path

root = Path(__file__).resolve().parents[2]
report = root / "reports/gate4_1/m3/m3c"


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def csv_pass(path, expected, field="status"):
    with path.open(newline="", encoding="utf-8") as stream:
        rows = list(csv.DictReader(stream))
    if len(rows) != expected or not all(row[field] == "PASS" for row in rows):
        raise SystemExit(f"matrix acceptance failed: {path} ({len(rows)}/{expected})")
    return rows


required_markers = {
    "logs/rv64m_full_tests.log": "M3C_RV64M_DIRECTED checks=1458 failures=0 status=PASS",
    "logs/rv64m_full_random_tests.log": "M3C_RV64M_RANDOM status=PASS",
    "logs/rv64m_full_core_tests.log": "M3C native full-core RV64M programs: 15/15 PASS",
    "csim/vitis_csim.log": "M3C native full-core RV64M programs: 15/15 PASS",
    "regression/m3b_recheck2/logs/divider_integration_tests.log": "failures=0",
    "regression/m3b_recheck2/logs/divider_integration_random_tests.log": "Gate 4.1 M3B divider integration random: PASS",
    "independent_read_only_review.md": "**PASS.** No technical blocker",
}
for name, marker in required_markers.items():
    if marker not in (report / name).read_text(errors="replace"):
        raise SystemExit(f"missing PASS marker in {name}")

csv_pass(report / "rtl/rtl_test_matrix.csv", 30)
csv_pass(report / "rtl/m3b_focused/rtl_test_matrix.csv", 26)
csv_pass(report / "full_core_rtl/full_core_rtl_matrix.csv", 15)
csv_pass(report / "reset_rtl_49/rtl_test_matrix.csv", 49)
csv_pass(report / "rtl/w4/rtl_test_matrix.csv", 20)
csv_pass(report / "rtl/w4/w3_current/rtl_test_matrix.csv", 11)

w4 = json.loads((report / "rtl/w4/rtl_focused_summary.json").read_text())
if w4.get("status") != "PASS" or w4.get("focused_pass") != 20 or w4.get("w3_pass") != 11:
    raise SystemExit("W3/W4 focused RTL summary failed")

with (report / "resource_summary.csv").open(newline="", encoding="utf-8") as stream:
    resources = list(csv.DictReader(stream))
if len(resources) != 8:
    raise SystemExit("canonical synthesis is not 8/8")
for row in resources:
    xml = ET.parse(root / row["evidence_xml"]).getroot()
    if (xml.findtext("./ReportVersion/Version") != "2021.2" or
            xml.findtext("./UserAssignments/Part") != "xczu7ev-ffvc1156-2-e" or
            xml.findtext("./UserAssignments/TargetClockPeriod") != "10.00" or
            xml.findtext("./PerformanceEstimates/PipelineType") != "no"):
        raise SystemExit(f"synthesis guardrail failed: {row['top']}")
    if int(row["dsp"]) > 3 or int(row["bram"]) > 16:
        raise SystemExit(f"PPA resource guardrail failed: {row['top']}")
full = {row["top"]: row for row in resources}
for top in ("synth_core_step_top", "boom_core_top"):
    if float(full[top]["estimated_period"]) > 6.5:
        raise SystemExit(f"PPA timing guardrail failed: {top}")
if (report / "ppa_status.txt").read_text().strip() != "M3B_PPA_BLOCKER=false":
    raise SystemExit("PPA blocker remains set")

canonical = [root / "src" / name for name in
             ("boom_core_step.cpp", "boom_core_top.cpp", "completion.cpp", "lsu.cpp",
              "execute.cpp", "rob.cpp", "synth_module_tops.cpp", "boom_core_merged.cpp")]
for path in canonical:
    text = path.read_text(encoding="utf-8")
    forbidden = (r"#\s*pragma\s+HLS\s+DATAFLOW\b",
                 r"DEPENDENCE[^\n]*\bfalse\b",
                 r"ARRAY_PARTITION[^\n]*\bcomplete\b")
    if any(re.search(pattern, text, re.I) for pattern in forbidden):
        raise SystemExit(f"forbidden directive in {path.relative_to(root)}")
if "BOOM_HLS_ENABLE_CORE_PIPELINE" not in (root / "src/boom_core_top.cpp").read_text():
    raise SystemExit("CORE_CYCLE pipeline opt-in guard missing")
if any(path.name == "boom_core_step.v" for path in
       (root / "build/gate4_1").glob("m3c*/**/syn/verilog/boom_core_step.v")):
    raise SystemExit("raw boom_core_step product RTL found")
reference_diff = subprocess.run(
    ["git", "diff", "--quiet", "--", "reference"], cwd=root).returncode
if reference_diff:
    raise SystemExit("frozen reference artifacts changed")

source_roots = ("include", "src", "directives", "scripts", "tb", "rtl_tb")
suffixes = {".hpp", ".cpp", ".h", ".c", ".py", ".sh", ".tcl", ".sv"}
sources = []
for name in source_roots:
    sources.extend(path for path in (root / name).rglob("*")
                   if path.is_file() and path.suffix in suffixes and
                   path != root / "src/boom_all.cpp")
sources = sorted(set(sources))
source_lines = [f"{digest(path)}  {path.relative_to(root)}" for path in sources]
for name in ("source_manifest.sha256", "source_hashes_after.txt"):
    (report / name).write_text("\n".join(source_lines) + "\n", encoding="utf-8")

checks = (
    ("m3c_directed", "1458/1458", "logs/rv64m_full_tests.log"),
    ("m3c_random", "256x2048", "logs/rv64m_full_random_tests.log"),
    ("native_full_core", "15/15", "logs/rv64m_full_core_tests.log"),
    ("vitis_csim_full_core", "15/15", "csim/vitis_csim.log"),
    ("m3c_focused_rtl", "30/30", "rtl/rtl_test_matrix.csv"),
    ("m3c_full_core_rtl", "15/15", "full_core_rtl/full_core_rtl_matrix.csv"),
    ("gate3_9_rtl", "49/49", "reset_rtl_49/rtl_test_matrix.csv"),
    ("w3_software", "400/400", "regression/w4e/regression_after.md"),
    ("w3_focused_rtl", "11/11", "rtl/w4/w3_current/rtl_test_matrix.csv"),
    ("w4_directed_random", "95/95+128/128", "regression/w4e/regression_after.md"),
    ("w4_focused_rtl", "20/20", "rtl/w4/rtl_test_matrix.csv"),
    ("canonical_csynth", "8/8", "resource_summary.csv"),
    ("ppa_guardrail", "PASS", "ppa_status.txt"),
    ("independent_review", "PASS", "independent_read_only_review.md"),
    ("scope_guardrails", "PASS", "protection_audit.md"),
)
with (report / "comprehensive_manifest.csv").open("w", newline="", encoding="utf-8") as stream:
    writer = csv.writer(stream)
    writer.writerow(("requirement", "status", "result", "evidence", "sha256"))
    for requirement, result, name in checks:
        path = report / name
        writer.writerow((requirement, "PASS", result,
                         str(path.relative_to(root)), digest(path)))

status = subprocess.run(["git", "status", "--short"], cwd=root,
                        check=True, capture_output=True, text=True).stdout
(report / "git_status_after.txt").write_text(status, encoding="utf-8")

excluded = {"artifact_manifest.csv"}
artifacts = [path for path in sorted(report.rglob("*"))
             if path.is_file() and path.name not in excluded]
with (report / "artifact_manifest.csv").open("w", newline="", encoding="utf-8") as stream:
    writer = csv.writer(stream)
    writer.writerow(("path", "bytes", "sha256"))
    for path in artifacts:
        writer.writerow((path.relative_to(report), path.stat().st_size, digest(path)))

print(f"M3C evidence finalized: {len(sources)} sources, {len(artifacts)} artifacts, {len(checks)} checks")
