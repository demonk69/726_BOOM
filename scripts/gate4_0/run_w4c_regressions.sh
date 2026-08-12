#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
BUILD_DIR="$ROOT/build/gate4_0/w4c_regression"
REPORT_DIR="$ROOT/reports/gate4_0/w4/regression/w4c"
LOG_DIR="$REPORT_DIR/logs"
mkdir -p "$BUILD_DIR" "$LOG_DIR"

BOOM_REGRESSION_BUILD_DIR="$BUILD_DIR/product_full" \
BOOM_REGRESSION_REPORT_DIR="$REPORT_DIR/product_full" \
  "$ROOT/scripts/gate4_0/run_w3_regressions.sh"

COMMON_SRCS=(
  "$ROOT/src/boom_core_step.cpp" "$ROOT/src/frontend.cpp" "$ROOT/src/rvc.cpp" "$ROOT/src/decode.cpp"
  "$ROOT/src/rename.cpp" "$ROOT/src/rob.cpp" "$ROOT/src/issue.cpp"
  "$ROOT/src/execute.cpp" "$ROOT/src/branch.cpp" "$ROOT/src/lsu.cpp"
  "$ROOT/src/completion.cpp" "$ROOT/src/commit.cpp" "$ROOT/src/csr.cpp"
  "$ROOT/src/reset.cpp"
)

compile_run() {
  local name=$1 source=$2
  shift 2
  ${CXX:-g++} -std=c++11 -I"$ROOT/include" "$@" "${COMMON_SRCS[@]}" "$source" \
    -o "$BUILD_DIR/$name" > "$LOG_DIR/${name}_compile.log" 2>&1
  "$BUILD_DIR/$name" > "$LOG_DIR/$name.log" 2>&1
}

compile_run w4a_completion_interface_tests \
  "$ROOT/tb/differential/w4a_completion_interface_tests.cpp" \
  -DBOOM_HLS_W4A_COMPLETION_DIAGNOSTIC
compile_run w4b_multi_rob_complete_tests \
  "$ROOT/tb/differential/w4b_multi_rob_complete_tests.cpp"
compile_run w4_multi_wakeup_tests \
  "$ROOT/tb/differential/w4_multi_wakeup_tests.cpp"
compile_run w4_bypass_tests \
  "$ROOT/tb/differential/w4_bypass_tests.cpp"
compile_run w4a_random_diagnostic \
  "$ROOT/tb/differential/w3_dual_execute_random_tests.cpp" \
  -DBOOM_HLS_W4A_COMPLETION_DIAGNOSTIC

grep -q 'Software suites:.*400 passed, 0 failed' "$REPORT_DIR/product_full/regression_after.md"
grep -q 'W4A completion interfaces: 19 passed, 0 failed' "$LOG_DIR/w4a_completion_interface_tests.log"
grep -q 'W4B multi ROB complete: 17 passed, 0 failed' "$LOG_DIR/w4b_multi_rob_complete_tests.log"
grep -q 'W4C multi wakeup: 13 passed, 0 failed' "$LOG_DIR/w4_multi_wakeup_tests.log"
grep -q 'W4C bypass: 11 passed, 0 failed' "$LOG_DIR/w4_bypass_tests.log"
grep -q 'random differential: PASS' "$LOG_DIR/w4a_random_diagnostic.log"

python3 - "$REPORT_DIR" <<'PY'
import csv
import re
import sys
from pathlib import Path

report = Path(sys.argv[1])
logs = report / "logs"
wakeup = re.search(r"W4C_WAKEUP_METRICS peak_wakeups=(\d+) peak_prf_writes=(\d+) bounded_wait=(\d+)",
                   (logs / "w4_multi_wakeup_tests.log").read_text())
bypass = re.search(r"W4C_BYPASS_METRICS peak_bypass=(\d+) peak_prf_writes=(\d+) conflicts_checked=(\d+)",
                   (logs / "w4_bypass_tests.log").read_text())
if not wakeup or not bypass:
    raise SystemExit("missing W4C metrics")
peak_wakeup, wake_prf, wait = map(int, wakeup.groups())
peak_bypass, bypass_prf, conflicts = map(int, bypass.groups())
if peak_wakeup < 2 or peak_bypass < 2 or wake_prf != 1 or bypass_prf != 1 or wait > 3 or conflicts != 1:
    raise SystemExit("W4C metric guard failed")
with (report / "w4c_metrics.csv").open("w", newline="", encoding="utf-8") as stream:
    writer = csv.writer(stream)
    writer.writerow(("metric", "observed", "requirement", "status"))
    writer.writerow(("peak_wakeups", peak_wakeup, ">=2", "PASS"))
    writer.writerow(("peak_bypass", peak_bypass, ">=2", "PASS"))
    writer.writerow(("peak_prf_writes", max(wake_prf, bypass_prf), "=1", "PASS"))
    writer.writerow(("bounded_wait_cycles", wait, "<=3", "PASS"))
    writer.writerow(("conflict_check", conflicts, "=1", "PASS"))

def metrics(path):
    return {name: int(value) for name, value in re.findall(r"^METRIC,([^,]+),(\d+)$", path.read_text(), re.M)}
product = metrics(report / "product_full/logs/w3_dual_execute_random_tests.log")
baseline = metrics(logs / "w4a_random_diagnostic.log")
names = ("accepted_uops", "execute_events", "completion_consumed", "product_prf_writes",
         "product_wakeups", "product_rob_completes", "committed_tokens", "total_random_cycles")
with (report / "w4c_event_deltas.csv").open("w", newline="", encoding="utf-8") as stream:
    writer = csv.writer(stream)
    writer.writerow(("metric", "w4a_diagnostic", "w4c_product", "delta"))
    for name in names:
        writer.writerow((name, baseline[name], product[name], product[name] - baseline[name]))
PY

printf '%s\n' 'Gate 4.0 W4C regressions complete: product 400/400; W4A 19/19; W4B 17/17; wakeup 13/13; bypass 11/11.'
