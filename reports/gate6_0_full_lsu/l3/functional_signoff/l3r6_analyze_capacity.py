#!/usr/bin/env python3
import csv
import hashlib
import json
import sys
from pathlib import Path

DATA_BASE = 0x80000100
TOHOST_ADDR = 0x80000080

def read_csv(path):
    with path.open(newline="") as stream:
        return list(csv.DictReader(stream))

def write_csv(path, header, rows):
    with path.open("w", newline="") as stream:
        writer = csv.writer(stream, lineterminator="\n")
        writer.writerow(header)
        writer.writerows(rows)

def parse_summary(path):
    return {row["field"]: row["value"] for row in read_csv(path)}

def parse_int(text):
    text = str(text).strip()
    return int(text, 16) if text.startswith("0x") else int(text)

def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def classify_limit(summary):
    max_external = int(summary.get("max_external_outstanding", "0"))
    if max_external >= 4:
        return "NONE"
    if int(summary.get("dmem_load_count", "0")) < 4:
        return "ISSUE_FRONTEND"
    if int(summary.get("max_internal_outstanding", "0")) < 4:
        return "ROB_OR_COMPLETION"
    return "LSU_CAPACITY"

def main():
    if len(sys.argv) != 5:
        raise SystemExit("usage: l3r6_analyze_capacity.py <out_dir> <report_dir> <rtl_dir> <program_bin>")
    out_dir = Path(sys.argv[1])
    report_dir = Path(sys.argv[2])
    rtl_dir = Path(sys.argv[3])
    program_bin = Path(sys.argv[4])
    report_dir.mkdir(parents=True, exist_ok=True)

    summary = parse_summary(out_dir / "capacity_summary.csv")
    trace = read_csv(out_dir / "full_core_capacity_trace.csv")
    commits = read_csv(out_dir / "capacity_commit_trace.csv")
    memory = read_csv(out_dir / "capacity_final_memory.csv")

    (report_dir / "full_core_capacity_trace.csv").write_text((out_dir / "full_core_capacity_trace.csv").read_text(), encoding="utf-8")

    mem_actual = {parse_int(row["address"]): parse_int(row["observed"]) for row in memory}
    expected_mem = {
        DATA_BASE + 0: 1,
        DATA_BASE + 8: 2,
        DATA_BASE + 16: 4,
        DATA_BASE + 24: 8,
        DATA_BASE + 32: 16,
        DATA_BASE + 40: 32,
        DATA_BASE + 64: 63,
        TOHOST_ADDR: 1,
    }
    memory_match = all(mem_actual.get(addr) == value for addr, value in expected_mem.items())
    final_a0_match = summary.get("final_a0") == "0x000000000000003f"
    max_external = int(summary.get("max_external_outstanding", "0"))
    max_internal = int(summary.get("max_internal_outstanding", "0"))
    reorders = int(summary.get("response_reorder_count", "0"))
    post_capacity = int(summary.get("post_capacity_load_accepts", "0"))
    over_capacity = int(summary.get("over_capacity_handshakes", "0"))
    complete = summary.get("status") == "COMPLETE"
    protocol = summary.get("protocol_error") == "0" and summary.get("first_fetch_error") == "0"
    tohost = summary.get("tohost_store_seen") == "1" and summary.get("tohost_commit_seen") == "1"
    capacity_4_4 = (
        complete and protocol and tohost and memory_match and final_a0_match and
        max_external >= 4 and reorders >= 1 and post_capacity >= 1 and over_capacity == 0
    )
    limit = classify_limit(summary)

    matrix_rows = [
        ["CONFIG_4_4", "PASS" if capacity_4_4 else "LIMITED", max_external, max_internal, reorders,
         post_capacity, over_capacity, "PASS" if memory_match else "FAIL", "PASS" if final_a0_match else "FAIL",
         limit, str(out_dir / "full_core_capacity_trace.csv")],
        ["CONFIG_8_8", "NOT_RUN", "", "", "", "", "", "", "", "NOT_RUN_RESOURCE_SCOPE", "No 8_8 generated full-core RTL was available in the current PPA6 evidence bundle."],
        ["CONFIG_16_16", "NOT_RUN", "", "", "", "", "", "", "", "NOT_RUN_RESOURCE_SCOPE", "No 16_16 generated full-core RTL was available in the current PPA6 evidence bundle."],
    ]
    write_csv(
        report_dir / "full_core_capacity_matrix.csv",
        ["config", "status", "max_external_outstanding", "max_internal_outstanding", "reordered_responses",
         "post_capacity_load_accepts", "over_capacity_handshakes", "memory_match", "final_a0_match",
         "limit_classification", "evidence"],
        matrix_rows,
    )

    artifact_rows = [
        ["capacity_program", str(report_dir / "l3r6_capacity_program.S"), sha256(report_dir / "l3r6_capacity_program.S")],
        ["capacity_testbench", str(report_dir / "l3r6_capacity_tb.sv"), sha256(report_dir / "l3r6_capacity_tb.sv")],
        ["program_bin", str(program_bin), sha256(program_bin)],
        ["rtl_top", str(rtl_dir / "boom_core_top.v"), sha256(rtl_dir / "boom_core_top.v")],
    ]
    write_csv(report_dir / "capacity_artifact_hashes.csv", ["artifact", "path", "sha256"], artifact_rows)

    print(json.dumps({
        "capacity_4_4": capacity_4_4,
        "max_external": max_external,
        "max_internal": max_internal,
        "reorders": reorders,
        "post_capacity_load_accepts": post_capacity,
        "limit": limit,
    }, sort_keys=True))
    return 0 if complete and protocol and tohost else 1

if __name__ == "__main__":
    raise SystemExit(main())
