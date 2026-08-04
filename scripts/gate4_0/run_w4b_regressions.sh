#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
BUILD_DIR="$ROOT/build/gate4_0/w4b_regression"
REPORT_DIR="$ROOT/reports/gate4_0/w4/regression/w4b"
LOG_DIR="$REPORT_DIR/logs"
mkdir -p "$BUILD_DIR" "$LOG_DIR"

# The complete software/csim/trace suite runs the canonical default W4B product.
BOOM_REGRESSION_BUILD_DIR="$BUILD_DIR/product_full" \
BOOM_REGRESSION_REPORT_DIR="$REPORT_DIR/product_full" \
  "$ROOT/scripts/gate4_0/run_w3_regressions.sh"

COMMON_SRCS=(
  "$ROOT/src/boom_core_step.cpp" "$ROOT/src/frontend.cpp" "$ROOT/src/decode.cpp"
  "$ROOT/src/rename.cpp" "$ROOT/src/rob.cpp" "$ROOT/src/issue.cpp"
  "$ROOT/src/execute.cpp" "$ROOT/src/branch.cpp" "$ROOT/src/lsu.cpp"
  "$ROOT/src/completion.cpp" "$ROOT/src/commit.cpp" "$ROOT/src/csr.cpp"
  "$ROOT/src/reset.cpp"
)

${CXX:-g++} -std=c++11 -DBOOM_HLS_W4A_COMPLETION_DIAGNOSTIC \
  -I"$ROOT/include" "${COMMON_SRCS[@]}" \
  "$ROOT/tb/differential/w4a_completion_interface_tests.cpp" \
  -o "$BUILD_DIR/w4a_completion_interface_tests" \
  > "$LOG_DIR/w4a_completion_interface_tests_compile.log" 2>&1
"$BUILD_DIR/w4a_completion_interface_tests" \
  > "$LOG_DIR/w4a_completion_interface_tests.log" 2>&1

${CXX:-g++} -std=c++11 -I"$ROOT/include" "${COMMON_SRCS[@]}" \
  "$ROOT/tb/differential/w4b_multi_rob_complete_tests.cpp" \
  -o "$BUILD_DIR/w4b_multi_rob_complete_tests" \
  > "$LOG_DIR/w4b_multi_rob_complete_tests_compile.log" 2>&1
"$BUILD_DIR/w4b_multi_rob_complete_tests" \
  > "$LOG_DIR/w4b_multi_rob_complete_tests.log" 2>&1

${CXX:-g++} -std=c++11 -DBOOM_HLS_W4A_COMPLETION_DIAGNOSTIC \
  -I"$ROOT/include" "${COMMON_SRCS[@]}" \
  "$ROOT/tb/differential/w3_dual_execute_random_tests.cpp" \
  -o "$BUILD_DIR/w4a_random_diagnostic" \
  > "$LOG_DIR/w4a_random_diagnostic_compile.log" 2>&1
"$BUILD_DIR/w4a_random_diagnostic" > "$LOG_DIR/w4a_random_diagnostic.log" 2>&1

grep -q 'Software suites:.*400 passed, 0 failed' \
  "$REPORT_DIR/product_full/regression_after.md"
grep -q 'W4A completion interfaces: 19 passed, 0 failed' \
  "$LOG_DIR/w4a_completion_interface_tests.log"
grep -q 'W4B multi ROB complete: 17 passed, 0 failed' \
  "$LOG_DIR/w4b_multi_rob_complete_tests.log"
grep -q 'random differential: PASS' "$LOG_DIR/w4a_random_diagnostic.log"

python3 - "$LOG_DIR/w4b_multi_rob_complete_tests.log" "$REPORT_DIR/w4b_metrics.csv" <<'PY'
import csv
import re
import sys
from pathlib import Path

log, output = map(Path, sys.argv[1:])
text = log.read_text(encoding="utf-8")
match = re.search(r"W4B_METRICS peak_rob_complete=(\d+) peak_prf_writes=(\d+) peak_wakeups=(\d+) total_rob_complete=(\d+) total_prf_writes=(\d+) total_wakeups=(\d+)", text)
if not match:
    raise SystemExit("missing W4B metrics")
rob, prf, wakeup, total_rob, total_prf, total_wakeup = map(int, match.groups())
if rob < 2 or prf != 1 or wakeup != 1:
    raise SystemExit(f"W4B metric guard failed: {rob}, {prf}, {wakeup}")
if total_rob <= 0 or total_prf <= 0 or total_wakeup != total_prf:
    raise SystemExit(f"W4B total metric guard failed: {total_rob}, {total_prf}, {total_wakeup}")
with output.open("w", newline="", encoding="utf-8") as stream:
    writer = csv.writer(stream)
    writer.writerow(("metric", "observed", "requirement", "status"))
    writer.writerow(("peak_rob_complete", rob, ">=2", "PASS"))
    writer.writerow(("peak_prf_writes", prf, "=1", "PASS"))
    writer.writerow(("peak_wakeups", wakeup, "=1", "PASS"))
    writer.writerow(("total_rob_complete", total_rob, ">0", "PASS"))
    writer.writerow(("total_prf_writes", total_prf, ">0", "PASS"))
    writer.writerow(("total_wakeups", total_wakeup, "=total_prf_writes", "PASS" if total_wakeup == total_prf else "FAIL"))
PY

python3 - "$REPORT_DIR/product_full/logs/w3_dual_execute_random_tests.log" \
  "$LOG_DIR/w4a_random_diagnostic.log" "$REPORT_DIR/w4b_event_deltas.csv" <<'PY'
import csv
import re
import sys
from pathlib import Path

product_path, baseline_path, output = map(Path, sys.argv[1:])
def metrics(path):
    return {name: int(value) for name, value in
            re.findall(r"^METRIC,([^,]+),(\d+)$", path.read_text(), re.M)}
product = metrics(product_path)
baseline = metrics(baseline_path)
rows = [
    ("issue", "accepted_uops"),
    ("execute", "execute_events"),
    ("completion", "completion_consumed"),
    ("writeback", "product_prf_writes"),
    ("rob_complete", "product_rob_completes"),
    ("commit", "committed_tokens"),
    ("total_cycles", "total_random_cycles"),
]
required = {name for _, name in rows}
if not required <= product.keys() or not required <= baseline.keys():
    raise SystemExit("missing W4A/W4B event-delta metrics")
if product["product_peak_rob_completes"] < 2 or product["product_peak_prf_writes"] != 1 or product["product_peak_wakeups"] != 1:
    raise SystemExit("default-product random peak guard failed")
with output.open("w", newline="", encoding="utf-8") as stream:
    writer = csv.writer(stream)
    writer.writerow(("event", "metric", "w4a_diagnostic", "w4b_product", "delta"))
    for event, name in rows:
        writer.writerow((event, name, baseline[name], product[name], product[name] - baseline[name]))
PY

printf '%s\n' 'Gate 4.0 W4B regressions complete: product 400/400; W4A diagnostic 19/19; W4B 17/17.'
