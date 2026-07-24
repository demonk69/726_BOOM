#!/usr/bin/env python3
"""Compare two normalized Provisional Gate 3 traces."""

from __future__ import annotations

import argparse
import json
from pathlib import Path

from provisional_gate3_lib import compare_normalized, load_jsonl


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--kind", required=True, choices=("arch", "event", "cycle"))
    parser.add_argument("--ref", required=True, type=Path)
    parser.add_argument("--dut", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()

    result = compare_normalized(load_jsonl(args.ref), load_jsonl(args.dut), args.kind)
    result["ref"] = str(args.ref)
    result["dut"] = str(args.dut)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    mismatch = result.get("first_mismatch")
    mismatch_summary = "" if mismatch is None else json.dumps(mismatch, sort_keys=True)
    print(f"{args.kind},{result['status']},{result['compared_events']},{mismatch_summary}")
    return 0 if result["status"] == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
