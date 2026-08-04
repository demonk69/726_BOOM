#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
BUILD_DIR="$ROOT/build/gate4_0/w4d_regression"
REPORT_DIR="$ROOT/reports/gate4_0/w4/regression/w4d"
LOG_DIR="$REPORT_DIR/logs"
mkdir -p "$BUILD_DIR" "$LOG_DIR"

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
compile_run() {
  local name=$1 source=$2
  shift 2
  ${CXX:-g++} -std=c++11 -I"$ROOT/include" "$@" "${COMMON_SRCS[@]}" "$source" \
    -o "$BUILD_DIR/$name" > "$LOG_DIR/${name}_compile.log" 2>&1
  "$BUILD_DIR/$name" > "$LOG_DIR/$name.log" 2>&1
}

compile_run w4a_completion_interface_tests "$ROOT/tb/differential/w4a_completion_interface_tests.cpp" -DBOOM_HLS_W4A_COMPLETION_DIAGNOSTIC
compile_run w4b_multi_rob_complete_tests "$ROOT/tb/differential/w4b_multi_rob_complete_tests.cpp"
compile_run w4_multi_wakeup_tests "$ROOT/tb/differential/w4_multi_wakeup_tests.cpp"
compile_run w4_bypass_tests "$ROOT/tb/differential/w4_bypass_tests.cpp"
compile_run w4_multi_writeback_tests "$ROOT/tb/differential/w4_multi_writeback_tests.cpp"
compile_run w4_multi_completion_tests "$ROOT/tb/differential/w4_multi_completion_tests.cpp"
compile_run w4d_rtl_oracle_tests "$ROOT/tb/differential/w4d_rtl_oracle_tests.cpp" \
  "$ROOT/src/synth_module_tops.cpp"

grep -q 'Software suites:.*400 passed, 0 failed' "$REPORT_DIR/product_full/regression_after.md"
grep -q 'W4A completion interfaces: 19 passed, 0 failed' "$LOG_DIR/w4a_completion_interface_tests.log"
grep -q 'W4B multi ROB complete: 17 passed, 0 failed' "$LOG_DIR/w4b_multi_rob_complete_tests.log"
grep -q 'W4C multi wakeup: 14 passed, 0 failed' "$LOG_DIR/w4_multi_wakeup_tests.log"
grep -q 'W4D bypass: 14 passed, 0 failed' "$LOG_DIR/w4_bypass_tests.log"
grep -q 'W4D multi writeback: 13 passed, 0 failed' "$LOG_DIR/w4_multi_writeback_tests.log"
grep -q 'W4D multi completion: 6 passed, 0 failed' "$LOG_DIR/w4_multi_completion_tests.log"
grep -q 'W4D RTL oracle preparation: 10 passed, 0 failed' "$LOG_DIR/w4d_rtl_oracle_tests.log"

python3 - "$REPORT_DIR" <<'PY'
import csv, re, sys
from pathlib import Path
report=Path(sys.argv[1]); logs=report/'logs'
text=(logs/'w4_multi_writeback_tests.log').read_text()
m=re.search(r'peak_completion_accepts=(\d+) peak_rob_completes=(\d+) peak_prf_writes=(\d+) peak_wakeups=(\d+) bounded_wait=(\d+) completion_drops=(\d+) writeback_drops=(\d+) duplicate_writes=(\d+) conflicts=(\d+) validation_faults=(\d+) fault_events=(\d+) deduplications=(\d+) hold_violation_probe=(\d+)',text)
if not m: raise SystemExit('missing W4D metrics')
values=list(map(int,m.groups()))
checks=(values[0]>=2,values[1]>=2,values[2]==2,values[3]>=2,values[4]<=2,
        values[5]==0,values[6]==0,values[7]==0,values[8]>=1,values[9]>=1,
        values[10]>=2,values[11]>=1,values[12]==1)
if not all(checks): raise SystemExit(f'W4D metric guard failed: {values}')
names=('peak_completion_accepts','peak_rob_completes','peak_prf_writes','peak_wakeups',
       'bounded_wait_cycles','dropped_completions','dropped_writebacks','duplicate_writebacks',
       'writeback_conflicts','validation_faults','fault_events','deduplications',
       'injected_hold_violation_probe')
requirements=('>=2','>=2','=2','>=2','<=2','=0','=0','=0','>=1','>=1','>=2','>=1','=1')
with (report/'w4d_metrics.csv').open('w',newline='',encoding='utf-8') as f:
    w=csv.writer(f);w.writerow(('metric','observed','requirement','status'))
    for n,v,r in zip(names,values,requirements):w.writerow((n,v,r,'PASS'))
def metrics(path):return {n:int(v) for n,v in re.findall(r'^METRIC,([^,]+),(\d+)$',path.read_text(),re.M)}
before=metrics(report.parent/'w4c'/'product_full'/'logs'/'w3_dual_execute_random_tests.log')
after=metrics(report/'product_full'/'logs'/'w3_dual_execute_random_tests.log')
names=('accepted_uops','execute_events','completion_consumed','product_prf_writes','product_wakeups','product_rob_completes','committed_tokens','total_random_cycles')
with (report/'w4d_event_cycle_deltas.csv').open('w',newline='',encoding='utf-8') as f:
    w=csv.writer(f);w.writerow(('metric','w4c','w4d','delta'))
    for n in names:w.writerow((n,before[n],after[n],after[n]-before[n]))
PY

printf '%s\n' 'Gate 4.0 W4D regressions complete: product 400/400 and cumulative W4A-W4D directed PASS.'
