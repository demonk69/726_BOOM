#!/usr/bin/env python3
"""Inventory Vitis HLS automatic array partitioning from an HLS log."""

from __future__ import annotations

import argparse
import csv
import re
from collections import Counter
from pathlib import Path
from typing import Dict, Optional, Tuple


PARTITION_RE = re.compile(
    r"Partitioning array '([^']+)'(?: \(([^:()]+):(\d+)\))?(?: ([^.]+))?\."
)
FINISHED_RE = re.compile(r"Finished ([^:]+):.*Elapsed time: ([^;]+); current allocated memory: ([^\n]+)")


def element_count(array: str) -> str:
    known = [
        ("br_snapshots", "32x8"),
        ("rob.entries", "32"),
        ("int_rf", "52"),
        ("fp_rf", "48"),
        ("free_list", "52"),
        ("busy_table", "52"),
        ("alu_iq.entries", "8"),
        ("ldq", "8"),
        ("stq", "8"),
        ("issued_uops", "3"),
        ("alu_results", "1"),
        ("dec_uops", "1"),
        ("dec_valids", "1"),
        ("dispatch_packets", "1"),
        ("map_table", "32"),
        ("committed_map_table", "32"),
    ]
    for key, count in known:
        if key in array:
            return count
    return "unknown"


def classify(array: str, source_file: str, source_line: str) -> Tuple[str, str, str]:
    if array.startswith("next_state.") or (source_file.endswith("boom_core_step.cpp") and source_line == "132"):
        if "br_snapshots" in array:
            return (
                "BoomCoreState next_state = state clones branch snapshot storage before every cycle update",
                "persistent RAM, not per-cycle copied temporaries",
                "HIGH",
            )
        if any(key in array for key in ("rob.entries", "alu_iq.entries", "int_rf", "fp_rf", "ldq", "stq")):
            return (
                "BoomCoreState next_state = state exposes persistent arrays as next_state temporaries",
                "persistent module state with explicit write ports",
                "HIGH",
            )
        if any(key in array for key in ("issued_uops", "alu_results", "dec_uops", "dispatch_packets")):
            return (
                "whole-state copy plus top-level scheduling flattened a small lane array field",
                "small register array",
                "MEDIUM",
            )
        return (
            "whole-state copy creates a field-level next_state temporary",
            "scalar/register or explicit next-state field",
            "MEDIUM",
        )
    if "state_rename_int_map_table_br_snapshots" in array:
        return (
            "static BoomCoreState branch snapshot RAM was automatically partitioned under top scheduling",
            "persistent RAM retained for future BOOM branch equivalence",
            "HIGH",
        )
    if "pipe." in array:
        return (
            "internal hls::stream aggregation from top-level PipeSignals",
            "stream FIFO",
            "LOW",
        )
    return (
        "Vitis automatic partitioning from pipeline/unroll analysis",
        "case-specific",
        "MEDIUM",
    )


def parse_partition_kind(tail: Optional[str]) -> Tuple[str, str]:
    if not tail:
        return "unknown", ""
    tail = tail.strip()
    dim_match = re.search(r"in dimension (\d+) completely", tail)
    if dim_match:
        return "complete", dim_match.group(1)
    if "automatically" in tail:
        dim_match = re.search(r"in dimension (\d+) automatically", tail)
        return "automatic", dim_match.group(1) if dim_match else "all/unknown"
    return tail, ""


def analyze(log_path: Path) -> Tuple[list[Dict[str, str]], Dict[str, str], Counter]:
    rows = []
    last_pass = {"name": "", "elapsed": "", "memory": ""}
    risks: Counter = Counter()
    with log_path.open("r", encoding="utf-8", errors="replace") as handle:
        for line in handle:
            finished = FINISHED_RE.search(line)
            if finished:
                last_pass = {
                    "name": finished.group(1).strip(),
                    "elapsed": finished.group(2).strip(),
                    "memory": finished.group(3).strip(),
                }
            match = PARTITION_RE.search(line)
            if not match:
                continue
            array = match.group(1)
            source_file = match.group(2) or ""
            source_line = match.group(3) or ""
            partition_type, dimension = parse_partition_kind(match.group(4))
            reason, expected_storage, risk = classify(array, source_file, source_line)
            risks[risk] += 1
            rows.append({
                "array": array,
                "source_file": source_file,
                "source_line": source_line,
                "partition_type": partition_type,
                "dimension": dimension,
                "element_count": element_count(array),
                "reason": reason,
                "expected_storage": expected_storage,
                "risk": risk,
            })
    return rows, last_pass, risks


def write_markdown(path: Path, log_path: Path, rows: list[Dict[str, str]], last_pass: Dict[str, str], risks: Counter) -> None:
    next_state_rows = [row for row in rows if row["array"].startswith("next_state.")]
    branch_snapshot_rows = [row for row in rows if "br_snapshots" in row["array"]]
    lines = [
        "# Gate 3.2 Csynth Timeout Analysis",
        "",
        f"Input log: `{log_path}`",
        "",
        f"Automatic partition records: {len(rows)}",
        f"Records caused by `next_state` temporaries: {len(next_state_rows)}",
        f"Branch snapshot partition records: {len(branch_snapshot_rows)}",
        f"Risk counts: HIGH={risks.get('HIGH', 0)}, MEDIUM={risks.get('MEDIUM', 0)}, LOW={risks.get('LOW', 0)}",
        "",
        "Last completed HLS pass before timeout:",
        "",
        f"| Pass | Elapsed | Memory |",
        f"|---|---:|---:|",
        f"| {last_pass.get('name') or 'unknown'} | {last_pass.get('elapsed') or 'unknown'} | {last_pass.get('memory') or 'unknown'} |",
        "",
        "Root cause:",
        "",
        "The post-LSU csynth timeout is dominated by Vitis HLS transformation/auto-partition expansion of the full `BoomCoreState next_state = state` copy in `boom_core_step.cpp`. The copy turns every persistent state array into a field-level `next_state.*` temporary. With the existing top-level loop pipeline, HLS then tries to flatten/partition these temporaries, including branch snapshots, ROB/IQ-related fields, and lane arrays. The earlier hardcoded `#pragma HLS PIPELINE II=1` on `CORE_CYCLE` made this behavior part of the nominal baseline rather than a performance-only experiment.",
        "",
        "Required fix direction:",
        "",
        "- Remove hardcoded baseline core-loop pipelining.",
        "- Stop copying the whole `BoomCoreState` every cycle.",
        "- Keep large persistent arrays as module state with explicit write updates.",
        "- Keep branch snapshots allocated for future BOOM equivalence, but do not expose them as per-cycle copy temporaries.",
        "",
        "See `reports/gate3_2/automatic_partition_inventory.csv` for the full inventory.",
    ]
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", required=True, type=Path)
    parser.add_argument("--csv-output", required=True, type=Path)
    parser.add_argument("--md-output", required=True, type=Path)
    args = parser.parse_args()

    rows, last_pass, risks = analyze(args.input)
    args.csv_output.parent.mkdir(parents=True, exist_ok=True)
    with args.csv_output.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=[
            "array", "source_file", "source_line", "partition_type", "dimension",
            "element_count", "reason", "expected_storage", "risk",
        ])
        writer.writeheader()
        writer.writerows(rows)
    write_markdown(args.md_output, args.input, rows, last_pass, risks)
    print(f"partition_records={len(rows)}")
    print(f"last_pass={last_pass.get('name', '')}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
