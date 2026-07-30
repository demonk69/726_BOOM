#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path


PROGRAMS = ["branch_not_taken", "branch_taken", "independent_alu", "nested_branch", "raw_chain"]
SOURCES = ["hls_cpp", "hls_csim"]


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--baseline", required=True, type=Path)
    parser.add_argument("--actual", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--title", default="Gate 3.7 Final Trace Diff")
    args = parser.parse_args()
    rows = []
    for source in SOURCES:
        for program in PROGRAMS:
            name = f"{program}_{source}_full.jsonl"
            expected = args.baseline / name
            actual = args.actual / name
            status = "PASS" if expected.exists() and actual.exists() and expected.read_bytes() == actual.read_bytes() else "FAIL"
            rows.append((program, source, status, expected, actual))
    lines = [
        f"# {args.title}", "",
        f"Result: {sum(row[2] == 'PASS' for row in rows)}/{len(rows)} byte-identical", "",
        "| Program | Source | Status |", "|---|---|---|",
    ]
    lines.extend(f"| {program} | {source} | {status} |" for program, source, status, _expected, _actual in rows)
    failures = [row for row in rows if row[2] != "PASS"]
    if failures:
        first = failures[0]
        lines.extend(["", f"First mismatch: {first[0]} {first[1]}", f"Expected: `{first[3]}`", f"Actual: `{first[4]}`"])
    args.output.write_text("\n".join(lines) + "\n", encoding="utf-8")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
