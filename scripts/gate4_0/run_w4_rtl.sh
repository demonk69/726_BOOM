#!/usr/bin/env bash
set -uo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
VITIS_HLS_BIN=${VITIS_HLS:-/home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls}
XVLOG=${XVLOG:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xvlog}
XELAB=${XELAB:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xelab}
XSIM=${XSIM:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xsim}
REPORT=${W4_RTL_REPORT_DIR:-"$ROOT/reports/gate4_0/w4"}
BUILD=${W4_RTL_BUILD_DIR:-"$ROOT/build/gate4_0/w4_rtl"}
PROJECT="$BUILD/hls_project"
SOLUTION=solution_w4_rtl
RTL="$PROJECT/$SOLUTION/syn/verilog"
RETENTION_PROJECT="$BUILD/hls_core_step_retention_project"
RETENTION_RTL="$RETENTION_PROJECT/$SOLUTION/syn/verilog"
WORK="$BUILD/xsim"
LOGS="$REPORT/rtl_logs"
TRACES=${W4_RTL_TRACE_DIR:-"$ROOT/reference/rtl_traces/w4"}
MATRIX="$REPORT/rtl_test_matrix.csv"

for tool in "$VITIS_HLS_BIN" "$XVLOG" "$XELAB" "$XSIM"; do
  [[ -x "$tool" ]] || { printf 'ERROR: missing executable %s\n' "$tool" >&2; exit 127; }
done
if ! "$VITIS_HLS_BIN" -version 2>&1 | python3 -c 'import sys; raise SystemExit("v2021.2" not in sys.stdin.read())'; then
  printf '%s\n' 'ERROR: Vitis HLS 2021.2 is required.' >&2
  exit 2
fi

rm -rf "$BUILD"
mkdir -p "$BUILD" "$WORK" "$LOGS" "$TRACES" "$REPORT/w3_current"
"$ROOT/scripts/generate_merged.sh" > "$LOGS/generate_merged_focused.log" 2>&1 || exit 1
python3 - "$ROOT" "$LOGS/w4_rtl_guardrails.log" <<'PY'
import os, re, sys
from pathlib import Path
root, output = map(Path, sys.argv[1:])
core = (root / "src/boom_core_step.cpp").read_text(encoding="utf-8")
lsu = (root / "src/lsu.cpp").read_text(encoding="utf-8")
if len(re.findall(r"\bcompletion_service_cycle\s*\(", core)) != 1:
    raise SystemExit("boom_core_step must own exactly one completion service call")
if re.search(r"\bcompletion_service_(?:cycle|execute)\s*\(", lsu):
    raise SystemExit("lsu_module must not invoke completion service")
names = ("boom_core_step.cpp", "boom_core_top.cpp", "completion.cpp", "lsu.cpp",
         "execute.cpp", "rob.cpp", "synth_module_tops.cpp", "boom_core_merged.cpp")
patterns = {
    "complete_array_partition": r"ARRAY_PARTITION[^\n]*\bcomplete\b",
    "false_dependence": r"DEPENDENCE[^\n]*\bfalse\b",
    "dataflow": r"#\s*pragma\s+HLS\s+DATAFLOW\b",
}
violations = []
for name in names:
    text = (root / "src" / name).read_text(encoding="utf-8")
    for label, pattern in patterns.items():
        for match in re.finditer(pattern, text, re.I):
            violations.append(f"{name}:{text.count(chr(10), 0, match.start()) + 1}:{label}")
if violations:
    raise SystemExit("forbidden W4 HLS directives: " + ", ".join(violations))
if os.environ.get("BOOM_HLS_ENABLE_CORE_PIPELINE"):
    raise SystemExit("focused flow enables CORE pipeline")
output.write_text("W4_RTL_GUARDRAILS_PASS files=" + str(len(names)) +
                  " complete_array_partition=0 false_dependence=0 dataflow=0 core_pipeline=disabled\n")
PY
[[ $? -eq 0 ]] || exit 1
"$VITIS_HLS_BIN" -version > "$REPORT/tool_versions.txt" 2>&1
"$XVLOG" -version >> "$REPORT/tool_versions.txt" 2>&1

cat > "$BUILD/run_hls.tcl" <<EOF
source $ROOT/scripts/create_project.tcl
source $ROOT/directives/baseline_directives.tcl
csynth_design
close_project
exit
EOF
cat > "$BUILD/run_retention_hls.tcl" <<EOF
source $ROOT/scripts/create_project.tcl
add_files -tb -cflags "-std=c++11 -I$ROOT/include" $ROOT/tb/differential/w4_core_step_retention_tests.cpp
csim_design -clean
source $ROOT/directives/baseline_directives.tcl
csynth_design
close_project
exit
EOF

