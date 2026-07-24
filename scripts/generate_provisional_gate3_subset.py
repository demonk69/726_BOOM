#!/usr/bin/env python3
"""Generate the Provisional Gate 3 common instruction subset CSV."""

from __future__ import annotations

import argparse
from pathlib import Path

from provisional_gate3_lib import write_subset_csv


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", default=Path(__file__).resolve().parents[1], type=Path)
    parser.add_argument(
        "--output",
        default=None,
        type=Path,
        help="default: reports/equivalence/provisional_gate3/common_instruction_subset.csv",
    )
    args = parser.parse_args()
    root = args.root.resolve()
    output = args.output or root / "reports" / "equivalence" / "provisional_gate3" / "common_instruction_subset.csv"
    write_subset_csv(root, output)
    print(f"WROTE {output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
