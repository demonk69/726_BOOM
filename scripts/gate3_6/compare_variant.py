#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path


PROGRAMS = ["branch_not_taken", "branch_taken", "independent_alu", "nested_branch", "raw_chain"]
SOURCES = ["hls_cpp", "hls_csim"]


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--root", required=True, type=Path)
    parser.add_argument("--variant", required=True)
    args = parser.parse_args()
    root = args.root.resolve()
    report_dir = root / "reports/gate3_6/variants" / args.variant
    baseline = root / "reports/gate3_6/baseline_artifacts/accepted_hls_traces"
    traces = report_dir / "hls_traces"
    rows = []
    for source in SOURCES:
        for program in PROGRAMS:
            name = f"{program}_{source}_full.jsonl"
            expected = baseline / name
            actual = traces / name
            status = "PASS" if expected.exists() and actual.exists() and expected.read_bytes() == actual.read_bytes() else "FAIL"
            rows.append((program, source, status, expected, actual))
    lines = [f"# {args.variant} Trace Diff", "", f"Result: {sum(row[2] == 'PASS' for row in rows)}/{len(rows)} byte-identical", "", "| Program | Source | Status |", "|---|---|---|"]
    for program, source, status, _expected, _actual in rows:
        lines.append(f"| {program} | {source} | {status} |")
    failures = [row for row in rows if row[2] != "PASS"]
    if failures:
        first = failures[0]
        lines.extend(["", f"First mismatch: {first[0]} {first[1]}", f"Expected: `{first[3]}`", f"Actual: `{first[4]}`"])
    (report_dir / "trace_diff.md").write_text("\n".join(lines) + "\n", encoding="utf-8")
    return 0 if not failures else 1


if __name__ == "__main__":
    raise SystemExit(main())
