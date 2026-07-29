#!/usr/bin/env python3
import argparse
import csv
import json
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description="Summarize Gate 3.9 reset-to-fetch/commit latency")
    parser.add_argument("--status", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    root = args.status.resolve().parents[2]
    rows = []

    with args.status.open(newline="") as handle:
        for source in csv.DictReader(handle):
            trace_path = root / source["trace"]
            if not trace_path.is_file():
                continue
            records = [json.loads(line) for line in trace_path.read_text().splitlines() if line.strip()]
            releases = [record for record in records
                        if record.get("event") == "reset" and record.get("reset") is False]
            for ordinal, release in enumerate(releases, 1):
                release_cycle = release["cycle"]
                next_assert = next((record for record in records
                                    if record.get("event") == "reset" and record.get("reset") is True
                                    and record.get("cycle", -1) > release_cycle), None)
                limit = next_assert["cycle"] if next_assert else None
                fetch = next((record for record in records
                              if record.get("event") == "imem_request" and record.get("cycle", -1) > release_cycle
                              and (limit is None or record.get("cycle", -1) < limit)), None)
                commit = next((record for record in records
                               if record.get("event") == "commit" and record.get("cycle", -1) > release_cycle
                               and (limit is None or record.get("cycle", -1) < limit)), None)
                interrupted = next_assert is not None and not (fetch and commit)
                rows.append({
                    "test": source["test"],
                    "reset_ordinal": ordinal,
                    "release_cycle": release_cycle,
                    "first_fetch_cycle": fetch["cycle"] if fetch else "",
                    "first_fetch_latency": fetch["cycle"] - release_cycle if fetch else "",
                    "first_fetch_pc": fetch.get("pc", "") if fetch else "",
                    "first_commit_cycle": commit["cycle"] if commit else "",
                    "first_commit_latency": commit["cycle"] - release_cycle if commit else "",
                    "first_commit_pc": commit.get("pc", "") if commit else "",
                    "status": "INTERRUPTED" if interrupted else ("PASS" if fetch and commit else "FAIL"),
                })

    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open("w", newline="") as handle:
        fields = list(rows[0]) if rows else ["test", "status"]
        writer = csv.DictWriter(handle, fieldnames=fields)
        writer.writeheader()
        writer.writerows(rows)
    completed = sum(row["status"] == "PASS" for row in rows)
    interrupted = sum(row["status"] == "INTERRUPTED" for row in rows)
    failed = sum(row["status"] == "FAIL" for row in rows)
    print(f"GATE3_9_RESET_LATENCY releases={len(rows)} completed={completed} "
          f"interrupted={interrupted} fail={failed}")
    return 0 if failed == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
