#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import re
from pathlib import Path


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="replace") if path.exists() else ""


def parse_report(path: Path) -> dict[str, str]:
    text = read(path)
    result = {"estimated_period_ns": "", "lut": "", "ff": "", "bram_18k": "", "dsp": ""}
    match = re.search(r"\|ap_clk\s*\|\s*[^|]+\|\s*([0-9.]+) ns", text)
    if match: result["estimated_period_ns"] = match.group(1)
    for line in text.splitlines():
        cells = [c.strip() for c in line.strip().strip("|").split("|")]
        if len(cells) >= 6 and cells[0] == "Total":
            result.update({"bram_18k": cells[1], "dsp": cells[2], "ff": cells[3], "lut": cells[4]})
            break
    return result


def time_values(path: Path) -> tuple[str, str]:
    runtime = peak = ""
    for line in read(path).splitlines():
        if line.startswith("runtime_seconds="): runtime = line.split("=", 1)[1]
        if line.startswith("peak_memory_kb="): peak = line.split("=", 1)[1]
    return runtime, peak


def append(args: argparse.Namespace) -> None:
    metrics = parse_report(args.report)
    runtime, peak = time_values(args.time_log)
    log = read(args.log)
    row = {"top": args.top, "status": args.status, "runtime_seconds": runtime, "peak_memory_kb": peak, **metrics, "automatic_partition_count": log.count("Partitioning array '"), "warning_count": log.count("WARNING:"), "report_path": str(args.report)}
    with args.append.open("a", encoding="utf-8", newline="") as handle:
        csv.DictWriter(handle, fieldnames=row.keys()).writerow(row)


def markdown(csv_path: Path, output: Path) -> None:
    with csv_path.open(encoding="utf-8", newline="") as handle:
        rows = list(csv.DictReader(handle))
    lines = ["# Gate 3.6 N-cycle Resource Scaling", "", "| Top | Status | LUT | FF | BRAM | DSP | Period ns | Partitions | Runtime s |", "|---|---|---:|---:|---:|---:|---:|---:|---:|"]
    for row in rows:
        lines.append(f"| {row['top']} | {row['status']} | {row['lut']} | {row['ff']} | {row['bram_18k']} | {row['dsp']} | {row['estimated_period_ns']} | {row['automatic_partition_count']} | {row['runtime_seconds']} |")
    lines.extend(["", "No N-cycle top is a product replacement. The experiment isolates fixed loop trip count and retained wrapper effects with pipeline and unroll disabled."])
    output.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--append", type=Path)
    parser.add_argument("--top")
    parser.add_argument("--status")
    parser.add_argument("--report", type=Path)
    parser.add_argument("--log", type=Path)
    parser.add_argument("--time-log", type=Path)
    parser.add_argument("--markdown", nargs=2, metavar=("CSV", "MD"), type=Path)
    args = parser.parse_args()
    if args.append:
        append(args)
    elif args.markdown:
        markdown(args.markdown[0], args.markdown[1])
    else:
        parser.error("--append or --markdown is required")


if __name__ == "__main__":
    main()
