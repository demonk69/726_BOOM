#!/usr/bin/env bash
set -uo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
REPORT_DIR="$ROOT/reports/gate4_0/w3"
FULL_CORE_DIR="$REPORT_DIR/full_core_rtl"
HLS_PROJECT="$FULL_CORE_DIR/hls_project"
SOLUTION=solution_w3_full_core_rtl
RTL_DIR="$HLS_PROJECT/$SOLUTION/syn/verilog"
XSIM_BUILD="$FULL_CORE_DIR/xsim"
LOG_DIR="$REPORT_DIR/full_core_rtl_logs"
TRACE_DIR="$REPORT_DIR/rtl_traces"
NORMALIZED_DIR="$REPORT_DIR/normalized_rtl_traces"
COMPARE_DIR="$REPORT_DIR/rtl_comparisons"
EVIDENCE_DIR="$REPORT_DIR/regression/hls_traces"
STATUS_CSV="$REPORT_DIR/rtl_run_status.csv"
MATRIX_CSV="$REPORT_DIR/full_core_rtl_matrix.csv"
VITIS_HLS_BIN=${VITIS_HLS:-/home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls}

CASES=(
  "R0_POWER_ON_RESET:independent_alu" "R1_RESET_FRONTEND_OUTSTANDING:independent_alu"
  "R2_RESET_ROB_NONEMPTY:raw_chain" "R3_RESET_IQ_NONEMPTY:raw_chain"
  "R4_RESET_LOAD_PENDING:load_store" "R5_RESET_STORE_PENDING:tohost"
  "R6_RESET_BRANCH_RECOVERY:branch_taken" "R7_RESET_TRACE_BACKPRESSURE:independent_alu"
  "B0_TRACE_READY_ALWAYS:independent_alu" "B1_TRACE_STALL_1:raw_chain"
  "B2_TRACE_STALL_BURST:branch_taken" "B3_TRACE_RANDOM_STALL:nested_branch"
  "B4_TRACE_STALL_AT_BRANCH_COMMIT:branch_taken" "B5_TRACE_STALL_AT_STORE_COMMIT:independent_alu"
  "I0_IMEM_READY_ALWAYS:independent_alu" "I1_IMEM_REQ_STALL:raw_chain"
  "I2_IMEM_RESPONSE_DELAY_1:independent_alu" "I3_IMEM_RESPONSE_DELAY_4:branch_not_taken"
  "I4_IMEM_RANDOM_DELAY:nested_branch" "I5_IMEM_STALE_RESPONSE_AFTER_REDIRECT:branch_taken"
  "I6_IMEM_RESPONSE_DURING_RESET:independent_alu" "D0_DMEM_READY_ALWAYS:load_store"
  "D1_STORE_REQ_STALL:independent_alu" "D2_LOAD_REQ_STALL:load_store"
  "D3_LOAD_RESPONSE_DELAY:load_store" "D4_LOAD_RANDOM_DELAY:load_store"
  "D5_STALE_LOAD_RESPONSE_AFTER_FLUSH:load_store" "D6_STORE_DURING_BRANCH_FLUSH:branch_taken"
  "D7_DMEM_RESPONSE_DURING_RESET:load_store" "D8_TOHOST_STORE_BACKPRESSURE:tohost"
  "P0_RESET_AND_BRANCH_MISPREDICT:branch_taken" "P1_RESET_AND_LOAD_RESPONSE:load_store"
  "P2_RESET_AND_STORE_ACCEPT:tohost" "P3_EXCEPTION_AND_BRANCH_MISPREDICT:branch_taken"
  "P4_BRANCH_MISPREDICT_AND_LOAD_RESPONSE:load_store" "P5_COMMIT_AND_TRACE_BACKPRESSURE:independent_alu"
  "P6_COMMIT_AND_STORE_BACKPRESSURE:tohost" "P7_REDIRECT_AND_IMEM_RESPONSE:branch_taken"
  "N0_NORMAL_INDEPENDENT_ALU:independent_alu" "N1_NORMAL_RAW_CHAIN:raw_chain"
  "N2_NORMAL_BRANCH_TAKEN:branch_taken" "N3_NORMAL_BRANCH_NOT_TAKEN:branch_not_taken"
  "N4_NORMAL_NESTED_BRANCH:nested_branch" "N5_NORMAL_LOAD_STORE:load_store"
  "N6_NORMAL_TOHOST:tohost" "R8_DOUBLE_RUNTIME_RESET:independent_alu"
  "R9_RESET_DURING_RESET_INITIALIZATION:independent_alu"
  "R10_RESET_IMMEDIATELY_AFTER_RELEASE:independent_alu"
  "R11_RESET_AFTER_TOHOST_CLEAR_AND_RESTART:tohost"
)

