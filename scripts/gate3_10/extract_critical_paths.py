#!/usr/bin/env python3
import argparse
import csv
import re
from pathlib import Path


STATE_RE = re.compile(r" <State (\d+)>: ([0-9.]+)ns")
SOURCE_RE = re.compile(r"(/[^:]+):(\d+)")
OP_RE = re.compile(r"'([^']+)' operation")
ARRAY_RE = re.compile(r"on array '([^']+)'")

META = {
    ("lsu_module", 5): ("load response extraction", "load_value", "LOCAL_COMBINATIONAL", "yes", "medium"),
    ("execute_module", 3): ("multiply result", "EXECUTE_RESULTS", "TRUE_STATE_RECURRENCE", "no", "high"),
    ("rob_commit_module", 18): ("free-list duplicate scan", "FREE_LIST_SCAN", "TRUE_STATE_RECURRENCE", "no", "high"),
    ("try_issue_load", 3): ("load request selection", "LOAD_REQUEST", "EXTERNAL_HANDSHAKE", "no", "high"),
    ("rob_commit_module", 13): ("store request", "STORE_COMMIT", "EXTERNAL_HANDSHAKE", "no", "high"),
    ("rename_module", 6): ("free-list selection", "FREE_LIST_SELECT", "TRUE_STATE_RECURRENCE", "no", "high"),
    ("issue_module", 1): ("issue ready/select", "ISSUE_SELECT", "TRUE_STATE_RECURRENCE", "no", "high"),
    ("recover_mispredict", 1): ("branch recovery", "RECOVERY", "SAME_CYCLE_RECOVERY", "no", "high"),
    ("boom_core_reset_step", 1): ("reset initialization", "reset switch", "RESET_ONLY", "yes", "low"),
    ("lsu_module", 1): ("DMEM response handshake", "", "EXTERNAL_HANDSHAKE", "no", "high"),
    ("frontend_module", 1): ("IMEM response handshake", "", "EXTERNAL_HANDSHAKE", "no", "high"),
    ("try_issue_load", 9): ("load request selection", "LOAD_REQUEST", "EXTERNAL_HANDSHAKE", "no", "high"),
    ("decode_module", 1): ("immediate decode", "", "LOCAL_COMBINATIONAL", "yes", "medium"),
    ("kill_rob_younger_than", 4): ("younger ROB kill", "RECOVERY", "SAME_CYCLE_RECOVERY", "no", "high"),
    ("rob_allocate", 1): ("ROB tail update", "ROB_ALLOCATE", "TRUE_STATE_RECURRENCE", "no", "high"),
    ("rob_commit_module", 21): ("trap comparison", "ROB_COMMIT", "TRUE_STATE_RECURRENCE", "no", "high"),
    ("rob_commit_module", 16): ("stale pdst release", "ROB_COMMIT", "TRUE_STATE_RECURRENCE", "no", "high"),
    ("execute_module", 2): ("PRF operand read", "EXECUTE_RESULTS", "TRUE_STATE_RECURRENCE", "no", "high"),
    ("issue_module", 5): ("resolved mask clear", "CLEAR_RESOLVED_MASKS", "SAME_CYCLE_RECOVERY", "no", "high"),
    ("kill_issue_state", 4): ("IQ younger kill", "RECOVERY", "SAME_CYCLE_RECOVERY", "no", "high"),
    ("kill_lsu_state", 10): ("LSU younger kill", "RECOVERY", "SAME_CYCLE_RECOVERY", "no", "high"),
    ("boom_core_cycle_io", 1): ("stream write/control", "CORE_CYCLE", "EXTERNAL_HANDSHAKE", "no", "high"),
    ("recover_mispredict", 18): ("allocation-list recovery", "RECOVERY", "SAME_CYCLE_RECOVERY", "no", "high"),
}

TRIP_COUNTS = {"FREE_LIST_SCAN": 52, "FREE_LIST_SELECT": 52, "EXECUTE_RESULTS": 1,
               "RECOVERY": "8/32/52", "ROB_ALLOCATE": 1, "ROB_COMMIT": 3,
               "CLEAR_RESOLVED_MASKS": 8, "CORE_CYCLE": "infinite", "reset switch": 1}


