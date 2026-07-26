#!/usr/bin/env python3
"""Compare loaded-program BOOM commits against complete HLS traces through tohost."""

from __future__ import annotations

import argparse
import csv
import json
from pathlib import Path
from typing import Dict, List, Optional, Sequence, Tuple

from provisional_gate3_lib import ENTRY_PC, PROGRAMS, TOHOST_ADDR, fmt_inst, fmt_pc, load_jsonl, parse_int


def bool_value(value) -> bool:
    if isinstance(value, bool):
        return value
    if isinstance(value, str):
        return value.strip().lower() in ("1", "true", "yes")
    return bool(value)


def trace_sequence(record: dict) -> int:
    seq = parse_int(record.get("trace_sequence_id"))
    if seq is not None:
        return seq
    return parse_int(record.get("event_index")) or 0


def cycle(record: dict) -> int:
    return parse_int(record.get("cycle")) or parse_int(record.get("original_cycle")) or 0


def record_order_key(record: dict) -> Tuple[int, int]:
    return cycle(record), trace_sequence(record)


def commit_signature(record: dict) -> Tuple[object, ...]:
    rd_valid = bool_value(record.get("rd_valid"))
    exception = bool_value(record.get("exception"))
    return (
        fmt_pc(parse_int(record.get("pc"))),
        fmt_inst(parse_int(record.get("instruction"))),
        rd_valid,
        parse_int(record.get("rd")) if rd_valid else None,
        fmt_pc(parse_int(record.get("rd_value"))) if rd_valid else None,
        exception,
        parse_int(record.get("exception_cause")) if exception else None,
    )


def loaded_commits(records: Sequence[dict]) -> List[dict]:
    commits = []
    for record in sorted(records, key=record_order_key):
        if record.get("event") != "commit":
            continue
        pc = parse_int(record.get("pc"))
        inst = parse_int(record.get("instruction"))
        if pc is None or pc < ENTRY_PC or inst is None:
            continue
        commits.append(record)
    return commits


def tohost_event(records: Sequence[dict]) -> Optional[dict]:
    for record in sorted(records, key=record_order_key):
        if record.get("event") != "tohost":
            continue
        address = parse_int(record.get("address"))
        if address == TOHOST_ADDR:
            return record
    return None


def store_commit(commits: Sequence[dict]) -> Optional[dict]:
    for record in commits:
        inst = parse_int(record.get("instruction"))
        if inst is not None and (inst & 0x7F) == 0x23 and ((inst >> 12) & 0x7) == 3:
            return record
    return None


def compare_program(root: Path, program_info: Dict[str, object], hls_source: str,
                    hls_trace_dir: Optional[Path] = None) -> Dict[str, object]:
    program = str(program_info["name"])
    boom_path = root / "reference" / "boom_traces" / str(program_info["boom_trace"])
    hls_base = hls_trace_dir or root / "reference" / "hls_traces"
    hls_path = hls_base / f"{program}_{hls_source}_full.jsonl"
    row: Dict[str, object] = {
        "program": program,
        "hls_source": hls_source,
        "status": "PASS",
        "boom_commits": 0,
        "hls_commits": 0,
        "compared_commits": 0,
        "first_mismatch_index": "",
        "first_mismatch_reason": "",
        "store_commit_match": "",
        "tohost_match": "",
        "boom_store_pc": "",
        "hls_store_pc": "",
        "boom_tohost_address": "",
        "hls_tohost_address": "",
        "boom_tohost_value": "",
        "hls_tohost_value": "",
        "hls_store_memory_address": "",
        "hls_store_memory_data": "",
        "hls_store_memory_mask": "",
    }
    if not hls_path.exists():
        row["status"] = "MISSING"
        row["first_mismatch_reason"] = f"missing {hls_path.relative_to(root)}"
        return row

    boom_records = load_jsonl(boom_path)
    hls_records = load_jsonl(hls_path)
    boom_commits = loaded_commits(boom_records)
    hls_commits = loaded_commits(hls_records)
    row["boom_commits"] = len(boom_commits)
    row["hls_commits"] = len(hls_commits)
    row["compared_commits"] = min(len(boom_commits), len(hls_commits))

    if len(boom_commits) != len(hls_commits):
        row["status"] = "FAIL"
        row["first_mismatch_index"] = min(len(boom_commits), len(hls_commits))
        row["first_mismatch_reason"] = "commit_count_mismatch"
    else:
        for idx, (boom, hls) in enumerate(zip(boom_commits, hls_commits)):
            if commit_signature(boom) != commit_signature(hls):
                row["status"] = "FAIL"
                row["first_mismatch_index"] = idx
                row["first_mismatch_reason"] = json.dumps({
                    "boom": commit_signature(boom),
                    "hls": commit_signature(hls),
                })
                break

    boom_store = store_commit(boom_commits)
    hls_store = store_commit(hls_commits)
    boom_tohost = tohost_event(boom_records)
    hls_tohost = tohost_event(hls_records)

    boom_store_pc = fmt_pc(parse_int(boom_store.get("pc"))) if boom_store else None
    hls_store_pc = fmt_pc(parse_int(hls_store.get("pc"))) if hls_store else None
    boom_addr = fmt_pc(parse_int(boom_tohost.get("address"))) if boom_tohost else None
    hls_addr = fmt_pc(parse_int(hls_tohost.get("address"))) if hls_tohost else None
    boom_value = fmt_pc(parse_int(boom_tohost.get("value"))) if boom_tohost else None
    hls_value = fmt_pc(parse_int(hls_tohost.get("value"))) if hls_tohost else None
    hls_store_addr = fmt_pc(parse_int(hls_store.get("memory_address"))) if hls_store else None
    hls_store_data = fmt_pc(parse_int(hls_store.get("memory_data"))) if hls_store else None
    hls_store_mask = fmt_inst(parse_int(hls_store.get("memory_mask"))) if hls_store else None

    row["boom_store_pc"] = boom_store_pc or ""
    row["hls_store_pc"] = hls_store_pc or ""
    row["boom_tohost_address"] = boom_addr or ""
    row["hls_tohost_address"] = hls_addr or ""
    row["boom_tohost_value"] = boom_value or ""
    row["hls_tohost_value"] = hls_value or ""
    row["hls_store_memory_address"] = hls_store_addr or ""
    row["hls_store_memory_data"] = hls_store_data or ""
    row["hls_store_memory_mask"] = hls_store_mask or ""
    row["store_commit_match"] = str(bool(boom_store_pc and boom_store_pc == hls_store_pc)).upper()
    row["tohost_match"] = str(bool(boom_addr == hls_addr == fmt_pc(TOHOST_ADDR) and boom_value == hls_value == fmt_pc(1))).upper()

    hls_memory_match = hls_store_addr == fmt_pc(TOHOST_ADDR) and hls_store_data == fmt_pc(1)
    if row["status"] == "PASS" and (row["store_commit_match"] != "TRUE" or row["tohost_match"] != "TRUE" or not hls_memory_match):
        row["status"] = "FAIL"
        row["first_mismatch_reason"] = "store_tohost_mismatch"
    return row


