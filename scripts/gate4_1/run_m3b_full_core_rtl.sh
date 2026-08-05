#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
BUILD=${BOOM_M3B_FULL_CORE_RTL_BUILD_DIR:-"$ROOT/build/gate4_1/m3b_full_core_rtl"}
REPORT=${BOOM_M3B_FULL_CORE_RTL_REPORT_DIR:-"$ROOT/reports/gate4_1/m3/m3b/full_core_rtl"}
PROJECT="$BUILD/projects/boom_core_top"
SOLUTION=solution_gate4_1_m3b_full_core_rtl
RTL="$PROJECT/$SOLUTION/syn/verilog"
XSIM_BUILD="$BUILD/xsim"
VITIS_HLS_BIN=${VITIS_HLS:-/home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls}
XVLOG_BIN=${XVLOG:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xvlog}
XELAB_BIN=${XELAB:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xelab}
XSIM_BIN=${XSIM:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xsim}
MAX_CYCLES=${BOOM_M3B_FULL_CORE_RTL_MAX_CYCLES:-1000000}
SCENARIO=N0_NORMAL_INDEPENDENT_ALU
START_SECONDS=$SECONDS

for tool in "$VITIS_HLS_BIN" "$XVLOG_BIN" "$XELAB_BIN" "$XSIM_BIN"; do
  [[ -x "$tool" ]] || { printf 'ERROR: required tool is not executable: %s\n' "$tool" >&2; exit 2; }
done
"$VITIS_HLS_BIN" -version 2>&1 | grep -q 'v2021\.2' || {
  printf '%s\n' 'ERROR: Vitis HLS 2021.2 is required.' >&2; exit 2;
}
"$XVLOG_BIN" -version 2>&1 | grep -q 'v2021\.2' || {
  printf '%s\n' 'ERROR: Vivado/XSim 2021.2 is required.' >&2; exit 2;
}
[[ "$MAX_CYCLES" =~ ^[0-9]+$ ]] && ((MAX_CYCLES >= 3000)) || {
  printf 'ERROR: BOOM_M3B_FULL_CORE_RTL_MAX_CYCLES must be an integer >= 3000 (got %s).\n' "$MAX_CYCLES" >&2
  exit 2
}

rm -rf "$BUILD" "$REPORT"
mkdir -p "$BUILD/projects" "$BUILD/programs" "$REPORT/logs/suite" "$REPORT/traces" "$REPORT/csynth_reports"

"$ROOT/scripts/generate_merged.sh" > "$REPORT/logs/generate_merged.log" 2>&1
(
  cd "$BUILD/projects"
  BOOM_HLS_GATE=gate4_1_m3b_full_core_rtl BOOM_HLS_TOP=boom_core_top \
    BOOM_HLS_PROJECT=boom_core_top BOOM_HLS_SOLUTION="$SOLUTION" \
    BOOM_HLS_CFLAGS_EXTRA= FPGA_PART=xczu7ev-ffvc1156-2-e CLOCK_PERIOD=10 \
    "$VITIS_HLS_BIN" -f "$ROOT/scripts/run_top_csynth.tcl"
) > "$REPORT/logs/csynth.log" 2>&1

