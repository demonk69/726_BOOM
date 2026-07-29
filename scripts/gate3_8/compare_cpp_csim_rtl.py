#!/usr/bin/env python3
import argparse
import json
from pathlib import Path

from normalize_rtl_trace import ENTRY_PC, normalize, normalize_commit, parse_hex


def load_hls(path):
    result = []
    for line in path.read_text().splitlines():
        if not line.strip():
            continue
        record = json.loads(line)
        if record.get("event") == "commit" and parse_hex(record.get("pc")) >= ENTRY_PC:
            result.append(normalize_commit(record))
        elif record.get("event") == "tohost":
            result.append({
                "event": "tohost",
                "address": parse_hex(record.get("address")),
                "value": parse_hex(record.get("value")),
                "mask": parse_hex(record.get("mask")),
                "committed": bool(record.get("committed")),
            })
    return result


def first_difference(expected, actual):
    for index, (left, right) in enumerate(zip(expected, actual)):
        if left != right:
            return f"record {index}: expected={left} actual={right}"
    if len(expected) != len(actual):
        return f"record count: expected={len(expected)} actual={len(actual)}"
    return ""


def main():
    parser = argparse.ArgumentParser(description="Compare accepted C++, csim and XSim RTL architectural traces")
    parser.add_argument("--cpp", required=True, type=Path)
    parser.add_argument("--csim", required=True, type=Path)
    parser.add_argument("--rtl", required=True, type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()

    cpp = load_hls(args.cpp)
    csim = load_hls(args.csim)
    rtl = normalize(args.rtl)
    cpp_commits = [record for record in cpp if record["event"] == "commit"]
    csim_commits = [record for record in csim if record["event"] == "commit"]
    rtl_commits = [record for record in rtl if record["event"] == "commit"]
    cpp_tohost = [record for record in cpp if record["event"] == "tohost"]
    csim_tohost = [record for record in csim if record["event"] == "tohost"]
    rtl_tohost = [record for record in rtl if record["event"] == "tohost"]
    cpp_csim_commit_diff = first_difference(cpp_commits, csim_commits)
    cpp_rtl_commit_diff = first_difference(cpp_commits, rtl_commits)
    cpp_csim_tohost_diff = first_difference(cpp_tohost, csim_tohost)
    cpp_rtl_tohost_diff = first_difference(cpp_tohost, rtl_tohost)
    differences = (cpp_csim_commit_diff, cpp_rtl_commit_diff, cpp_csim_tohost_diff, cpp_rtl_tohost_diff)
    status = "PASS" if not any(differences) else "FAIL"
    report = {
        "status": status,
        "cpp_records": len(cpp),
        "csim_records": len(csim),
        "rtl_records": len(rtl),
        "cpp_csim_commit_first_difference": cpp_csim_commit_diff or None,
        "cpp_rtl_commit_first_difference": cpp_rtl_commit_diff or None,
        "cpp_csim_tohost_first_difference": cpp_csim_tohost_diff or None,
        "cpp_rtl_tohost_first_difference": cpp_rtl_tohost_diff or None,
    }
    text = json.dumps(report, indent=2, sort_keys=True) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(text)
    print(text, end="")
    return 0 if status == "PASS" else 1


if __name__ == "__main__":
    raise SystemExit(main())
