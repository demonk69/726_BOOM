#!/usr/bin/env python3
import csv
import shutil
import sys
from pathlib import Path

DATA_BASE = 0x80000100
TOHOST_ADDR = 0x80000080
EXPECTED_REGS = {8: 3, 9: 5, 18: 7, 19: 11, 20: 90, 21: 3, 10: 119}
EXPECTED_MEMORY = {
    DATA_BASE + 0: 3,
    DATA_BASE + 8: 5,
    DATA_BASE + 16: 7,
    DATA_BASE + 24: 11,
    DATA_BASE + 32: 26,
    DATA_BASE + 40: 90,
    DATA_BASE + 48: 119,
    TOHOST_ADDR: 1,
}

def read_csv(path):
    with path.open(newline="") as stream:
        return list(csv.DictReader(stream))

def write_csv(path, fieldnames, rows):
    with path.open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=fieldnames, lineterminator="\n")
        writer.writeheader()
        writer.writerows(rows)

def parse_int(text):
    text = str(text).strip()
    return int(text, 16) if text.startswith("0x") else int(text)

def copy_case(src, dst):
    dst.mkdir(parents=True, exist_ok=True)
    for name in [
        "outstanding_transaction_trace.csv",
        "commit_trace_observed.csv",
        "final_memory_observed.csv",
        "ppa6_sim_summary.csv",
    ]:
        shutil.copy2(src / name, dst / name)

def validate(case_dir, expected_regs=None):
    expected_regs = expected_regs or EXPECTED_REGS
    outstanding = read_csv(case_dir / "outstanding_transaction_trace.csv")
    commits = read_csv(case_dir / "commit_trace_observed.csv")
    memory = read_csv(case_dir / "final_memory_observed.csv")
    summary = {row["field"]: row["value"] for row in read_csv(case_dir / "ppa6_sim_summary.csv")}

    accepts = [row for row in outstanding if row["event"] == "accept_load"]
    responses = [row for row in outstanding if row["event"] == "response_load"]
    expected_by_txid = {row["txid"]: (row["address"], row["data"]) for row in accepts}
    response_by_txid = {row["txid"]: (row["address"], row["data"]) for row in responses}
    checks = {}
    checks["all_responses_present"] = sorted(expected_by_txid) == sorted(response_by_txid)
    checks["txid_owner_data_match"] = checks["all_responses_present"] and all(
        response_by_txid[txid] == expected_by_txid[txid] for txid in expected_by_txid)
    checks["out_of_order_observed"] = int(summary.get("response_reorder_count", "0")) >= 1

    final_regs = {}
    for row in commits:
        if row["rd_valid"] == "1":
            final_regs[int(row["rd"])] = parse_int(row["rd_value"])
    checks["final_registers_match"] = all(final_regs.get(reg) == value for reg, value in expected_regs.items())

    mem_actual = {parse_int(row["address"]): parse_int(row["observed"]) for row in memory}
    checks["final_memory_match"] = all(mem_actual.get(addr) == value for addr, value in EXPECTED_MEMORY.items())
    checks["tohost_committed"] = summary.get("tohost_store_seen") == "1" and summary.get("tohost_commit_seen") == "1"
    checks["complete"] = summary.get("status") == "COMPLETE"
    return checks

def corrupt_swap_response_data(case_dir):
    path = case_dir / "outstanding_transaction_trace.csv"
    rows = read_csv(path)
    response_indexes = [idx for idx, row in enumerate(rows) if row["event"] == "response_load"]
    if len(response_indexes) < 2:
        raise SystemExit("need at least two response rows for swap sensitivity")
    a, b = response_indexes[0], response_indexes[1]
    rows[a]["data"], rows[b]["data"] = rows[b]["data"], rows[a]["data"]
    write_csv(path, list(rows[0].keys()), rows)

def corrupt_omit_response(case_dir):
    path = case_dir / "outstanding_transaction_trace.csv"
    rows = read_csv(path)
    removed = False
    kept = []
    for row in rows:
        if row["event"] == "response_load" and not removed:
            removed = True
            continue
        kept.append(row)
    if not removed:
        raise SystemExit("need response row for omission sensitivity")
    write_csv(path, list(rows[0].keys()), kept)

def status(checks):
    return "PASS" if all(checks.values()) else "FAIL"

def main():
    if len(sys.argv) != 4:
        raise SystemExit("usage: l3r6_oracle_sensitivity.py <ppa6_out_dir> <report_dir> <scratch_dir>")
    ppa6_out = Path(sys.argv[1])
    report_dir = Path(sys.argv[2])
    scratch = Path(sys.argv[3])
    report_dir.mkdir(parents=True, exist_ok=True)
    scratch.mkdir(parents=True, exist_ok=True)

    rows = []
    base_checks = validate(ppa6_out)
    rows.append(["baseline", "PASS", status(base_checks), ";".join(k for k, v in base_checks.items() if not v)])

    swap_dir = scratch / "swap_load_return_data"
    copy_case(ppa6_out, swap_dir)
    corrupt_swap_response_data(swap_dir)
    swap_checks = validate(swap_dir)
    rows.append(["swap_load_return_data", "FAIL", status(swap_checks), ";".join(k for k, v in swap_checks.items() if not v)])

    omit_dir = scratch / "omit_required_response"
    copy_case(ppa6_out, omit_dir)
    corrupt_omit_response(omit_dir)
    omit_checks = validate(omit_dir)
    rows.append(["omit_required_response", "FAIL", status(omit_checks), ";".join(k for k, v in omit_checks.items() if not v)])

    bad_regs = dict(EXPECTED_REGS)
    bad_regs[10] = 120
    bad_checks = validate(ppa6_out, expected_regs=bad_regs)
    rows.append(["tamper_expected_register_x10", "FAIL", status(bad_checks), ";".join(k for k, v in bad_checks.items() if not v)])

    csv_path = report_dir / "oracle_sensitivity_results.csv"
    with csv_path.open("w", newline="") as stream:
        writer = csv.writer(stream, lineterminator="\n")
        writer.writerow(["case", "expected_validator_status", "observed_validator_status", "detected_failures"])
        writer.writerows(rows)

    ok = all(row[1] == row[2] for row in rows)
    md = [
        "# Oracle Sensitivity\n\n",
        "The sensitivity checks use isolated copies of observed PPA6 traces under `/home/lab_726/opencode_tmp`; product RTL, product source, and the committed PPA6 PASS trace are not modified.\n\n",
        "| Case | Expected Validator Status | Observed Validator Status | Detected Failures |\n",
        "| --- | --- | --- | --- |\n",
    ]
    for row in rows:
        md.append(f"| `{row[0]}` | `{row[1]}` | `{row[2]}` | `{row[3] or 'NONE'}` |\n")
    md.extend([
        "\n## Result\n\n",
        f"`L3_R6_ORACLE_SENSITIVITY={'PASS' if ok else 'FAIL'}`\n\n",
        "`L3_R6_ORACLE_INDEPENDENCE=PASS_STATIC_PROGRAM_ORACLE_NO_DUT_STATE`\n",
    ])
    (report_dir / "oracle_sensitivity.md").write_text("".join(md), encoding="utf-8")
    print("PASS" if ok else "FAIL")
    return 0 if ok else 1

if __name__ == "__main__":
    raise SystemExit(main())
