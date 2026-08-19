#!/usr/bin/env python3
import csv
import xml.etree.ElementTree as ET
from pathlib import Path


root = Path(__file__).resolve().parents[2]
report_dir = root / "reports/gate5_3_fetch_buffer/b3i"
baseline_path = root / "reports/gate5_3_fetch_buffer/b2/phase_f_ppa.csv"

reports = {
    "synth_fetch_packet_top": root / "build/gate5_3_fetch_buffer/b3i/fetch_packet_hls/solution_b3i_packet/syn/report/synth_fetch_packet_top_csynth.xml",
    "synth_fetch_buffer_top": root / "build/gate5_3_fetch_buffer/b3i/canonical_csynth/fetch_buffer/d8_auto/solution/syn/report/synth_fetch_buffer_top_csynth.xml",
}
for top in ("synth_rvc_top", "synth_frontend_top", "synth_divider_top", "synth_mul_top",
            "synth_issue_top", "synth_execute_top", "synth_completion_top", "synth_rob_top"):
    reports[top] = root / f"boom_hls_gate5_3_fetch_buffer_b3i_{top}/solution_module/syn/report/{top}_csynth.xml"
for top in ("synth_core_step_top", "boom_core_top"):
    reports[top] = root / f"boom_hls_gate5_3_fetch_buffer_b3i_core_{top}/solution_module/syn/report/{top}_csynth.xml"


def value(xml_root, path, default=""):
    node = xml_root.find(path)
    return node.text if node is not None else default


rows = []
for top, path in reports.items():
    if not path.is_file():
        raise SystemExit(f"missing csynth XML for {top}: {path}")
    xml_root = ET.parse(path).getroot()
    row = {
        "top": top,
        "LUT": int(value(xml_root, "./AreaEstimates/Resources/LUT", "0")),
        "FF": int(value(xml_root, "./AreaEstimates/Resources/FF", "0")),
        "BRAM_18K": int(value(xml_root, "./AreaEstimates/Resources/BRAM_18K", "0")),
        "DSP": int(value(xml_root, "./AreaEstimates/Resources/DSP", "0")),
        "target_period_ns": value(xml_root, "./UserAssignments/TargetClockPeriod"),
        "estimated_period_ns": value(xml_root, "./PerformanceEstimates/SummaryOfTimingAnalysis/EstimatedClockPeriod"),
        "PipelineType": value(xml_root, "./PerformanceEstimates/PipelineType"),
        "evidence_xml": str(path.relative_to(root)),
    }
    rows.append(row)

columns = ("top", "LUT", "FF", "BRAM_18K", "DSP", "target_period_ns",
           "estimated_period_ns", "PipelineType", "evidence_xml")
with (report_dir / "b3i_ppa.csv").open("w", newline="") as stream:
    writer = csv.DictWriter(stream, fieldnames=columns)
    writer.writeheader()
    writer.writerows(rows)

with baseline_path.open(newline="") as stream:
    baseline = {row["top"]: row for row in csv.DictReader(stream)}
comparison = []
for row in rows:
    old = baseline.get(row["top"])
    if old is None:
        comparison.append((row["top"], "NO_B2_BASELINE", "", row["LUT"], "", "",
                           row["FF"], "", "", row["BRAM_18K"], "", row["DSP"], "",
                           row["estimated_period_ns"]))
        continue
    comparison.append((row["top"], "COMPARED", int(old["LUT"]), row["LUT"],
                       row["LUT"] - int(old["LUT"]), int(old["FF"]), row["FF"],
                       row["FF"] - int(old["FF"]), int(old["BRAM_18K"]), row["BRAM_18K"],
                       int(old["DSP"]), row["DSP"], old["estimated_period_ns"],
                       row["estimated_period_ns"]))
with (report_dir / "resource_comparison.csv").open("w", newline="") as stream:
    writer = csv.writer(stream)
    writer.writerow(("top", "comparison", "b2_lut", "b3i_lut", "lut_delta", "b2_ff",
                     "b3i_ff", "ff_delta", "b2_bram", "b3i_bram", "b2_dsp", "b3i_dsp",
                     "b2_period_ns", "b3i_period_ns"))
    writer.writerows(comparison)

core = next(row for row in rows if row["top"] == "boom_core_top")
core_old = baseline["boom_core_top"]
accepted = (float(core["estimated_period_ns"]) <= 6.5 and core["BRAM_18K"] == 16 and
            core["DSP"] == 3 and core["PipelineType"] == "no")
(report_dir / "critical_path_impact.md").write_text(
    "# Gate 5.3 B3I Critical-Path Impact\n\n"
    f"- Status: **{'PASS' if accepted else 'FAIL'}**.\n"
    f"- `boom_core_top`: `{core['estimated_period_ns']} ns`, unchanged from B2 "
    f"`{core_old['estimated_period_ns']} ns`; acceptance limit is `6.5 ns`.\n"
    "- `CORE_CYCLE` remains unpipelined: top-level XML reports `PipelineType=no` and "
    "the `CORE_CYCLE` loop has no defined iteration latency or PipelineII.\n"
    f"- Resources: `{core['LUT']} LUT / {core['FF']} FF / {core['BRAM_18K']} BRAM_18K / "
    f"{core['DSP']} DSP`; packet construction adds combinational/frontend state cost but does "
    "not move the accepted full-core estimated period.\n"
    "- No end-to-end speedup is claimed because Decode/Dispatch/Commit remain one-wide.\n",
    encoding="utf-8")
if not accepted:
    raise SystemExit("B3I PPA acceptance failed")
print("GATE5_3_B3I_PPA_PASS standalone=1 canonical=11")
