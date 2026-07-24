#!/usr/bin/env python3
"""Normalize a standalone BOOM JSONL trace to the Provisional Gate 3 prefix schema."""

from __future__ import annotations

import argparse
from pathlib import Path

from provisional_gate3_lib import load_jsonl, normalize_boom, write_jsonl


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--program", required=True)
    parser.add_argument("--input", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    normalized = normalize_boom(load_jsonl(args.input), args.program)
    write_jsonl(args.output, normalized)
    print(f"WROTE {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