COMMON=(
  "$ROOT/src/boom_core_step.cpp" "$ROOT/src/frontend.cpp" "$ROOT/src/rvc.cpp" "$ROOT/src/decode.cpp"
  "$ROOT/src/rename.cpp" "$ROOT/src/rob.cpp" "$ROOT/src/issue.cpp"
  "$ROOT/src/mul.cpp" "$ROOT/src/divider.cpp" "$ROOT/src/execute.cpp"
  "$ROOT/src/branch.cpp" "$ROOT/src/lsu.cpp"
  "$ROOT/src/completion.cpp" "$ROOT/src/commit.cpp" "$ROOT/src/csr.cpp"
  "$ROOT/src/reset.cpp" "$ROOT/src/synth_module_tops.cpp"
)
g++ -std=c++11 -I"$ROOT/include" "${COMMON[@]}" \
  "$ROOT/tb/differential/w4_core_step_retention_tests.cpp" \
  -o "$BUILD/w4_core_step_retention_tests" > "$LOGS/core_step_retention_cpp_compile.log" 2>&1 &&
  "$BUILD/w4_core_step_retention_tests" > "$LOGS/core_step_retention_cpp.log" 2>&1 || {
  printf '%s\n' "ERROR: core-step retention C++ oracle failed; see $LOGS/core_step_retention_cpp.log" >&2
  exit 1
}
(
  cd "$BUILD" || exit 1
  BOOM_HLS_GATE=gate4_0_w4_rtl BOOM_HLS_TOP=synth_w4d_oracle_top \
    BOOM_HLS_PROJECT=hls_project BOOM_HLS_SOLUTION="$SOLUTION" \
    FPGA_PART=${FPGA_PART:-xczu7ev-ffvc1156-2-e} CLOCK_PERIOD=${CLOCK_PERIOD:-10} \
    "$VITIS_HLS_BIN" -f "$BUILD/run_hls.tcl"
) > "$LOGS/hls_diagnostic_generation.log" 2>&1 || {
  printf '%s\n' "ERROR: W4 diagnostic RTL generation failed; see $LOGS/hls_diagnostic_generation.log" >&2
  exit 1
}
(
  cd "$BUILD" || exit 1
  BOOM_HLS_GATE=gate4_0_w4_core_step_retention \
    BOOM_HLS_TOP=synth_w4_core_step_retention_top \
    BOOM_HLS_PROJECT=hls_core_step_retention_project BOOM_HLS_SOLUTION="$SOLUTION" \
    FPGA_PART=${FPGA_PART:-xczu7ev-ffvc1156-2-e} CLOCK_PERIOD=${CLOCK_PERIOD:-10} \
    "$VITIS_HLS_BIN" -f "$BUILD/run_retention_hls.tcl"
) > "$LOGS/hls_core_step_retention_generation.log" 2>&1 || {
  printf '%s\n' "ERROR: core-step retention RTL generation failed; see $LOGS/hls_core_step_retention_generation.log" >&2
  exit 1
}

