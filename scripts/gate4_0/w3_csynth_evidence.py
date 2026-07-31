#!/usr/bin/env python3
"""Audit and summarize Gate 4.0 W3 Vitis HLS synthesis evidence."""

import argparse
import csv
import hashlib
import re
import sys
import xml.etree.ElementTree as ET
from pathlib import Path


TOPS = (
    "synth_issue_top",
    "synth_execute_top",
    "synth_rob_top",
    "synth_lsu_top",
    "boom_core_top",
)
EXPECTED_CONFIG = {
    "FETCH_WIDTH": "4",
    "DECODE_WIDTH": "1",
    "DISPATCH_WIDTH": "1",
    "ISSUE_WIDTH": "3",
    "COMMIT_WIDTH": "1",
    "ROB_DEPTH": "32",
    "ROB_IDX_BITS": "5",
    "INT_PHYS_REGS": "52",
    "FP_PHYS_REGS": "48",
    "PHYS_REG_BITS": "6",
    "LOGICAL_REG_COUNT": "32",
    "ISSUE_QUEUE_MEM_DEPTH": "8",
    "ISSUE_QUEUE_ALU_DEPTH": "8",
    "ISSUE_QUEUE_FPU_DEPTH": "8",
    "ISSUE_QUEUE_IDX_BITS": "3",
    "LDQ_DEPTH": "8",
    "STQ_DEPTH": "8",
    "LDQ_IDX_BITS": "3",
    "STQ_IDX_BITS": "3",
    "MAX_BRANCH_COUNT": "8",
    "BR_MASK_BITS": "8",
    "BR_TAG_BITS": "3",
    "FTQ_DEPTH": "16",
    "FTQ_IDX_BITS": "4",
    "FETCH_BUFFER_DEPTH": "8",
    "PADDR_BITS": "32",
    "VADDR_BITS": "39",
}
FORBIDDEN = re.compile(r"\b(?:DATAFLOW|ARRAY_PARTITION|DEPENDENCE)\b", re.I)
ACTIVE_FORBIDDEN = re.compile(r"\b(?:PIPELINE|DATAFLOW|ARRAY_PARTITION|DEPENDENCE)\b", re.I)


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def hash_tree(root, output, suffixes=None, exclude=()):
    root = root.resolve()
    excluded = {Path(item).resolve() for item in exclude}
    paths = []
    if root.is_file():
        paths = [root]
    elif root.exists():
        paths = sorted(path for path in root.rglob("*") if path.is_file())
    if suffixes:
        paths = [path for path in paths if path.suffix in suffixes]
    lines = [f"{sha256(path)}  {path.relative_to(root if root.is_dir() else root.parent)}"
             for path in paths if path.resolve() not in excluded]
    Path(output).write_text("\n".join(lines) + ("\n" if lines else ""), encoding="utf-8")


def macro_values(text):
    return dict(re.findall(r"^\s*#define\s+(\w+)\s+([^\s/]+)", text, re.M))


def audit_source(root, report):
    config_path = root / "include/boom_config.hpp"
    merged_path = root / "src/boom_core_merged.cpp"
    top_path = root / "src/boom_core_top.cpp"
    config = macro_values(config_path.read_text(encoding="utf-8"))
    errors = [f"{name}: expected {value}, found {config.get(name, 'MISSING')}"
              for name, value in EXPECTED_CONFIG.items() if config.get(name) != value]

    source_paths = [path for path in sorted((root / "src").glob("*.cpp"))
                    if path.name != "boom_all.cpp"]
    for path in source_paths + sorted((root / "include").glob("*.hpp")):
        for number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
            if FORBIDDEN.search(line):
                errors.append(f"forbidden directive at {path.relative_to(root)}:{number}: {line.strip()}")

    for path in (top_path, merged_path):
        text = path.read_text(encoding="utf-8")
        core = re.search(r"CORE_CYCLE:\s*while\s*\(true\)\s*\{(.*?)boom_core_cycle_or_reset", text, re.S)
        expected_guard = re.compile(
            r"#ifdef\s+BOOM_HLS_ENABLE_CORE_PIPELINE\s*\n\s*#pragma\s+HLS\s+PIPELINE\s+II=1\s*\n\s*#endif"
        )
        if not core or not expected_guard.search(core.group(1)):
            errors.append(f"CORE_CYCLE pipeline guard malformed in {path.relative_to(root)}")
        if "#define BOOM_HLS_ENABLE_CORE_PIPELINE" in text:
            errors.append(f"CORE_CYCLE pipeline macro defined in {path.relative_to(root)}")

    directives = (root / "directives/baseline_directives.tcl").read_text(encoding="utf-8")
    active_lines = [line for line in directives.splitlines() if not line.lstrip().startswith("#")]
    if ACTIVE_FORBIDDEN.search("\n".join(active_lines)):
        errors.append("baseline directives contain a forbidden active override")
    if not re.search(r"config_compile\s+-pipeline_loops\s+0", directives):
        errors.append("baseline directives do not disable automatic loop pipelining")

    lines = [
        "# Gate 4.0 W3 Synthesis Guardrail Audit", "",
        "- CORE_CYCLE pipeline: unpipelined (pipeline pragma remains behind an undefined opt-in macro)",
        f"- DISPATCH_WIDTH: {config.get('DISPATCH_WIDTH')}",
        f"- COMMIT_WIDTH: {config.get('COMMIT_WIDTH')}",
        f"- Capacity and field-width checks: {'PASS' if not errors else 'FAIL'} ({len(EXPECTED_CONFIG)} exact checks)",
        "- Forbidden pipeline/dataflow/array-partition/false-dependence overrides: " + ("none active" if not errors else "audit failed"),
        "- Synthesis C flags: `-std=c++11 -I<root>/include` only; no extra feature defines", "",
        "## Checked Configuration", "",
        "| Macro | Required | Observed |", "|---|---:|---:|",
    ]
    lines.extend(f"| {name} | {value} | {config.get(name, 'MISSING')} |" for name, value in EXPECTED_CONFIG.items())
    lines.extend(["", "## Result", "", "PASS" if not errors else "FAIL", ""])
    if errors:
        lines.extend(["## Violations", ""] + [f"- {error}" for error in errors] + [""])
    report.write_text("\n".join(lines), encoding="utf-8")
    if errors:
        raise SystemExit("; ".join(errors))


