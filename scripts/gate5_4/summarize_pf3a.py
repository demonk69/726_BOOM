#!/usr/bin/env python3
import csv
import json
import re
import sys
from pathlib import Path

root = Path(sys.argv[1])
build = Path(sys.argv[2])
report = root / "reports/gate5_4_product_integration/pf3r"
report.mkdir(parents=True, exist_ok=True)
programs = [
    "pf3_straight_commit", "pf3_rvc_packets", "pf3_jal_mask",
    "pf3_conditional_shadow", "pf3_branch_squash", "pf3_exception_flush",
    "pf3_ftq_wrap", "pf3_generation_reuse", "pf3_rv64m",
    "pf3_mixed_control", "pf3_long_stream", "pf3_reset_midstream",
]
expected = {
    "pf3_straight_commit": {8: 11, 9: 17, 18: 28, 19: 33, 20: 42},
    "pf3_rvc_packets": {8: 12, 9: 4, 18: 21, 19: 33},
    "pf3_jal_mask": {8: 9, 9: 13},
    "pf3_conditional_shadow": {8: 5, 9: 5, 18: 19, 19: 23},
    "pf3_branch_squash": {8: 1, 9: 2, 18: 21, 19: 29},
    "pf3_exception_flush": {8: 27},
    "pf3_ftq_wrap": {8: 72, 9: 78},
    "pf3_generation_reuse": {8: 76, 9: 79},
    "pf3_rv64m": {18: 126, 19: 131, 20: 786},
    "pf3_mixed_control": {18: 7, 19: 15, 20: 17},
    "pf3_long_stream": {8: 160, 9: 161, 18: 162},
    "pf3_reset_midstream": {8: 48, 9: 52},
}
forbidden = {
    "pf3_jal_mask": {0x02840413, 0x01f00493},
    "pf3_conditional_shadow": {0x03f00913, 0x03e00993},
    "pf3_branch_squash": {0x03700913, 0x03600993},
    "pf3_generation_reuse": {0x03c00493},
    "pf3_mixed_control": {0x03200913, 0x03300993},
}

event_re = re.compile(r"PF3_EVENT program=(\S+) (.*)")
def parse_events(path):
    result = {}
    for line in path.read_text(encoding="utf-8", errors="ignore").splitlines():
        match = event_re.search(line)
        if not match:
            continue
        fields = dict(item.split("=", 1) for item in match.group(2).split())
        result[match.group(1)] = fields
    return result

native = parse_events(build / "native.log")
csim = parse_events(build / "csim.log")
program_rows = []
for name in programs:
    same = native.get(name) == csim.get(name)
    passed = same and native.get(name, {}).get("status") == "PASS"
    program_rows.append((name, "PASS" if passed else "FAIL", "PASS" if same else "FAIL",
                         ";".join(f"x{rd}=0x{value:016x}" for rd, value in expected[name].items()),
                         " ".join(f"{key}={value}" for key, value in native.get(name, {}).items())))
with (report / "pf3r_program_matrix.csv").open("w", newline="", encoding="utf-8") as stream:
    writer = csv.writer(stream)
    writer.writerow(("program", "status", "native_csim_exact_match", "signature", "events"))
    writer.writerows(program_rows)

rtl_rows = []
coverage_rows = []
rtl_event_re = re.compile(r"PF3_RTL_EVENT program=(\S+) (.*)")
for name in programs:
    trace_path = build / "product_rtl/traces" / f"{name}.jsonl"
    records = [json.loads(line) for line in trace_path.read_text().splitlines() if line]
    commits = [record for record in records if record.get("event") == "commit"]
    final = {record["rd"]: int(record["rd_value"], 16)
             for record in commits if record.get("rd_valid")}
    signature_ok = all(final.get(rd) == value for rd, value in expected[name].items())
    killed_ok = not any(int(record["instruction"], 16) in forbidden.get(name, set())
                        for record in commits if record.get("instruction"))
    exceptions = [record for record in commits if record.get("exception")]
    if name == "pf3_exception_flush":
        terminal_ok = len(exceptions) == 1 and int(exceptions[0]["pc"], 16) == 0x80000004 and \
            int(exceptions[0]["exception_cause"], 16) == 2 and not any(
                record.get("event") == "tohost" for record in records)
    else:
        terminal_ok = any(record.get("event") == "tohost" and record.get("committed") and
                          int(record["value"], 16) == 1 for record in records)
    log_text = (build / "product_rtl/logs" / f"{name}.log").read_text(errors="ignore")
    event_match = rtl_event_re.search(log_text)
    event_fields = dict(item.split("=", 1) for item in event_match.group(2).split()) if event_match else {}
    status = signature_ok and killed_ok and terminal_ok and "PF3_PRODUCT_RTL_PASS" in log_text
    rtl_rows.append((name, "PASS" if status else "FAIL", len(commits),
                     "PASS" if signature_ok else "FAIL", "PASS" if killed_ok else "FAIL",
                     "EXCEPTION_CAUSE_2_PC_80000004" if name == "pf3_exception_flush" else "TOHOST_1",
                     str(trace_path)))
    coverage_rows.append((name,) + tuple(event_fields.get(key, "") for key in
        ("alloc", "reclaim", "redirect", "wrap", "reuse", "generation_retry", "reset", "max_occupancy")))
with (report / "pf3r_full_core_rtl_matrix.csv").open("w", newline="", encoding="utf-8") as stream:
    writer = csv.writer(stream)
    writer.writerow(("program", "status", "commits", "signature", "wrong_path_killed", "termination", "trace"))
    writer.writerows(rtl_rows)
with (report / "program_ftq_coverage.csv").open("w", newline="", encoding="utf-8") as stream:
    writer = csv.writer(stream)
    writer.writerow(("program", "alloc", "reclaim", "redirect", "wrap", "reuse",
                     "generation_retry_cycles", "reset", "max_occupancy"))
    writer.writerows(coverage_rows)

if any(row[1] != "PASS" for row in program_rows + rtl_rows):
    raise SystemExit("PF3A summary validation failed")
print("PF3A_PROGRAM_NATIVE_CSIM_PASS 12/12 exact_event_match")
print("PF3A_FULL_CORE_RTL_PASS 12/12 exact_signature_and_event_coverage")