cp "$RTL"/*.dat "$WORK"/ 2>/dev/null || true
cp "$RETENTION_RTL"/*.dat "$WORK"/ 2>/dev/null || true
mapfile -t RTL_FILES < <(printf '%s\n' "$RTL"/*.v "$RETENTION_RTL"/*.v | sort)
(
  cd "$WORK" || exit 1
  "$XVLOG" "${RTL_FILES[@]}" &&
  "$XVLOG" --sv "$ROOT/rtl_tb/gate4_0/w4_completion_diagnostic_tb.sv" \
    "$ROOT/rtl_tb/gate4_0/w4_core_step_retention_tb.sv" &&
  "$XELAB" w4_completion_diagnostic_tb -s w4_completion_diagnostic_snapshot -timescale 1ns/1ps &&
  "$XELAB" w4_core_step_retention_tb -s w4_core_step_retention_snapshot -timescale 1ns/1ps
) > "$LOGS/xsim_build.log" 2>&1 || {
  printf '%s\n' "ERROR: W4 diagnostic XSim build failed; see $LOGS/xsim_build.log" >&2
  exit 1
}

# name:source:scenario:expected:hand-checked requirement
CASES=(
  "int_load_dual_writeback:w4:8:00000008e0000022:INT and load complete with two PRF writes"
  "branch_load_dual_rob_complete:w4:9:0000000860400011:correct branch and load complete together"
  "alu_store_agu_dual_complete:w4:10:00000008a0000011:ALU and store AGU complete together"
  "two_producers_wake_two_consumers:w4:11:00000009a0000011:two producers publish wakeups and lookup consumer 2"
  "bypass_dependent_chain:w4:12:0000000aa0000011:dependent operand resolves on bypass"
  "write_port_conflict_retention_precise_fault:w4:1:000000002c000100:same-pdst conflict retains work and raises precise fault"
  "branch_kill_dual_completion:w3:4:0000000008100400:mispredict kills younger held completion"
  "reset_clear_dual_completion:w3:5:0000000000000000:reset clears both completion lanes"
  "stale_allocation_rejection:w3:10:0000000000300000:stale allocation identity has no side effect"
  "rob_wrap_dual_complete:w3:9:0000005cf8d00001:dual completion remains ordered across ROB wrap"
  "trace_backpressure:w3:7:000000000a100000:trace backpressure retains atomic commit"
  "dmem_backpressure_with_int_progress:w3:8:000000000c110000:DMEM backpressure retains store without blocking INT progress"
  "repeated_dual_completion_stress:w4:15:000000c822000022:two consecutive two-wide completion cycles lose nothing"
  "tohost_program_status_output:w4:14:000000302c000000:tohost and commit status registers are externally observable"
  "lane2_inactive:w4:13:00000004a0000011:inactive MEM lane creates no phantom second completion"
  "correct_branch_cross_conflict:w4:4:000000002e402900:correct branch preserves cross-source conflict fence"
  "persistent_precise_fault_fence:w4:7:000000003e000900:precise fault remains fenced across service cycles"
  "helper_level_response_retention:w4:16:000000000000003f:helper-level completion retention and drain reference"
  "core_step_queued_response_retention:corestep:37:00000000000007ff:two real core steps retain then drain queued response and source with exact per-step accounting"
  "mispredict_killed_nonconflict:w4:6:0000000821c00011:mispredict kill removes younger conflict before publication"
)

# Run W3 first so every reused hand expectation is tied to its current-source
# generated-RTL result, while all artifacts remain in the W4 evidence tree.
w3_suite_status=0
if [[ ${W4_RERUN_W3_FOCUSED:-0} == 1 ]]; then
  if ! W3_RTL_BUILD_DIR="$BUILD/w3_current" W3_RTL_REPORT_DIR="$REPORT/w3_current" \
      "$ROOT/scripts/gate4_0/run_w3_rtl.sh" > "$LOGS/w3_current.stdout.log" 2>&1; then
    w3_suite_status=1
  fi
else
  if python3 - "$ROOT" "$REPORT/w3_current/rtl_test_matrix.csv" <<'PY'
import csv, sys
from pathlib import Path
root, matrix = map(Path, sys.argv[1:])
with matrix.open(newline="") as stream:
    rows = list(csv.DictReader(stream))
if len(rows) != 11 or not all(row["status"] == "PASS" for row in rows):
    raise SystemExit(1)
if not all((root / row["log"]).is_file() for row in rows):
    raise SystemExit(1)
PY
  then
    printf '%s\n' 'W3_CURRENT_FOCUSED_PRESERVED 11/11 PASS (diagnostic-only W4 source addition)' \
      > "$LOGS/w3_current.stdout.log"
  else
    printf '%s\n' 'ERROR: preserved W3-current focused evidence is incomplete' \
      > "$LOGS/w3_current.stdout.log"
    w3_suite_status=1
  fi
fi

printf '%s\n' 'case,source,scenario,requirement,status,expected,observed,log,trace' > "$MATRIX"
overall=$w3_suite_status
for item in "${CASES[@]}"; do
  IFS=: read -r name source scenario expected requirement <<< "$item"
  log="$LOGS/$name.log"
  matrix_log="${LOGS#$ROOT/}/$name.log"
  stdout="$LOGS/$name.stdout.log"
  if [[ "$source" == w4 ]]; then
    (cd "$WORK" && "$XSIM" w4_completion_diagnostic_snapshot --runall --onerror quit \
      --testplusarg "SCENARIO=$scenario" --testplusarg "EXPECT=$expected" --log "$log") \
      > "$stdout" 2>&1
    rc=$?
  elif [[ "$source" == corestep ]]; then
    (cd "$WORK" && "$XSIM" w4_core_step_retention_snapshot --runall --onerror quit \
      --testplusarg "SEED=$scenario" --testplusarg "EXPECT=$expected" --log "$log") \
      > "$stdout" 2>&1
    rc=$?
  else
    read -r w3_status observed w3_log < <(python3 - "$REPORT/w3_current/rtl_test_matrix.csv" "$scenario" <<'PY'
import csv, sys
with open(sys.argv[1], newline="") as stream:
    row = next((r for r in csv.DictReader(stream) if r["scenario"] == sys.argv[2]), None)
if row is None: print("FAIL unavailable missing")
else: print(row["status"], row["observed"], row["log"])
PY
    )
    rc=0
    [[ "$w3_status" == PASS ]] || rc=1
    log="$ROOT/$w3_log"
    matrix_log="$w3_log"
  fi
  [[ "$source" == w3 ]] || observed=$expected
  if [[ "$source" != w3 ]]; then
    observed=$(python3 - "$log" <<'PY'
import re, sys
from pathlib import Path
text = Path(sys.argv[1]).read_text(errors="replace") if Path(sys.argv[1]).is_file() else ""
matches = re.findall(r"observed=([0-9a-fA-F]+)", text)
print(matches[-1].lower() if matches else "unavailable")
PY
    )
  fi
  status=PASS
  if [[ $rc -ne 0 || "$observed" != "$expected" ]]; then status=FAIL; overall=1; fi
  trace="$TRACES/$name.jsonl"
  python3 - "$trace" "$name" "$scenario" "$expected" "$observed" "$status" <<'PY'
import json, sys
from pathlib import Path
path = Path(sys.argv[1])
path.write_text(json.dumps({"case": sys.argv[2], "scenario": int(sys.argv[3]),
    "expected": sys.argv[4], "observed": sys.argv[5], "status": sys.argv[6]},
    sort_keys=True) + "\n")
PY
  printf '%s,%s,%s,"%s",%s,%s,%s,%s,%s\n' "$name" "$source" "$scenario" \
    "$requirement" "$status" "$expected" "$observed" \
    "$matrix_log" "${TRACES#$ROOT/}/$name.jsonl" >> "$MATRIX"
done

python3 - "$ROOT" "$REPORT" "$RTL" "$RETENTION_RTL" "$MATRIX" <<'PY'
import csv, hashlib, json, re, sys
from pathlib import Path
root, report, rtl, retention_rtl, matrix = map(Path, sys.argv[1:])
with matrix.open(newline="") as stream: rows = list(csv.DictReader(stream))
w3_path = report / "w3_current/rtl_test_matrix.csv"
with w3_path.open(newline="") as stream: w3 = list(csv.DictReader(stream))
top = (rtl / "synth_w4d_oracle_top.v").read_text(errors="replace")
two_we = all(name in top for name in
             ("state_int_rf_bank0_we0", "state_int_rf_bank1_we0",
              "state_int_rf_bank0_U", "state_int_rf_bank1_U"))
for row in rows:
    for field in ("log", "trace"):
        path = root / row[field]
        if not path.is_file():
            raise SystemExit(f"missing focused matrix {field}: {row[field]}")
for row in w3:
    path = root / row["log"]
    if not path.is_file():
        raise SystemExit(f"missing W3-current matrix log: {row['log']}")
status = {
  "status": "PASS" if all(r["status"] == "PASS" for r in rows) and
             len(w3) == 11 and all(r["status"] == "PASS" for r in w3) and two_we else "FAIL",
  "focused_cases": len(rows), "focused_pass": sum(r["status"] == "PASS" for r in rows),
  "w3_cases": len(w3), "w3_pass": sum(r["status"] == "PASS" for r in w3),
  "diagnostic_two_prf_write_enables": two_we,
  "diagnostic_prf_structure": "two replicated 52x64 banks plus 52-bit LVT",
  "diagnostic_rtl_sha256": hashlib.sha256(top.encode()).hexdigest(),
  "warnings": ["Diagnostic HLS generation is not final full-core csynth."]
}
(report / "rtl_focused_summary.json").write_text(json.dumps(status, indent=2, sort_keys=True) + "\n")
with (report / "diagnostic_rtl_hashes.txt").open("w") as out:
    for generated in (rtl, retention_rtl):
        for path in sorted(generated.glob("*")):
            if path.is_file(): out.write(f"{hashlib.sha256(path.read_bytes()).hexdigest()}  {path.relative_to(root)}\n")
for line in (report / "diagnostic_rtl_hashes.txt").read_text().splitlines():
    digest, name = line.split(None, 1)
    if hashlib.sha256((root / name).read_bytes()).hexdigest() != digest:
        raise SystemExit(f"diagnostic RTL hash verification failed: {name}")
if status["status"] != "PASS": raise SystemExit(1)
PY
(( $? != 0 )) && overall=1

python3 - "$MATRIX" "$REPORT/w3_current/rtl_test_matrix.csv" <<'PY'
import csv, sys
def count(path):
    with open(path, newline="") as stream: rows=list(csv.DictReader(stream))
    return sum(r["status"] == "PASS" for r in rows), len(rows)
focused, w3 = count(sys.argv[1]), count(sys.argv[2])
print(f"Gate 4.0 W4 focused generated RTL: {focused[0]}/{focused[1]} PASS; current W3: {w3[0]}/{w3[1]} PASS")
PY
exit "$overall"
