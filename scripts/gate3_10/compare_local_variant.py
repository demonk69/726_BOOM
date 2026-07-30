#!/usr/bin/env python3
import argparse
import csv
import json
from pathlib import Path


CASES = [("N0_NORMAL_INDEPENDENT_ALU", "independent_alu"), ("N1_NORMAL_RAW_CHAIN", "raw_chain"),
         ("N2_NORMAL_BRANCH_TAKEN", "branch_taken"), ("N3_NORMAL_BRANCH_NOT_TAKEN", "branch_not_taken"),
         ("N4_NORMAL_NESTED_BRANCH", "nested_branch"), ("N5_NORMAL_LOAD_STORE", "load_store"),
         ("N6_NORMAL_TOHOST", "tohost")]
EVENTS = {"imem_request", "commit", "dmem_request", "tohost"}


def load(path):
    records = [json.loads(line) for line in path.read_text().splitlines() if line.strip()]
    records = [record for record in records if record.get("event") in EVENTS]
    origin = next((record["cycle"] for record in records if record["event"] == "imem_request"), 0)
    payload = [{key: value for key, value in record.items() if key not in {"source", "program", "scenario"}}
               for record in records]
    normalized = [{**record, "cycle": record["cycle"] - origin} for record in payload]
    architectural = [{key: value for key, value in record.items() if key != "cycle"} for record in payload]
    return payload, normalized, architectural


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--baseline", required=True, type=Path)
    parser.add_argument("--actual", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    rows = []
    for test, program in CASES:
        name = f"{program}_{test}.jsonl"
        baseline = load(args.baseline / name)
        actual = load(args.actual / name)
        rows.append({"test": test, "program": program,
                     "architecture": "PASS" if baseline[2] == actual[2] else "FAIL",
                     "absolute_cycle_exact": "YES" if baseline[0] == actual[0] else "NO",
                     "normal_cycle_exact_after_first_fetch": "YES" if baseline[1] == actual[1] else "NO",
                     "event_count": len(actual[0])})
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open("w", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(rows[0])); writer.writeheader(); writer.writerows(rows)
    valid = all(row["architecture"] == "PASS" and row["normal_cycle_exact_after_first_fetch"] == "YES" for row in rows)
    print(f"GATE3_10_LOCAL_COMPARE pass={sum(row['architecture'] == 'PASS' for row in rows)}/7 "
          f"normalized_cycle_exact={sum(row['normal_cycle_exact_after_first_fetch'] == 'YES' for row in rows)}/7")
    return 0 if valid else 1


if __name__ == "__main__":
    raise SystemExit(main())
