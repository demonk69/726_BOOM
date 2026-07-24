#!/usr/bin/env python3
"""Normalize source traces and run one Provisional Gate 3 diff kind across all programs."""

from __future__ import annotations

import argparse
import csv
import json
from pathlib import Path

from provisional_gate3_lib import (
    PROGRAMS,
    compare_normalized,
    load_jsonl,
    normalize_boom,
    normalize_hls,
    source_trace_path,
    write_jsonl,
)


def normalize_source(root: Path, out_dir: Path, source: str, program: str) -> Path:
    trace_path = source_trace_path(root, source, program)
    if not trace_path.exists():
        raise FileNotFoundError(trace_path)
    records = load_jsonl(trace_path)
    if source == "boom":
        normalized = normalize_boom(records, program)
    else:
        normalized = normalize_hls(records, program, source)
    out_path = out_dir / "normalized" / f"{program}_{source}.jsonl"
    write_jsonl(out_path, normalized)
    return out_path


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--kind", required=True, choices=("arch", "event", "cycle"))
    parser.add_argument("--ref-source", default="boom", choices=("boom", "hls_cpp", "hls_csim"))
    parser.add_argument("--dut-source", default="hls_csim", choices=("boom", "hls_cpp", "hls_csim"))
    parser.add_argument("--root", default=Path(__file__).resolve().parents[1], type=Path)
    parser.add_argument("--out-dir", default=None, type=Path)
    args = parser.parse_args()

    root = args.root.resolve()
    out_dir = args.out_dir or root / "reports" / "equivalence" / "provisional_gate3"
    diff_dir = out_dir / "diffs"
    diff_dir.mkdir(parents=True, exist_ok=True)
    summary_path = out_dir / f"{args.ref_source}_vs_{args.dut_source}_{args.kind}_diff.csv"

    rows = []
    all_pass = True
    for program_info in PROGRAMS:
        program = str(program_info["name"])
        ref_norm = normalize_source(root, out_dir, args.ref_source, program)
        dut_norm = normalize_source(root, out_dir, args.dut_source, program)
        result = compare_normalized(load_jsonl(ref_norm), load_jsonl(dut_norm), args.kind)
        all_pass = all_pass and result["status"] == "PASS"
        result.update({"program": program, "ref": str(ref_norm), "dut": str(dut_norm)})
        detail_path = diff_dir / f"{program}_{args.ref_source}_vs_{args.dut_source}_{args.kind}.json"
        detail_path.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        mismatch = result.get("first_mismatch")
        rows.append({
            "program": program,
            "ref_source": args.ref_source,
            "dut_source": args.dut_source,
            "kind": args.kind,
            "status": result["status"],
            "ref_events": result["ref_events"],
            "dut_events": result["dut_events"],
            "compared_events": result["compared_events"],
            "first_mismatch_index": result["first_mismatch_index"],
            "first_mismatch_reason": "" if mismatch is None else mismatch.get("reason", ""),
            "detail": str(detail_path.relative_to(root)),
        })
        print(f"{program}: {args.kind} {args.ref_source}->{args.dut_source} {result['status']} compared={result['compared_events']}")

    with summary_path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(rows[0].keys()))
        writer.writeheader()
        writer.writerows(rows)
    print(f"SUMMARY {summary_path}")
    return 0 if all_pass else 1


if __name__ == "__main__":
    raise SystemExit(main())
