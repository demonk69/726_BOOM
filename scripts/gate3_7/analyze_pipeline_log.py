#!/usr/bin/env python3
"""Summarize isolated Gate 3.7 pipeline synthesis attempts."""

from __future__ import annotations

import argparse
import csv
import re
from pathlib import Path


BASELINE = {"lut": 83286, "ff": 16611, "bram": 16, "dsp": 3, "period": 5.898}
VARIANT_ORDER = [
    "P0_BASELINE",
    "P1_PIPELINE_NO_II",
    "P2_PIPELINE_II_16",
    "P3_PIPELINE_II_8",
    "P4_PIPELINE_II_4",
    "P5_PIPELINE_II_2",
    "P6_PIPELINE_II_1",
]


def read_text(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="replace") if path.exists() else ""


def cells(line: str) -> list[str]:
    return [cell.strip() for cell in line.strip().strip("|").split("|")]


def parse_time(path: Path) -> tuple[str, str]:
    text = read_text(path)
    runtime = re.search(r"runtime_seconds=([0-9.]+)", text)
    memory = re.search(r"peak_memory_kb=(\d+)", text)
    return (runtime.group(1) if runtime else "", memory.group(1) if memory else "")


def parse_report(path: Path) -> dict[str, str | int]:
    result: dict[str, str | int] = {
        "period": "", "lut": "", "ff": "", "bram": "", "dsp": "",
        "latency_min": "", "latency_max": "", "interval_min": "", "interval_max": "",
        "iteration_latency": "", "achieved_ii": "", "target_ii": "", "pipelined": "",
    }
    text = read_text(path)
    period = re.search(r"\|ap_clk\s*\|\s*[^|]+\|\s*([0-9.]+) ns", text)
    if period:
        result["period"] = period.group(1)
    for line in text.splitlines():
        row = cells(line)
        if len(row) >= 6 and row[0] == "Total":
            try:
                result.update({"bram": int(row[1]), "dsp": int(row[2]), "ff": int(row[3]), "lut": int(row[4])})
                break
            except ValueError:
                pass
    for line in text.splitlines():
        if "CORE_CYCLE" not in line or "|" not in line:
            continue
        row = cells(line)
        if len(row) >= 8:
            result.update({
                "latency_min": row[1], "latency_max": row[2], "iteration_latency": row[3],
                "achieved_ii": row[4], "target_ii": row[5], "pipelined": row[7],
            })
            break
    return result


def log_metrics(text: str) -> dict[str, str | int | list[str]]:
    finished = re.findall(r"Finished ([^:\n]+):", text)
    warnings = [line.strip() for line in text.splitlines() if "WARNING:" in line]
    conflicts = [line.strip() for line in text.splitlines() if re.search(r"memory port|port conflict|Unable to schedule.*(?:load|store)", line, re.IGNORECASE)]
    dependencies = [line.strip() for line in text.splitlines() if re.search(r"dependenc|carried constraint|recurrence", line, re.IGNORECASE)]
    ii_violations = [line.strip() for line in text.splitlines() if re.search(r"Unable to satisfy pipeline|II violation", line, re.IGNORECASE)]
    allocated_mb = []
    for value, unit in re.findall(r"current allocated memory: ([0-9.]+) (KB|MB|GB)", text):
        scale = {"KB": 1.0 / 1024.0, "MB": 1.0, "GB": 1024.0}[unit]
        allocated_mb.append(float(value) * scale)
    last_pass = finished[-1] if finished else "NO_COMPLETED_PASS"
    last_pass = re.sub(r" CPU user time.*$", "", last_pass)
    nonempty = [line.strip() for line in text.splitlines() if line.strip()]
    last_operation = nonempty[-1] if nonempty else "NO_LOG_MESSAGE"
    stage = "PRESYN2_IF_CONVERSION" if "Performing if-conversion" in last_operation else "NOT_CLASSIFIED"
    return {
        "last_pass": last_pass,
        "last_operation": last_operation,
        "stage_classification": stage,
        "warning_count": len(warnings),
        "automatic_partitions": text.count("Partitioning array '"),
        "inline_count": len(re.findall(r"Inlining function", text)),
        "memory_promotion_count": len(re.findall(r"memory promotion|Promoting memory", text, re.IGNORECASE)),
        "implied_unroll_count": text.count("complete unroll implied by the pipeline pragma"),
        "completed_unroll_count": len(re.findall(r"Unrolling loop .* completely with a factor", text)),
        "unroll_all_function_count": text.count("Unrolling all loops for pipelining in function"),
        "cannot_unroll_count": text.count("Cannot unroll loop"),
        "hls_max_current_allocated_mb": f"{max(allocated_mb):.3f}" if allocated_mb else "",
        "memory_port_conflict_count": len(conflicts),
        "dependency_message_count": len(dependencies),
        "ii_violation_count": len(ii_violations),
        "interesting": (ii_violations + conflicts + dependencies + warnings)[:40],
    }


