#!/usr/bin/env python3
"""Collect key Vitis HLS csynth metrics from reports and logs."""

from __future__ import annotations

import argparse
import csv
import re
from pathlib import Path


MODULES = [
    "synth_frontend_top",
    "synth_decode_top",
    "synth_rename_top",
    "synth_rob_top",
    "synth_issue_top",
    "synth_execute_top",
    "synth_lsu_top",
    "synth_commit_top",
    "synth_core_step_top",
]


def read_text(path: Path) -> str:
    if not path.exists():
        return ""
    return path.read_text(encoding="utf-8", errors="replace")


def parse_time(path: Path) -> tuple[str, str]:
    runtime = ""
    peak = ""
    for line in read_text(path).splitlines():
        if line.startswith("runtime_seconds="):
            runtime = line.split("=", 1)[1]
        elif line.startswith("peak_memory_kb="):
            peak = line.split("=", 1)[1]
    return runtime, peak


def parse_log(path: Path) -> tuple[str, str]:
    last_pass = ""
    warnings = 0
    for line in read_text(path).splitlines():
        match = re.search(r"Finished ([^:]+):", line)
        if match:
            last_pass = match.group(1)
        if "WARNING:" in line:
            warnings += 1
    return last_pass, str(warnings)


def parse_report(path: Path) -> dict[str, str]:
    result = {
        "LUT": "",
        "FF": "",
        "BRAM": "",
        "DSP": "",
        "estimated_period": "",
        "latency_min": "",
        "latency_max": "",
        "interval_min": "",
        "interval_max": "",
    }
    lines = read_text(path).splitlines()
    for line in lines:
        if "|ap_clk" in line:
            cols = [col.strip() for col in line.strip().strip("|").split("|")]
            if len(cols) >= 3:
                result["estimated_period"] = cols[2]
        if line.strip().startswith("|Total"):
            cols = [col.strip() for col in line.strip().strip("|").split("|")]
            if len(cols) >= 6 and cols[1].replace("-", "").isdigit():
                result["BRAM"] = cols[1]
                result["DSP"] = cols[2]
                result["FF"] = cols[3]
                result["LUT"] = cols[4]
                break
    for idx, line in enumerate(lines):
        if "|  Latency (cycles) |" in line:
            for candidate in lines[idx + 1:idx + 8]:
                cols = [col.strip() for col in candidate.strip().strip("|").split("|")]
                if len(cols) >= 7 and cols[0] not in ("", "Latency (cycles)", "min") and not candidate.strip().startswith("+"):
                    result["latency_min"] = cols[0]
                    result["latency_max"] = cols[1]
                    result["interval_min"] = cols[4]
                    result["interval_max"] = cols[5]
                    return result
    return result


def collect_module_rows(root: Path, gate: str) -> list[dict[str, str]]:
    rows = []
    for module in MODULES:
        project = root / f"boom_hls_{gate}_{module}"
        report = project / "solution_module" / "syn" / "report" / f"{module}_csynth.rpt"
        log = root / "reports" / gate / "module_csynth" / f"{module}.log"
        time_log = root / "reports" / gate / "module_csynth" / f"{module}.time"
        runtime, peak = parse_time(time_log)
        last_pass, warnings = parse_log(log)
        metrics = parse_report(report)
        row = {
            "module": module,
            "status": "PASS" if report.exists() else "FAIL",
            "runtime": runtime,
            "peak_memory": peak,
            "LUT": metrics["LUT"],
            "FF": metrics["FF"],
            "BRAM": metrics["BRAM"],
            "DSP": metrics["DSP"],
            "estimated_period": metrics["estimated_period"],
            "last_pass": last_pass,
            "warnings": warnings,
            "report_path": str(report),
            "log_path": str(log),
        }
        rows.append(row)
    return rows


def write_module_csv(root: Path, gate: str, output: Path) -> None:
    rows = collect_module_rows(root, gate)
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(rows[0].keys()))
        writer.writeheader()
        writer.writerows(rows)


def write_single_md(report: Path, log: Path, time_log: Path, output: Path, title: str) -> None:
    runtime, peak = parse_time(time_log)
    last_pass, warnings = parse_log(log)
    metrics = parse_report(report)
    status = "PASS" if report.exists() else "FAIL"
    lines = [
        f"# {title}",
        "",
        f"Status: {status}",
        "",
        "| Metric | Value |",
        "|---|---:|",
        f"| Runtime seconds | {runtime or 'unknown'} |",
        f"| Peak memory KB | {peak or 'unknown'} |",
        f"| Estimated period | {metrics['estimated_period'] or 'unknown'} |",
        f"| LUT | {metrics['LUT'] or 'unknown'} |",
        f"| FF | {metrics['FF'] or 'unknown'} |",
        f"| BRAM_18K | {metrics['BRAM'] or 'unknown'} |",
        f"| DSP | {metrics['DSP'] or 'unknown'} |",
        f"| Latency min | {metrics['latency_min'] or 'unknown'} |",
        f"| Latency max | {metrics['latency_max'] or 'unknown'} |",
        f"| Interval min | {metrics['interval_min'] or 'unknown'} |",
        f"| Interval max | {metrics['interval_max'] or 'unknown'} |",
        f"| Last HLS pass | {last_pass or 'unknown'} |",
        f"| Warning count | {warnings} |",
        "",
        f"Report: `{report}`",
        "",
        f"Log: `{log}`",
    ]
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", default=Path(__file__).resolve().parents[1], type=Path)
    parser.add_argument("--gate", default="gate3_3")
    parser.add_argument("--module-output", type=Path)
    parser.add_argument("--single-report", type=Path)
    parser.add_argument("--single-log", type=Path)
    parser.add_argument("--single-time", type=Path)
    parser.add_argument("--single-output", type=Path)
    parser.add_argument("--single-title", default="Csynth Summary")
    args = parser.parse_args()

    root = args.root.resolve()
    if args.module_output:
        write_module_csv(root, args.gate, args.module_output)
    if args.single_output:
        if not (args.single_report and args.single_log and args.single_time):
            raise SystemExit("single summary requires --single-report, --single-log, and --single-time")
        write_single_md(args.single_report, args.single_log, args.single_time, args.single_output, args.single_title)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