def write_markdown(path: Path, rows: Sequence[Dict[str, object]]) -> None:
    pass_count = sum(1 for row in rows if row["status"] == "PASS")
    lines = [
        "# Full-Program Architectural Diff",
        "",
        "Scope: loaded-program commits through the retired `SD` to `tohost`.",
        "",
        f"Result: {pass_count}/{len(rows)} PASS",
        "",
        "| Program | HLS Source | Status | Commits | Store PC Match | Tohost Match |",
        "|---|---|---|---:|---|---|",
    ]
    for row in rows:
        lines.append(
            f"| {row['program']} | {row['hls_source']} | {row['status']} | "
            f"{row['compared_commits']} | {row['store_commit_match']} | {row['tohost_match']} |"
        )
    failures = [row for row in rows if row["status"] != "PASS"]
    if failures:
        lines.extend(["", "## Failures", ""])
        for row in failures:
            lines.append(f"- `{row['program']}` `{row['hls_source']}`: {row['first_mismatch_reason']}")
    lines.extend([
        "",
        "This is still not official Gate 3 equivalence: the official Chipyard/FESVR/DRAMSim simulator path remains unavailable, and the HLS LSU remains a minimal integer LSU path rather than a full BOOM LSU/cache/MMU implementation.",
    ])
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", default=Path(__file__).resolve().parents[1], type=Path)
    parser.add_argument("--hls-source", action="append", choices=("hls_cpp", "hls_csim"),
                        help="HLS trace source to compare. May be passed more than once. Defaults to hls_cpp.")
    parser.add_argument("--hls-trace-dir", default=None, type=Path,
                        help="Override HLS trace directory. Defaults to reference/hls_traces.")
    parser.add_argument("--csv-output", default=None, type=Path)
    parser.add_argument("--md-output", default=None, type=Path)
    args = parser.parse_args()

    root = args.root.resolve()
    out_dir = root / "reports" / "equivalence" / "provisional_gate3_1"
    csv_output = args.csv_output or out_dir / "full_program_architectural_diff.csv"
    md_output = args.md_output or out_dir / "full_program_architectural_diff.md"
    csv_output.parent.mkdir(parents=True, exist_ok=True)

    hls_sources = args.hls_source or ["hls_cpp"]
    hls_trace_dir = args.hls_trace_dir.resolve() if args.hls_trace_dir else None
    rows = [compare_program(root, program_info, hls_source, hls_trace_dir)
            for hls_source in hls_sources for program_info in PROGRAMS]
    with csv_output.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(rows[0].keys()))
        writer.writeheader()
        writer.writerows(rows)
    write_markdown(md_output, rows)

    for row in rows:
        print(f"{row['program']}: {row['hls_source']} {row['status']} commits={row['compared_commits']}")
    print(f"CSV {csv_output}")
    print(f"MD {md_output}")
    return 0 if all(row["status"] == "PASS" for row in rows) else 1


if __name__ == "__main__":
    raise SystemExit(main())
