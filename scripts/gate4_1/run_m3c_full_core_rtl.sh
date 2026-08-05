#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
BUILD=${BOOM_M3C_FULL_RTL_BUILD_DIR:-"$ROOT/build/gate4_1/m3c_full_core_rtl"}
REPORT=${BOOM_M3C_FULL_RTL_REPORT_DIR:-"$ROOT/reports/gate4_1/m3/m3c/full_core_rtl"}
BASE_BUILD="$BUILD/base_m3b"
BASE_REPORT="$REPORT/base_m3b"
XSIM_BUILD="$BASE_BUILD/xsim"
XSIM_BIN=${XSIM:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xsim}
MAX_CYCLES=${BOOM_M3C_FULL_RTL_MAX_CYCLES:-1000000}
START_SECONDS=$SECONDS

rm -rf "$BUILD" "$REPORT"
mkdir -p "$BUILD/programs" "$REPORT/logs/suite" "$REPORT/traces"
BOOM_M3B_FULL_CORE_RTL_BUILD_DIR="$BASE_BUILD" \
BOOM_M3B_FULL_CORE_RTL_REPORT_DIR="$BASE_REPORT" \
  "$ROOT/scripts/gate4_1/run_m3b_full_core_rtl.sh" \
  > "$REPORT/logs/base_m3b.log" 2>&1

PROGRAMS=(mul_family_all div_family_all rv64m_all_13 mul_div_dependency
  div_mul_dependency high_multiply_mix word_multiply_divide_mix
  divide_by_zero_mix signed_overflow_mix rv64m_branch_mix rv64m_load_mix
  rv64m_store_mix rv64m_reset_replay rv64m_rob_wrap rv64m_tohost_stress)

for program in "${PROGRAMS[@]}"; do
  source_image="$ROOT/tb/programs/boom_reference/m3b/$program.hex"
  image="$BUILD/programs/$program.hex"
  trace="$REPORT/traces/$program.jsonl"
  log="$REPORT/logs/$program.log"
  stdout="$REPORT/logs/suite/$program.stdout.log"
  python3 - "$source_image" "$image" <<'PY'
import sys
from pathlib import Path
source, output = map(Path, sys.argv[1:])
tokens = [token for line in source.read_text().splitlines()
          for token in line.split("#", 1)[0].split()]
last = tokens[-1].lower()
had_loop = last[0:8] == "0000006f"
if had_loop:
    tokens[-1] = "00000013" + last[8:]
tokens.extend(["0000001300000013"] * 128)
if had_loop:
    tokens.append("0000006f00000013")
output.write_text("\n".join(tokens) + "\n", encoding="ascii")
PY
  scenario=N0_NORMAL_INDEPENDENT_ALU
  [[ "$program" != rv64m_reset_replay ]] || scenario=R8_DOUBLE_RUNTIME_RESET
  GATE3_9_XSIM_BUILD="$XSIM_BUILD" PROGRAM="$image" TRACE="$trace" LOG="$log" \
    MAX_CYCLES="$MAX_CYCLES" XSIM="$XSIM_BIN" \
    "$ROOT/scripts/gate3_9/run_xsim.sh" "$scenario" "$program" > "$stdout" 2>&1
done

python3 - "$ROOT" "$REPORT" <<'PY'
import csv
import json
import sys
from pathlib import Path

root, report = map(Path, sys.argv[1:])
expected = {
    "mul_family_all": {3: 0xffffffffffffffc1, 7: 0xffffffffffffffc1},
    "div_family_all": {3: 0xffffffffffffffff, 5: 123, 10: 123},
    "rv64m_all_13": {3: 700, 8: 0xffffffffffffffff, 10: 100, 15: 100},
    "mul_div_dependency": {3: 42, 5: 14, 7: 70},
    "div_mul_dependency": {3: 14, 5: 126, 6: 0},
    "high_multiply_mix": {3: 0xffffffffffffffff, 5: 2, 6: 0xfffffffffffffffa},
    "word_multiply_divide_mix": {3: 0xfffffffffffffd44, 4: 0xfffffffffffffff2, 6: 0xfffffffffffffffe},
    "divide_by_zero_mix": {3: 0xffffffffffffffff, 5: 123, 8: 123},
    "signed_overflow_mix": {3: 0x8000000000000000, 4: 0, 6: 0},
    "rv64m_branch_mix": {5: 9, 6: 81},
    "rv64m_load_mix": {2: 84, 4: 12},
    "rv64m_store_mix": {3: 99, 4: 99},
    "rv64m_reset_replay": {3: 24, 4: 120},
    "rv64m_rob_wrap": {1: 40, 3: 280, 4: 40},
    "rv64m_tohost_stress": {3: 12, 4: 144},
}
rows, failures = [], []

def is_m(inst):
    return (inst & 0x7f) in (0x33, 0x3b) and ((inst >> 25) & 0x7f) == 1

for name, registers in expected.items():
    trace_path = report / "traces" / f"{name}.jsonl"
    records = [json.loads(line) for line in trace_path.read_text().splitlines() if line.strip()]
    commits = [r for r in records if r.get("event") == "commit"]
    m_commits = [r for r in commits if is_m(int(r["instruction"], 16))]
    for rd, value in registers.items():
        if not any(r.get("rd_valid") is True and r.get("rd") == rd and
                   int(r.get("rd_value"), 16) == value for r in commits):
            failures.append(f"{name}: x{rd}=0x{value:016x} missing")
    tohost = any(r.get("event") == "tohost" and r.get("committed") is True and
                 int(r.get("address", "0"), 16) == 0x80000080 and
                 int(r.get("value", "0"), 16) == 1 for r in records)
    ending = [r for r in records if r.get("event") == "metadata" and r.get("phase") == "end"]
    if not tohost or len(ending) != 1 or ending[0].get("status") != "pass":
        failures.append(f"{name}: committed tohost/pass ending missing")
    if not m_commits:
        failures.append(f"{name}: no RV64M commit")
    if name == "rv64m_all_13" and len(m_commits) != 13:
        failures.append(f"{name}: expected 13 M commits, got {len(m_commits)}")
    scenario = "R8_DOUBLE_RUNTIME_RESET" if name == "rv64m_reset_replay" else "N0_NORMAL_INDEPENDENT_ALU"
    status = "FAIL" if any(f.startswith(name + ":") for f in failures) else "PASS"
    rows.append((name, scenario, len(commits), len(m_commits),
                 "PASS" if tohost else "FAIL", status,
                 str(trace_path.relative_to(root))))

with (report / "full_core_rtl_matrix.csv").open("w", newline="") as stream:
    writer = csv.writer(stream)
    writer.writerow(("program", "scenario", "commit_count", "rv64m_commit_count",
                     "tohost", "status", "trace"))
    writer.writerows(rows)
if failures or len(rows) != 15:
    raise SystemExit("\n".join(failures))
print("Gate 4.1 M3C full-core generated RTL: 15/15 PASS")
PY

runtime=$((SECONDS - START_SECONDS))
printf 'runtime_seconds,%s\n' "$runtime" > "$REPORT/runtime.csv"
printf 'Gate 4.1 M3C full-core RTL completed: 15/15 PASS in %ss.\n' "$runtime"