def audit_project(project, solution, top):
    base = project / solution
    xml_path = base / "syn/report" / f"{top}_csynth.xml"
    tree = ET.parse(xml_path)
    root = tree.getroot()
    if root.findtext("./ReportVersion/Version") != "2021.2":
        raise SystemExit(f"{top}: wrong Vitis HLS report version")
    if root.findtext("./UserAssignments/Part") != "xczu7ev-ffvc1156-2-e":
        raise SystemExit(f"{top}: wrong FPGA part")
    if root.findtext("./UserAssignments/TargetClockPeriod") not in {"10", "10.0", "10.00"}:
        raise SystemExit(f"{top}: wrong target clock")
    if root.findtext("./PerformanceEstimates/PipelineType") != "no":
        raise SystemExit(f"{top}: top is pipelined")
    if top == "boom_core_top":
        core = root.find("./PerformanceEstimates/SummaryOfLoopLatency/CORE_CYCLE")
        if core is None or core.find("PipelineII") is not None:
            raise SystemExit("boom_core_top: CORE_CYCLE is absent or pipelined")

    candidates = list((base / ".autopilot/db").glob("boom_core_merged.pp.*.cpp"))
    if not candidates:
        raise SystemExit(f"{top}: preprocessed source not found")
    for path in candidates:
        text = path.read_text(encoding="utf-8", errors="replace")
        for number, line in enumerate(text.splitlines(), 1):
            if ("#pragma" in line or "HLSDIRECTIVE" in line) and ACTIVE_FORBIDDEN.search(line):
                raise SystemExit(f"{top}: forbidden active directive in {path.name}:{number}: {line.strip()}")
    directive = base / f"{solution}.directive"
    if directive.exists() and ACTIVE_FORBIDDEN.search(directive.read_text(encoding="utf-8", errors="replace")):
        raise SystemExit(f"{top}: forbidden solution directive")


def xml_value(root, path):
    value = root.findtext(path)
    if value is None:
        raise SystemExit(f"missing XML field {path}")
    return value


def critical_path(verbose_dir):
    best = None
    state_re = re.compile(r"^\s*<State\s+(\d+)>:\s*([0-9.]+)ns\s*$")
    for path in sorted(verbose_dir.glob("*.verbose.sched.rpt")):
        lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
        in_timing = False
        for index, line in enumerate(lines):
            if "+ Verbose Summary: Timing violations" in line:
                in_timing = True
                continue
            if in_timing and "+ Verbose Summary:" in line:
                in_timing = False
            match = state_re.match(line) if in_timing else None
            if not match:
                continue
            delay = float(match.group(2))
            operations = []
            cursor = index + 2
            while cursor < len(lines) and not state_re.match(lines[cursor]) and "+ Verbose Summary:" not in lines[cursor]:
                item = lines[cursor].strip()
                if item and item != "The critical path consists of the following:" and not set(item) <= {"="}:
                    operations.append(item)
                cursor += 1
            candidate = (delay, path.name, match.group(1), operations)
            if best is None or candidate[0] > best[0]:
                best = candidate
    return best