[[ -s "$RTL/boom_core_top.v" ]] || {
  printf 'ERROR: canonical boom_core_top RTL was not generated: %s\n' "$RTL/boom_core_top.v" >&2
  exit 1
}
[[ ! -e "$RTL/boom_core_step.v" ]] || {
  printf '%s\n' 'ERROR: raw boom_core_step RTL was generated instead of the canonical top.' >&2
  exit 1
}
grep -q 'Running: set_top boom_core_top' "$REPORT/logs/csynth.log" || {
  printf '%s\n' 'ERROR: csynth log does not prove set_top boom_core_top.' >&2
  exit 1
}
cp "$PROJECT/$SOLUTION/syn/report"/* "$REPORT/csynth_reports/"

GATE3_9_RTL_DIR="$RTL" GATE3_9_XSIM_BUILD="$XSIM_BUILD" \
  XVLOG="$XVLOG_BIN" XELAB="$XELAB_BIN" \
  "$ROOT/scripts/gate3_9/build_xsim.sh" > "$REPORT/logs/xsim_build.log" 2>&1

PROGRAMS=(div divu rem remu word_div_rem_mix divide_by_zero divide_overflow
          divide_dependency divide_load_mix divide_branch_kill)
for program in "${PROGRAMS[@]}"; do
  source_image="$ROOT/tb/programs/boom_reference/m3b/$program.hex"
  image="$BUILD/programs/$program.hex"
  trace="$REPORT/traces/$program.jsonl"
  log="$REPORT/logs/$program.log"
  stdout="$REPORT/logs/suite/$program.stdout.log"
  [[ -s "$source_image" ]] || { printf 'ERROR: missing M3B program: %s\n' "$source_image" >&2; exit 1; }
  python3 - "$source_image" "$image" <<'PY'
import sys
from pathlib import Path

source, output = map(Path, sys.argv[1:])
tokens = []
for line in source.read_text().splitlines():
    tokens.extend(line.split("#", 1)[0].split())
if not tokens or any(len(token) != 16 or any(c not in "0123456789abcdefABCDEF" for c in token)
                     for token in tokens):
    raise SystemExit(f"{source}: expected packed 64-bit readmemh words")
last = tokens[-1].lower()
# Preserve the complete architectural stream and every original PC through the
# tohost store. Move only the post-completion self-loop beyond fetch lookahead.
had_terminal_loop = last[0:8] == "0000006f"
if had_terminal_loop:
    tokens[-1] = "00000013" + last[8:16]
tokens.extend(["0000001300000013"] * 128)
if had_terminal_loop:
    tokens.append("0000006f00000013")
output.write_text("\n".join(tokens) + "\n", encoding="ascii")
PY
  GATE3_9_XSIM_BUILD="$XSIM_BUILD" PROGRAM="$image" TRACE="$trace" LOG="$log" \
    MAX_CYCLES="$MAX_CYCLES" XSIM="$XSIM_BIN" \
    "$ROOT/scripts/gate3_9/run_xsim.sh" "$SCENARIO" "$program" > "$stdout" 2>&1
done

python3 - "$ROOT" "$REPORT" "$SCENARIO" "$MAX_CYCLES" <<'PY'
import csv
import json
import re
import sys
from pathlib import Path

root, report = map(Path, sys.argv[1:3])
scenario = sys.argv[3]
max_cycles = int(sys.argv[4])

# Deliberately duplicated from divider_full_core_tests.cpp: this checker does
# not execute or consume the native test's pass/fail output.
expected = {
    "div": {3: 0xfffffffffffffff2, 4: 0xfffffffffffffff3},
    "divu": {3: 0x1999999999999999, 4: 0x199999999999999a},
    "rem": {3: 0xfffffffffffffffe, 4: 0xffffffffffffffff},
    "remu": {3: 5, 4: 6},
    "word_div_rem_mix": {3: 0xffffffffd5555556, 4: 0x2aaaaaaa,
                         5: 0xfffffffffffffffe, 6: 2},
    "divide_by_zero": {3: 0xffffffffffffffff, 4: 123,
                       5: 0xffffffffffffffff, 6: 123},
    "divide_overflow": {3: 0x8000000000000000, 4: 0,
                        6: 0xffffffff80000000, 7: 0},
    "divide_dependency": {3: 14, 5: 7, 6: 2, 7: 8},
    "divide_load_mix": {2: 100, 4: 11, 5: 1},
    "divide_branch_kill": {4: 14, 5: 15},
}
programs = list(expected)
rows = []
failures = []
pass_re = re.compile(r"GATE3_8_PASS scenario=(\S+) cycles=(\d+) commits=(\d+).+tohost=([0-9a-fA-F]+)")

def is_divider(inst):
    return (inst & 0x7f) in (0x33, 0x3b) and ((inst >> 25) & 0x7f) == 1 and ((inst >> 12) & 7) >= 4

for program in programs:
    trace_path = report / "traces" / f"{program}.jsonl"
    log_path = report / "logs" / f"{program}.log"
    records = [json.loads(line) for line in trace_path.read_text().splitlines() if line.strip()]
    commits = [r for r in records if r.get("event") == "commit"]
    tohosts = [r for r in records if r.get("event") == "tohost"]
    endings = [r for r in records if r.get("event") == "metadata" and r.get("phase") == "end"]
    match = pass_re.search(log_path.read_text(errors="replace"))
    checks = []
    matched = []
    for rd, value in expected[program].items():
        hits = [r for r in commits if r.get("rd_valid") is True and r.get("rd") == rd
                and int(r.get("rd_value"), 16) == value]
        checks.append(f"x{rd}=0x{value:016x}")
        if not hits:
            failures.append(f"{program}: expected x{rd}=0x{value:016x} absent from commit trace")
        else:
            matched.append(min(int(r["cycle"]) for r in hits))
    committed_tohost = any(r.get("committed") is True and int(r.get("value", "0"), 16) == 1
                           and int(r.get("address", "0"), 16) == 0x80000080 for r in tohosts)
    commit_tohost = any(r.get("is_store") is True and
                        int(r.get("memory_address", "0"), 16) == 0x80000080 and
                        int(r.get("memory_data", "0"), 16) == 1 for r in commits)
    trace_pass = len(endings) == 1 and endings[0].get("status") == "pass"
    if not committed_tohost or not commit_tohost:
        failures.append(f"{program}: committed tohost=1 missing from tohost or commit trace")
    if not trace_pass:
        failures.append(f"{program}: trace does not have exactly one PASS ending")
    if not match or match.group(1) != scenario or int(match.group(4), 16) != 1:
        failures.append(f"{program}: normal-scenario XSim PASS record is missing or malformed")
    rtl_cycles = int(match.group(2)) if match else -1
    log_commits = int(match.group(3)) if match else -1
    if log_commits != len(commits):
        failures.append(f"{program}: log/trace commit count mismatch ({log_commits}/{len(commits)})")
    divider_commits = [r for r in commits if is_divider(int(r["instruction"], 16))]
    divider_commit_cycles = ";".join(str(r["cycle"]) for r in divider_commits)
    status = "PASS" if not any(f.startswith(program + ":") for f in failures) else "FAIL"
    rows.append((program, scenario, status, rtl_cycles, len(commits), len(divider_commits),
                 min(matched) if matched else "", max(matched) if matched else "",
                 divider_commit_cycles, "commit_trace", "not_exposed_by_gate3_9_harness",
                 ";".join(checks), "PASS" if committed_tohost and commit_tohost else "FAIL",
                 str(log_path.relative_to(root)), str(trace_path.relative_to(root))))

matrix = report / "full_core_rtl_matrix.csv"
with matrix.open("w", newline="") as stream:
    writer = csv.writer(stream)
    writer.writerow(("program", "scenario", "status", "rtl_cycles", "commit_count",
                     "divider_commit_count", "first_expected_commit_cycle",
                     "last_expected_commit_cycle", "divider_commit_cycles",
                     "architectural_value_source", "writeback_observation", "expected_registers",
                     "tohost_status", "log", "trace"))
    writer.writerows(rows)
if len(rows) != 10 or sum(row[2] == "PASS" for row in rows) != 10:
    failures.append(f"matrix: expected 10/10 PASS, got {sum(row[2] == 'PASS' for row in rows)}/10")
if failures:
    raise SystemExit("\n".join(failures))
(report / "verification.txt").write_text(
    "10/10 PASS\n"
    "Expected architectural values independently checked from JSONL commit records.\n"
    "Committed tohost store/value independently checked in commit and tohost records.\n"
    "Internal writeback is not exposed by the existing Gate3.9 harness; divider commit cycles are recorded.\n"
    "The post-tohost terminal self-loop is relocated behind NOP padding in staged images; all architectural/tohost instructions and PCs are unchanged.\n"
    f"scenario={scenario}\nmax_cycles={max_cycles}\n", encoding="ascii")
print(f"Gate 4.1 M3B full-core RTL: 10/10 PASS; total_rtl_cycles={sum(r[3] for r in rows)}; max_program_cycles={max(r[3] for r in rows)}")
PY

runtime=$((SECONDS - START_SECONDS))
printf 'runtime_seconds,%s\n' "$runtime" > "$REPORT/runtime.csv"
printf 'Gate 4.1 M3B full-core RTL completed: 10/10 PASS in %ss.\n' "$runtime"
