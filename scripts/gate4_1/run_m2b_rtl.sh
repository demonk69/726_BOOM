#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
BUILD_ROOT=${BOOM_M2B_RTL_BUILD_DIR:-"$ROOT/build/gate4_1/m2b_rtl"}
REPORT_DIR=${BOOM_M2B_RTL_REPORT_DIR:-"$ROOT/reports/gate4_1/m2/m2b/rtl"}
VITIS_HLS_BIN=${VITIS_HLS:-/home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls}
XVLOG=${XVLOG:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xvlog}
XELAB=${XELAB:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xelab}
XSIM=${XSIM:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xsim}
SOLUTION=solution_gate4_1_m2b_rtl

if ! "$VITIS_HLS_BIN" -version 2>&1 | grep -q 'v2021\.2'; then
  printf '%s\n' 'Vitis HLS 2021.2 is required.' >&2
  exit 1
fi

rm -rf "$BUILD_ROOT" "$REPORT_DIR"
mkdir -p "$BUILD_ROOT/projects" "$BUILD_ROOT/focused_xsim" "$REPORT_DIR/logs" \
  "$REPORT_DIR/traces"
"$ROOT/scripts/generate_merged.sh" > "$REPORT_DIR/logs/generate_merged.log" 2>&1

generate_rtl() {
  local top=$1
  (
    cd "$BUILD_ROOT/projects"
    BOOM_HLS_GATE=gate4_1_m2b_rtl BOOM_HLS_TOP="$top" \
      BOOM_HLS_PROJECT="$top" BOOM_HLS_SOLUTION="$SOLUTION" \
      BOOM_HLS_CFLAGS_EXTRA= FPGA_PART=xczu7ev-ffvc1156-2-e CLOCK_PERIOD=10 \
      "$VITIS_HLS_BIN" -f "$ROOT/scripts/run_top_csynth.tcl"
  ) > "$REPORT_DIR/logs/${top}_generation.log" 2>&1
}

generate_rtl synth_execute_top
EXECUTE_RTL="$BUILD_ROOT/projects/synth_execute_top/$SOLUTION/syn/verilog"
cp "$EXECUTE_RTL"/*.dat "$BUILD_ROOT/focused_xsim/" 2>/dev/null || true
mapfile -t EXECUTE_FILES < <(printf '%s\n' "$EXECUTE_RTL"/*.v | sort)
(
  cd "$BUILD_ROOT/focused_xsim"
  "$XVLOG" "${EXECUTE_FILES[@]}"
  "$XVLOG" --sv "$ROOT/rtl_tb/gate4_1/m2b_execute_tb.sv"
  "$XELAB" m2b_execute_tb -s m2b_execute_snapshot -timescale 1ns/1ps
) > "$REPORT_DIR/logs/focused_xsim_build.log" 2>&1
(
  cd "$BUILD_ROOT/focused_xsim"
  "$XSIM" m2b_execute_snapshot --runall --onerror quit \
    --log "$REPORT_DIR/logs/focused_execute.log"
) > "$REPORT_DIR/logs/focused_execute.stdout.log" 2>&1
grep -q 'M2B_FOCUSED_RTL_PASS cases=10' "$REPORT_DIR/logs/focused_execute.log"

generate_rtl boom_core_top
CORE_RTL="$BUILD_ROOT/projects/boom_core_top/$SOLUTION/syn/verilog"
CORE_XSIM="$BUILD_ROOT/core_xsim"
GATE3_9_RTL_DIR="$CORE_RTL" GATE3_9_XSIM_BUILD="$CORE_XSIM" \
  "$ROOT/scripts/gate3_9/build_xsim.sh" > "$REPORT_DIR/logs/core_xsim_build.log" 2>&1

printf '%s\n' 'scenario,status,trace,log' > "$REPORT_DIR/full_core_rtl_matrix.csv"
for scenario in N0_NORMAL_INDEPENDENT_ALU B1_TRACE_STALL_1; do
  trace="$REPORT_DIR/traces/${scenario}.jsonl"
  log="$REPORT_DIR/logs/full_core_${scenario}.log"
  GATE3_9_XSIM_BUILD="$CORE_XSIM" \
    PROGRAM="$ROOT/tb/programs/boom_reference/m2b_mul_family.hex" \
    TRACE="$trace" LOG="$log" MAX_CYCLES=100000 \
    "$ROOT/scripts/gate3_9/run_xsim.sh" "$scenario" m2b_mul_family \
    > "$REPORT_DIR/logs/full_core_${scenario}.stdout.log" 2>&1
  printf '%s,PASS,%s,%s\n' "$scenario" \
    "${trace#$ROOT/}" "${log#$ROOT/}" >> "$REPORT_DIR/full_core_rtl_matrix.csv"
done

python3 - "$REPORT_DIR" <<'PY'
import json
import sys
from pathlib import Path

report = Path(sys.argv[1])
expected = {3: 0xffffffffffffffeb, 4: 0xffffffffffffffff,
            5: 0xffffffffffffffff, 6: 6, 7: 0xffffffffffffffeb,
            8: 0xffffffffffffffec, 9: 0, 10: 0, 11: 7,
            12: 0xffffffffffffffec}
for trace in sorted((report / "traces").glob("*.jsonl")):
    commits = {}
    tohost = False
    for line in trace.read_text().splitlines():
        event = json.loads(line)
        if event.get("event") == "commit" and event.get("rd_valid"):
            commits[event["rd"]] = int(event["rd_value"], 16)
        tohost |= event.get("event") == "tohost" and int(event.get("value", "0"), 16) == 1
    if not tohost or any(commits.get(rd) != value for rd, value in expected.items()):
        raise SystemExit(f"full-core RTL architectural mismatch: {trace}")
(report / "rtl_summary.md").write_text(
    "# Gate 4.1 M2B RTL\n\n"
    "- Focused generated synth_execute_top RTL: 10/10 PASS.\n"
    "- Generated boom_core_top RTL multiply programs: 2/2 PASS.\n"
    "- Both full-core traces reached tohost=1 and matched 10 architectural register values.\n",
    encoding="utf-8")
PY

printf '%s\n' 'Gate 4.1 M2B focused and full-core generated RTL complete.'