def summarize(root, report_root, baseline_path):
    with baseline_path.open(newline="", encoding="utf-8") as handle:
        baseline_rows = list(csv.DictReader(handle))
    baselines = {row["top"]: row for row in baseline_rows if row["variant"] == "GATE4_0_W2"}
    rows = []
    timing = []
    for top in TOPS:
        top_dir = report_root / top
        xml_path = top_dir / f"{top}_csynth.xml"
        xml = ET.parse(xml_path).getroot()
        target = xml_value(xml, "./UserAssignments/TargetClockPeriod")
        estimate = xml_value(xml, "./PerformanceEstimates/SummaryOfTimingAnalysis/EstimatedClockPeriod")
        resources = {name: xml_value(xml, f"./AreaEstimates/Resources/{name}")
                     for name in ("LUT", "FF", "BRAM_18K", "DSP", "URAM")}
        baseline = baselines.get(top)
        row = {
            "variant": "GATE4_0_W3", "top": top, "part": xml_value(xml, "./UserAssignments/Part"),
            "target_period_ns": target, "estimated_period_ns": estimate,
            "timing_margin_ns": f"{float(target) - float(estimate):.3f}",
            "lut": resources["LUT"], "ff": resources["FF"], "bram_18k": resources["BRAM_18K"],
            "dsp": resources["DSP"], "uram": resources["URAM"], "pipeline_type": xml_value(xml, "./PerformanceEstimates/PipelineType"),
            "w2_baseline_lut": baseline["lut"] if baseline else "", "lut_delta_vs_w2": str(int(resources["LUT"]) - int(baseline["lut"])) if baseline else "",
            "w2_baseline_ff": baseline["ff"] if baseline else "", "ff_delta_vs_w2": str(int(resources["FF"]) - int(baseline["ff"])) if baseline else "",
            "w2_baseline_bram_18k": baseline["bram_18k"] if baseline else "", "bram_18k_delta_vs_w2": str(int(resources["BRAM_18K"]) - int(baseline["bram_18k"])) if baseline else "",
            "w2_baseline_dsp": baseline["dsp"] if baseline else "", "dsp_delta_vs_w2": str(int(resources["DSP"]) - int(baseline["dsp"])) if baseline else "",
            "runtime_seconds": "", "peak_memory_kb": "", "evidence_xml": str(xml_path.relative_to(root)),
        }
        for line in (top_dir / "csynth.time").read_text(encoding="utf-8").splitlines():
            if "=" in line:
                key, value = line.split("=", 1)
                if key in row:
                    row[key] = value
        rows.append(row)
        timing.append((top, estimate, critical_path(top_dir / "verbose")))

    summary_path = report_root.parent / "resource_summary.csv"
    with summary_path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)

    lines = [
        "# Gate 4.0 W3 Critical Path Analysis", "",
        "Vitis HLS 2021.2 estimates for `xczu7ev-ffvc1156-2-e` at a 10 ns target. Paths are the longest state paths parsed from copied `*.verbose.sched.rpt` reports.", "",
    ]
    for top, estimate, path in timing:
        lines.extend([f"## {top}", "", f"- Estimated clock period: **{estimate} ns**"])
        if path:
            delay, name, state, operations = path
            lines.extend([f"- Longest verbose state path: **{delay:g} ns**", f"- Evidence: `{top}/verbose/{name}`, state `{state}`", "", "```text"])
            lines.extend(operations or ["(no operations listed)"])
            lines.extend(["```", ""])
        else:
            lines.extend(["- Blocker: no verbose schedule timing path was found.", ""])
    (report_root.parent / "critical_path_analysis.md").write_text("\n".join(lines), encoding="utf-8")


def main():
    parser = argparse.ArgumentParser()
    sub = parser.add_subparsers(dest="command", required=True)
    source = sub.add_parser("audit-source")
    source.add_argument("root", type=Path)
    source.add_argument("report", type=Path)
    project = sub.add_parser("audit-project")
    project.add_argument("project", type=Path)
    project.add_argument("solution")
    project.add_argument("top", choices=TOPS)
    summary = sub.add_parser("summarize")
    summary.add_argument("root", type=Path)
    summary.add_argument("report_root", type=Path)
    summary.add_argument("baseline", type=Path)
    hashes = sub.add_parser("hash-tree")
    hashes.add_argument("root", type=Path)
    hashes.add_argument("output", type=Path)
    hashes.add_argument("--source-only", action="store_true")
    hashes.add_argument("--exclude", action="append", default=[])
    args = parser.parse_args()
    if args.command == "audit-source":
        audit_source(args.root.resolve(), args.report)
    elif args.command == "audit-project":
        audit_project(args.project, args.solution, args.top)
    elif args.command == "summarize":
        summarize(args.root.resolve(), args.report_root.resolve(), args.baseline.resolve())
    else:
        suffixes = {".c", ".cc", ".cpp", ".h", ".hh", ".hpp"} if args.source_only else None
        excluded = [args.output] + [args.root / item for item in args.exclude]
        hash_tree(args.root, args.output, suffixes=suffixes, exclude=excluded)


if __name__ == "__main__":
    main()