if [[ ${#CASES[@]} -ne 49 ]]; then
  printf 'ERROR: W3 full-core RTL suite has %d cases, expected 49\n' "${#CASES[@]}" >&2
  exit 2
fi
if [[ ! -x "$VITIS_HLS_BIN" ]]; then
  printf 'ERROR: Vitis HLS executable not found: %s\n' "$VITIS_HLS_BIN" >&2
  exit 127
fi
if ! "$VITIS_HLS_BIN" -version 2>&1 | grep -q 'v2021.2'; then
  printf '%s\n' 'ERROR: Vitis HLS 2021.2 is required.' >&2
  exit 2
fi

mkdir -p "$REPORT_DIR"
rm -rf "$FULL_CORE_DIR" "$LOG_DIR" "$TRACE_DIR" "$NORMALIZED_DIR" "$COMPARE_DIR"
mkdir -p "$FULL_CORE_DIR" "$LOG_DIR/suite" "$TRACE_DIR" "$NORMALIZED_DIR" "$COMPARE_DIR"

"$VITIS_HLS_BIN" -version > "$REPORT_DIR/tool_version.txt" 2>&1
"$ROOT/scripts/generate_merged.sh" > "$LOG_DIR/generate_merged.log" 2>&1 || {
  printf '%s\n' "ERROR: merged-source generation failed; see $LOG_DIR/generate_merged.log" >&2
  exit 1
}
if ! (cd "$ROOT" && sha256sum -c "reports/gate4_0/w3/regression/source_hashes_after.txt") \
    > "$LOG_DIR/w3_evidence_source_check.log" 2>&1; then
  printf '%s\n' 'ERROR: current sources do not match the W3 C++/csim evidence manifest.' >&2
  exit 1
fi
for program in branch_not_taken branch_taken independent_alu nested_branch raw_chain load_store tohost; do
  for producer in hls_cpp hls_csim; do
    evidence="$EVIDENCE_DIR/${program}_${producer}_full.jsonl"
    if [[ ! -s "$evidence" ]]; then
      printf 'ERROR: missing current W3 evidence: %s\n' "$evidence" >&2
      exit 1
    fi
  done
done

cat > "$FULL_CORE_DIR/run_csynth.tcl" <<EOF
source $ROOT/scripts/create_project.tcl
source $ROOT/directives/baseline_directives.tcl
csynth_design
close_project
exit
EOF

(
  cd "$FULL_CORE_DIR" || exit 1
  BOOM_HLS_GATE=gate4_0_w3_full_core BOOM_HLS_TOP=boom_core_top \
    BOOM_HLS_PROJECT=hls_project BOOM_HLS_SOLUTION="$SOLUTION" \
    FPGA_PART=${FPGA_PART:-xczu7ev-ffvc1156-2-e} CLOCK_PERIOD=${CLOCK_PERIOD:-10} \
    "$VITIS_HLS_BIN" -f "$FULL_CORE_DIR/run_csynth.tcl"
) > "$LOG_DIR/hls_generation.log" 2>&1 || {
  printf '%s\n' "ERROR: W3 boom_core_top RTL generation failed; see $LOG_DIR/hls_generation.log" >&2
  exit 1
}

if [[ ! -s "$RTL_DIR/boom_core_top.v" ]]; then
  printf 'ERROR: generated boom_core_top RTL not found under %s\n' "$RTL_DIR" >&2
  exit 1
fi
GATE3_9_RTL_DIR="$RTL_DIR" GATE3_9_XSIM_BUILD="$XSIM_BUILD" \
  "$ROOT/scripts/gate3_9/build_xsim.sh" > "$LOG_DIR/xsim_build.log" 2>&1 || {
  printf '%s\n' "ERROR: W3 full-core XSim build failed; see $LOG_DIR/xsim_build.log" >&2
  exit 1
}

printf '%s\n' 'test,program,run_status,runtime_seconds,log,trace' > "$STATUS_CSV"
printf '%s\n' 'test,program,xsim_status,normalize_status,architecture_status,runtime_seconds,cpp_records,csim_records,rtl_records,log,trace,normalized_trace,comparison' > "$MATRIX_CSV"
overall=0
for item in "${CASES[@]}"; do
  scenario=${item%%:*}
  program=${item#*:}
  stem="${program}_${scenario}"
  start=$SECONDS
  log="$LOG_DIR/$stem.log"
  trace="$TRACE_DIR/$stem.jsonl"
  normalized="$NORMALIZED_DIR/$stem.jsonl"
  comparison="$COMPARE_DIR/$stem.json"
  wrapper_log="$LOG_DIR/suite/$stem.stdout.log"
  xsim_status=FAIL
  normalize_status=FAIL
  architecture_status=FAIL
  if GATE3_9_XSIM_BUILD="$XSIM_BUILD" TRACE="$trace" LOG="$log" \
      "$ROOT/scripts/gate3_9/run_xsim.sh" "$scenario" "$program" > "$wrapper_log" 2>&1; then
    xsim_status=PASS
    if python3 "$ROOT/scripts/gate3_8/normalize_rtl_trace.py" "$trace" "$normalized" \
        >> "$wrapper_log" 2>&1; then
      normalize_status=PASS
      if python3 "$ROOT/scripts/gate3_8/compare_cpp_csim_rtl.py" \
          --cpp "$EVIDENCE_DIR/${program}_hls_cpp_full.jsonl" \
          --csim "$EVIDENCE_DIR/${program}_hls_csim_full.jsonl" \
          --rtl "$trace" --output "$comparison" >> "$wrapper_log" 2>&1; then
        architecture_status=PASS
      fi
    fi
  fi
  if [[ "$xsim_status" != PASS || "$normalize_status" != PASS || "$architecture_status" != PASS ]]; then
    overall=1
  fi
  runtime=$((SECONDS-start))
  read -r cpp_records csim_records rtl_records < <(python3 - "$comparison" <<'PY'
import json
import sys
from pathlib import Path
path = Path(sys.argv[1])
data = json.loads(path.read_text()) if path.is_file() else {}
print(data.get("cpp_records", 0), data.get("csim_records", 0), data.get("rtl_records", 0))
PY
  )
  run_status=XSIM_FAIL
  [[ "$xsim_status" == PASS ]] && run_status=XSIM_PASS
  printf '%s,%s,%s,%s,%s,%s\n' "$scenario" "$program" "$run_status" "$runtime" \
    "reports/gate4_0/w3/full_core_rtl_logs/$stem.log" \
    "reports/gate4_0/w3/rtl_traces/$stem.jsonl" >> "$STATUS_CSV"
  printf '%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n' \
    "$scenario" "$program" "$xsim_status" "$normalize_status" "$architecture_status" \
    "$runtime" "$cpp_records" "$csim_records" "$rtl_records" \
    "reports/gate4_0/w3/full_core_rtl_logs/$stem.log" \
    "reports/gate4_0/w3/rtl_traces/$stem.jsonl" \
    "reports/gate4_0/w3/normalized_rtl_traces/$stem.jsonl" \
    "reports/gate4_0/w3/rtl_comparisons/$stem.json" >> "$MATRIX_CSV"
  printf '%-44s %-16s XSIM=%s ARCH=%s (%ss)\n' \
    "$scenario" "$program" "$xsim_status" "$architecture_status" "$runtime"
done

if ! W3_RTL_BUILD_DIR="$REPORT_DIR/focused_rtl_work" \
    "$ROOT/scripts/gate4_0/run_w3_rtl.sh" > "$LOG_DIR/focused_w3_rtl.stdout.log" 2>&1; then
  printf '%s\n' "ERROR: focused W3 RTL suite failed; see $LOG_DIR/focused_w3_rtl.stdout.log" >&2
  overall=1
fi

python3 - "$ROOT" "$REPORT_DIR" "$RTL_DIR" "$MATRIX_CSV" <<'PY'
import csv
import hashlib
import json
import sys
from pathlib import Path

root, report, rtl_dir, matrix_path = map(Path, sys.argv[1:])
source_paths = sorted(list((root / "include").glob("*.hpp")) +
                      [path for path in (root / "src").glob("*.cpp") if path.name != "boom_all.cpp"])
source_lines = [f"{hashlib.sha256(path.read_bytes()).hexdigest()}  {path.relative_to(root)}" for path in source_paths]
(report / "full_core_source_hashes.txt").write_text("\n".join(source_lines) + "\n")
rtl_paths = sorted(path for path in rtl_dir.iterdir() if path.is_file() and path.suffix in (".v", ".dat"))
rtl_lines = [f"{hashlib.sha256(path.read_bytes()).hexdigest()}  {path.relative_to(report)}" for path in rtl_paths]
(report / "full_core_rtl_hashes.txt").write_text("\n".join(rtl_lines) + "\n")
with matrix_path.open(newline="") as stream:
    rows = list(csv.DictReader(stream))
full_pass = sum(row["xsim_status"] == row["normalize_status"] == row["architecture_status"] == "PASS" for row in rows)
focused_path = report / "rtl_test_matrix.csv"
with focused_path.open(newline="") as stream:
    focused = list(csv.DictReader(stream))
focused_pass = sum(row["status"] == "PASS" for row in focused)
merged = root / "src/boom_core_merged.cpp"
source_manifest = report / "full_core_source_hashes.txt"
rtl_manifest = report / "full_core_rtl_hashes.txt"
summary = {
    "status": "PASS" if full_pass == 49 and focused_pass == 11 else "FAIL",
    "full_core_cases": len(rows),
    "full_core_pass": full_pass,
    "full_core_fail": len(rows) - full_pass,
    "focused_cases": len(focused),
    "focused_pass": focused_pass,
    "focused_fail": len(focused) - focused_pass,
    "merged_source_sha256": hashlib.sha256(merged.read_bytes()).hexdigest(),
    "source_manifest_sha256": hashlib.sha256(source_manifest.read_bytes()).hexdigest(),
    "rtl_manifest_sha256": hashlib.sha256(rtl_manifest.read_bytes()).hexdigest(),
    "generated_rtl_files": len(rtl_paths),
    "warnings": ["Official Chipyard/FESVR/DRAMSim full-system flow is not part of this HLS-subset RTL suite."],
}
(report / "full_core_rtl_summary.json").write_text(json.dumps(summary, indent=2, sort_keys=True) + "\n")
artifacts = []
for path in sorted(report.rglob("*")):
    if path.is_file() and path.name != "full_core_artifact_hashes.csv":
        artifacts.append((str(path.relative_to(report)), path.stat().st_size,
                          hashlib.sha256(path.read_bytes()).hexdigest()))
with (report / "full_core_artifact_hashes.csv").open("w", newline="") as stream:
    writer = csv.writer(stream)
    writer.writerow(("path", "bytes", "sha256"))
    writer.writerows(artifacts)
if summary["status"] != "PASS":
    raise SystemExit(1)
PY
summary_status=$?
((summary_status != 0)) && overall=1

python3 - "$REPORT_DIR/full_core_rtl_summary.json" <<'PY'
import json
import sys
data = json.load(open(sys.argv[1]))
print(f"Gate 4.0 W3 full-core RTL: {data['full_core_pass']}/{data['full_core_cases']} PASS; "
      f"focused RTL: {data['focused_pass']}/{data['focused_cases']} PASS")
print(f"merged_source_sha256={data['merged_source_sha256']}")
print(f"rtl_manifest_sha256={data['rtl_manifest_sha256']}")
PY
exit "$overall"
