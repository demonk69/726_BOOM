#!/usr/bin/env python3
import argparse
import csv
import json
from pathlib import Path


CASES = [
    ("N0_NORMAL_INDEPENDENT_ALU", "independent_alu"),
    ("N1_NORMAL_RAW_CHAIN", "raw_chain"),
    ("N2_NORMAL_BRANCH_TAKEN", "branch_taken"),
    ("N3_NORMAL_BRANCH_NOT_TAKEN", "branch_not_taken"),
    ("N4_NORMAL_NESTED_BRANCH", "nested_branch"),
    ("N5_NORMAL_LOAD_STORE", "load_store"),
    ("N6_NORMAL_TOHOST", "tohost"),
]
EVENTS = {"imem_request", "commit", "dmem_request", "tohost"}
IGNORED = {"cycle", "source", "program", "scenario"}


def load(path):
    records = []
    for line in path.read_text().splitlines():
        record = json.loads(line)
        if record.get("event") in EVENTS:
            records.append({key: value for key, value in record.items() if key not in IGNORED})
    return records


def cycles(path, event):
    return [record["cycle"] for record in map(json.loads, path.read_text().splitlines())
            if record.get("event") == event]


def main():
    parser = argparse.ArgumentParser(description="Compare Gate 3.9 normal RTL traces with Gate 3.8")
    parser.add_argument("--baseline", required=True, type=Path)
    parser.add_argument("--actual", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    rows = []

    for scenario, program in CASES:
        name = f"{program}_{scenario}.jsonl"
        baseline = args.baseline / name
        actual = args.actual / name
        baseline_records = load(baseline) if baseline.is_file() else []
        actual_records = load(actual) if actual.is_file() else []
        baseline_commits = cycles(baseline, "commit") if baseline.is_file() else []
        actual_commits = cycles(actual, "commit") if actual.is_file() else []
        rows.append({
            "test": scenario,
            "program": program,
            "architectural_status": "PASS" if baseline_records == actual_records and baseline_records else "FAIL",
            "event_count": len(actual_records),
            "commit_count": len(actual_commits),
            "cycle_exact": "YES" if baseline_commits == actual_commits and baseline_commits else "NO",
            "baseline_first_commit": baseline_commits[0] if baseline_commits else "",
            "actual_first_commit": actual_commits[0] if actual_commits else "",
            "baseline_last_commit": baseline_commits[-1] if baseline_commits else "",
            "actual_last_commit": actual_commits[-1] if actual_commits else "",
        })

    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open("w", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)
    passed = sum(row["architectural_status"] == "PASS" for row in rows)
    print(f"GATE3_9_NORMAL_RTL_TRACE architectural={passed}/{len(rows)} cycle_exact="
          f"{sum(row['cycle_exact'] == 'YES' for row in rows)}/{len(rows)}")
    return 0 if passed == len(rows) else 1


if __name__ == "__main__":
    raise SystemExit(main())