def parse(path):
    lines = path.read_text(errors="replace").splitlines()
    rows = []
    index = 0
    while index < len(lines):
        match = STATE_RE.match(lines[index])
        if not match:
            index += 1
            continue
        state, delay = int(match.group(1)), float(match.group(2))
        detail = []
        index += 1
        while index < len(lines) and not STATE_RE.match(lines[index]) and not lines[index].startswith("===="):
            if "operation" in lines[index] or "blocking operation" in lines[index] or "fifo " in lines[index]:
                detail.append(lines[index].strip())
            index += 1
        source = next((SOURCE_RE.search(line) for line in detail if SOURCE_RE.search(line)), None)
        operators = []
        arrays = []
        for line in detail:
            op = OP_RE.search(line)
            if op:
                operators.append(op.group(1))
            elif "fifo read" in line:
                operators.append("fifo_read")
            elif "fifo write" in line:
                operators.append("fifo_write")
            array = ARRAY_RE.search(line)
            if array:
                arrays.append(array.group(1))
        rows.append({
            "estimated_delay": delay, "source_function": path.name.split(".verbose")[0],
            "source_file": source.group(1) if source else "", "source_line": source.group(2) if source else "",
            "state": state, "operators": ";".join(operators), "array_access": ";".join(arrays),
        })
    return rows


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--db", required=True, type=Path)
    parser.add_argument("--output-dir", required=True, type=Path)
    args = parser.parse_args()
    rows = []
    for path in args.db.glob("*.verbose.sched.rpt"):
        rows.extend(parse(path))
    rows.sort(key=lambda row: row["estimated_delay"], reverse=True)
    rows = rows[:20]
    fields = ["rank", "estimated_delay", "source_function", "source_file", "source_line", "start_state",
              "end_state", "operators", "array_access", "loop", "loop_trip_count", "pipeline_status",
              "state_feedback", "external_handshake", "classification", "candidate", "risk"]
    args.output_dir.mkdir(parents=True, exist_ok=True)
    with (args.output_dir / "critical_path_inventory.csv").open("w", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=fields)
        writer.writeheader()
        for rank, row in enumerate(rows, 1):
            meta = META.get((row["source_function"], row["state"]), ("state-local path", "", "INSUFFICIENT_EVIDENCE", "no", "medium"))
            writer.writerow({
                "rank": rank, "estimated_delay": f"{row['estimated_delay']:.3f}",
                "source_function": row["source_function"], "source_file": row["source_file"],
                "source_line": row["source_line"], "start_state": f"ST_{row['state']}",
                "end_state": f"ST_{row['state']}", "operators": row["operators"],
                "array_access": row["array_access"], "loop": meta[1],
                "loop_trip_count": TRIP_COUNTS.get(meta[1], ""),
                "pipeline_status": "no", "state_feedback": "yes" if meta[2] == "TRUE_STATE_RECURRENCE" else "no",
                "external_handshake": "yes" if meta[2] == "EXTERNAL_HANDSHAKE" else "no",
                "classification": meta[2], "candidate": meta[3], "risk": meta[4],
            })

    loops = [
        ["RESET_ROB_INIT", "boom_core_reset_step", 32, "no", "RESET_ONLY", "yes", "R1 experiment"],
        ["FREE_LIST_SELECT", "rename_module", 52, "no", "TRUE_STATE_RECURRENCE", "no", "head/count and first-match recurrence"],
        ["ISSUE_SELECT", "issue_module", 8, "no", "TRUE_STATE_RECURRENCE", "no", "oldest-ready and grant recurrence"],
        ["COMPACT_IQ", "issue_module", 8, "no", "TRUE_STATE_RECURRENCE", "no", "write-index recurrence"],
        ["CLEAR_RESOLVED_MASKS", "clear_resolved_masks_in_state", 8, "no", "SAME_CYCLE_RECOVERY", "no", "same-cycle recovery"],
        ["ROB_COMMIT", "rob_commit_module", 3, "no", "EXTERNAL_HANDSHAKE", "no", "ordered commit and trace backpressure"],
        ["LSU_LOAD_ISSUE_SCAN", "lsu_module", 32, "no", "TRUE_STATE_RECURRENCE", "no", "pending transaction and oldest order"],
    ]
    with (args.output_dir / "local_loop_inventory.csv").open("w", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow(["loop", "function", "trip_count", "pipeline_status", "classification", "candidate", "reason"])
        writer.writerows(loops)
    with (args.output_dir / "local_pipeline_legality.csv").open("w", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow(["variant", "object", "classification", "legal_to_run", "decision"])
        writer.writerow(["R1_RESET_INIT_PIPELINE", "RESET_ROB_INIT", "RESET_ONLY", "yes", "RUN"])
        writer.writerow(["L1_FREE_LIST_SELECT", "FREE_LIST_SELECT", "TRUE_STATE_RECURRENCE", "no", "NOT_RUN_ILLEGAL_DEPENDENCE"])
        writer.writerow(["L2_ISSUE_SELECT", "ISSUE_SELECT", "TRUE_STATE_RECURRENCE", "no", "NOT_RUN_ILLEGAL_DEPENDENCE"])
        writer.writerow(["L3_BRANCH_MASK_REDUCTION", "CLEAR_RESOLVED_MASKS", "SAME_CYCLE_RECOVERY", "no", "NOT_RUN_ILLEGAL_DEPENDENCE"])
        writer.writerow(["L4_ROB_COMMIT", "ROB_COMMIT", "EXTERNAL_HANDSHAKE", "no", "NOT_RUN_ILLEGAL_DEPENDENCE"])
        writer.writerow(["L5_LSU_SELECT", "LSU_LOAD_ISSUE_SCAN", "TRUE_STATE_RECURRENCE", "no", "NOT_RUN_ILLEGAL_DEPENDENCE"])


if __name__ == "__main__":
    main()
