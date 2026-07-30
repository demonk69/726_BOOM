#!/usr/bin/env python3
import argparse
import csv
import json
import re
from pathlib import Path


PASS_RE = re.compile(
    r"GATE3_8_PASS scenario=\S+ cycles=(\d+) commits=(\d+) resets=(\d+) "
    r"imem=(\d+) dmem=(\d+).*tohost=([0-9a-fA-F]+)"
)


def load_trace(path):
    records = []
    if not path.is_file():
        return records
    for line in path.read_text().splitlines():
        if not line.strip():
            continue
        try:
            records.append(json.loads(line))
        except json.JSONDecodeError:
            break
    return records


def first_failure(log_text):
    for line in log_text.splitlines():
        if "mid-run reset first fetch" in line:
            return line.split("Error: ", 1)[-1].strip()
        if "Fatal: GATE3_8_" in line:
            return line.split("Fatal: ", 1)[-1].strip()
        if "ERROR:" in line or "Error:" in line:
            return line.strip()
    return ""


def main():
    parser = argparse.ArgumentParser(description="Create the Gate 3.9 RTL verification matrix")
    parser.add_argument("--status", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--root", type=Path)
    args = parser.parse_args()
    root = args.root.resolve() if args.root else args.status.resolve().parents[2]
    rows = []

    with args.status.open(newline="") as handle:
        for source in csv.DictReader(handle):
            log_path = root / source["log"]
            trace_path = root / source["trace"]
            log_text = log_path.read_text(errors="replace") if log_path.is_file() else ""
            records = load_trace(trace_path)
            end = next((record for record in reversed(records)
                        if record.get("event") == "metadata" and record.get("phase") == "end"), {})
            match = PASS_RE.search(log_text)
            commits = [record for record in records if record.get("event") == "commit"]
            imem = [record for record in records if record.get("event") == "imem_request"]
            dmem = [record for record in records if record.get("event") == "dmem_request"]
            resets = [record for record in records if record.get("event") == "reset" and record.get("reset") is False]
            tohost = [record for record in records if record.get("event") == "tohost"]
            rows.append({
                "test": source["test"],
                "program": source["program"],
                "status": "PASS" if source["run_status"] == "XSIM_PASS" else "FAIL",
                "cycles": match.group(1) if match else end.get("cycle", max((r.get("cycle", 0) for r in records), default=0)),
                "commit_count": match.group(2) if match else end.get("commit_count", len(commits)),
                "reset_count": match.group(3) if match else len(resets),
                "imem_transfers": match.group(4) if match else len(imem),
                "dmem_transfers": match.group(5) if match else len(dmem),
                "trace_transfers": end.get("trace_transfers", len(commits)),
                "tohost": match.group(6) if match else (tohost[-1].get("value", "") if tohost else ""),
                "first_failure": "" if match else first_failure(log_text),
                "log": source["log"],
                "trace": source["trace"],
            })

    if len(rows) != 49:
        raise SystemExit(f"expected 49 Gate 3.9 rows, found {len(rows)}")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open("w", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)
    passed = sum(row["status"] == "PASS" for row in rows)
    print(f"GATE3_9_MATRIX tests={len(rows)} pass={passed} fail={len(rows) - passed}")
    return 0 if passed == 49 else 1


if __name__ == "__main__":
    raise SystemExit(main())