def value_delta(value: str | int, baseline: int | float) -> str:
    if value == "":
        return ""
    return str(float(value) - baseline) if isinstance(baseline, float) else str(int(value) - baseline)


def write_csv(path: Path, fields: list[str], rows: list[dict[str, object]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=fields)
        writer.writeheader()
        writer.writerows(rows)


def analyze(args: argparse.Namespace) -> None:
    root = Path(args.root)
    out = root / "reports/gate3_7/variants" / args.variant
    out.mkdir(parents=True, exist_ok=True)
    log_path = out / "csynth.log"
    report_path = out / "boom_core_top_csynth.rpt"
    if not log_path.exists():
        log_path.write_text(f"{args.variant}: {args.status}; no synthesis process was started.\n", encoding="utf-8")
    log = read_text(log_path)
    report = parse_report(report_path)
    metrics = log_metrics(log)
    runtime, wrapper_peak_memory = parse_time(out / "csynth.time")
    peak_memory = "" if args.status == "TIMEOUT" else wrapper_peak_memory

    if args.variant == "P0_BASELINE" and args.status == "PASS":
        decision = "BASELINE_REFERENCE"
    elif args.status in ("PASS", "REPORT_WITH_NONZERO_EXIT"):
        decision = "SYNTHESIS_CANDIDATE_PENDING_RTL_VALIDATION"
    elif args.status == "TIMEOUT":
        decision = "TIMEOUT_NO_REPORT"
    else:
        decision = args.status

    cosim = "NOT_APPLICABLE_BASELINE" if args.variant == "P0_BASELINE" else "NOT_RUN_NO_VERIFIED_PIPELINE_RTL"
    reset = "SOURCE_RESET_RETAINED_RTL_NOT_TESTED" if args.status in ("PASS", "REPORT_WITH_NONZERO_EXIT") else "NOT_RUN"
    trace = "BASELINE_GATE3_6_PASS" if args.variant == "P0_BASELINE" else "NOT_RUN_RTL"
    achieved_ii = "" if report["achieved_ii"] in ("", "-", "?") else report["achieved_ii"]
    latency = "unknown" if report["latency_min"] == "?" else (f"{report['latency_min']}..{report['latency_max']}" if report["latency_min"] else "")
    interval = "unknown" if report["interval_min"] == "?" else (f"{report['interval_min']}..{report['interval_max']}" if report["interval_min"] else "")
    not_run = args.status.startswith("NOT_RUN")
    scheduler_evidence = args.status in ("PASS", "REPORT_WITH_NONZERO_EXIT")
    row = {
        "variant": args.variant,
        "pipeline_location": "none" if args.variant == "P0_BASELINE" else "boom_core_top/CORE_CYCLE",
        "requested_ii": args.requested_ii,
        "achieved_ii": achieved_ii,
        "latency": latency,
        "interval": interval,
        "csynth_status": args.status,
        "runtime_seconds": runtime,
        "peak_memory_kb": peak_memory,
        "last_pass": metrics["last_pass"],
        "estimated_period_ns": report["period"],
        "lut": report["lut"], "ff": report["ff"], "bram": report["bram"], "dsp": report["dsp"],
        "automatic_partitions": "" if not_run else metrics["automatic_partitions"],
        "memory_port_conflicts": metrics["memory_port_conflict_count"] if scheduler_evidence else "NOT_REPORTED",
        "loop_carried_dependencies": "REAL_DEPENDENCIES_INVENTORIED",
        "dependency_violations": metrics["ii_violation_count"] if scheduler_evidence else "NOT_REPORTED",
        "false_dependence_directives": 0,
        "cosim_status": cosim,
        "rtl_reset_status": reset,
        "trace_status": trace,
        "decision": decision,
    }
    write_csv(out / "result.csv", list(row.keys()), [row])

    summary = [
        f"# {args.variant} Csynth Summary", "",
        f"Status: `{args.status}`.", "",
        "| Metric | Value |", "|---|---:|",
        f"| Requested II | {args.requested_ii} |",
        f"| Achieved II | {report['achieved_ii'] or 'not reported'} |",
        f"| CORE_CYCLE pipelined | {report['pipelined'] or 'not reported'} |",
        f"| Iteration latency | {report['iteration_latency'] or 'not reported'} |",
        f"| Runtime | {runtime or 'not captured'} s |",
        f"| Peak memory | {peak_memory or 'not captured'} KB |",
        f"| Timeout-wrapper peak RSS | {wrapper_peak_memory or 'not captured'} KB |",
        f"| Maximum reported HLS current allocation | {metrics['hls_max_current_allocated_mb'] or 'not captured'} MB |",
        f"| Last completed pass | {metrics['last_pass']} |",
        f"| Estimated period | {report['period'] or 'not reported'} ns |",
        f"| LUT | {report['lut'] or 'not reported'} |",
        f"| FF | {report['ff'] or 'not reported'} |",
        f"| BRAM_18K | {report['bram'] or 'not reported'} |",
        f"| DSP | {report['dsp'] or 'not reported'} |",
        f"| Automatic partitions | {metrics['automatic_partitions']} |",
        f"| Decision | `{decision}` |", "",
        "No `DEPENDENCE false`, reset removal, state replication, capacity reduction, or source behavior change was used.",
    ]
    (out / "csynth_summary.md").write_text("\n".join(summary) + "\n", encoding="utf-8")

    schedule = [
        f"# {args.variant} Schedule Analysis", "",
        f"- Requested II: `{args.requested_ii}`",
        f"- Achieved II: `{report['achieved_ii'] or 'NOT_REPORTED'}`",
        f"- Target II in report: `{report['target_ii'] or 'NOT_REPORTED'}`",
        f"- CORE_CYCLE pipelined: `{report['pipelined'] or 'NOT_REPORTED'}`",
        f"- Last completed pass: `{metrics['last_pass']}`",
        f"- Stage classification: `{metrics['stage_classification']}`",
        f"- Last observable operation: `{metrics['last_operation']}`",
        f"- Warning count: {metrics['warning_count']}",
        f"- Automatic partition count: {metrics['automatic_partitions']}",
        f"- Inlining records: {metrics['inline_count']}",
        f"- Memory-promotion records: {metrics['memory_promotion_count']}",
        f"- Loops marked implied complete-unroll: {metrics['implied_unroll_count']}",
        f"- Completed loop-unroll records: {metrics['completed_unroll_count']}",
        f"- Functions marked unroll-all for pipelining: {metrics['unroll_all_function_count']}",
        f"- Incomplete variable-bound unrolls: {metrics['cannot_unroll_count']}",
        f"- Maximum reported HLS current allocation: {metrics['hls_max_current_allocated_mb'] or 'NOT_CAPTURED'} MB",
        f"- Memory-port conflict records: {metrics['memory_port_conflict_count']}",
        f"- Dependency-message records: {metrics['dependency_message_count']}",
        f"- II-violation records: {metrics['ii_violation_count']}", "",
        "## Relevant Messages", "",
    ]
    schedule.extend(f"- `{line}`" for line in metrics["interesting"])
    if not metrics["interesting"]:
        schedule.append("- No scheduling/dependency message was reached or reported.")
    (out / "schedule_analysis.md").write_text("\n".join(schedule) + "\n", encoding="utf-8")

    cosim_lines = [
        f"# {args.variant} Cosim Summary", "",
        f"Status: `{cosim}`.", "",
        "C/RTL cosim, pin-level mid-run reset, and directed AXIS backpressure were not run by csynth. A pipeline variant cannot be functionally accepted from C simulation or synthesis alone.",
    ]
    (out / "cosim_summary.md").write_text("\n".join(cosim_lines) + "\n", encoding="utf-8")

    resources = []
    for metric, key in (("lut", "lut"), ("ff", "ff"), ("bram_18k", "bram"), ("dsp", "dsp"), ("estimated_period_ns", "period")):
        value = report[key]
        base_key = "period" if key == "period" else key
        resources.append({"metric": metric, "accepted_baseline": BASELINE[base_key], "variant_value": value, "delta": value_delta(value, BASELINE[base_key])})
    write_csv(out / "resource_delta.csv", ["metric", "accepted_baseline", "variant_value", "delta"], resources)


def aggregate(root: Path) -> None:
    fields = [
        "variant", "pipeline_location", "requested_ii", "achieved_ii", "latency", "interval",
        "csynth_status", "runtime_seconds", "peak_memory_kb", "last_pass", "estimated_period_ns",
        "lut", "ff", "bram", "dsp", "automatic_partitions", "memory_port_conflicts",
        "loop_carried_dependencies", "dependency_violations", "false_dependence_directives",
        "cosim_status", "rtl_reset_status", "trace_status", "decision",
    ]
    rows: list[dict[str, object]] = []
    for variant in VARIANT_ORDER:
        path = root / "reports/gate3_7/variants" / variant / "result.csv"
        if path.exists():
            with path.open(newline="", encoding="utf-8") as handle:
                rows.extend(csv.DictReader(handle))
    write_csv(root / "reports/gate3_7/pipeline_variant_summary.csv", fields, rows)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", required=True)
    parser.add_argument("--variant")
    parser.add_argument("--requested-ii", default="")
    parser.add_argument("--status", default="")
    parser.add_argument("--exit-code", default="0")
    parser.add_argument("--timeout-seconds", default="0")
    parser.add_argument("--aggregate", action="store_true")
    args = parser.parse_args()
    if args.aggregate:
        aggregate(Path(args.root))
    else:
        if not args.variant:
            parser.error("--variant is required unless --aggregate is used")
        analyze(args)


if __name__ == "__main__":
    main()
