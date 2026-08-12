#!/usr/bin/env bash
set -uo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
REPORT="$ROOT/reports/gate4_0/w4"
FULL="$REPORT/full_core_rtl"
PROJECT="$FULL/hls_project"
SOLUTION=solution_w4_full_core_rtl
RTL="$PROJECT/$SOLUTION/syn/verilog"
XSIM_BUILD="$FULL/xsim"
LOGS="$REPORT/full_core_rtl_logs"
TRACES="$ROOT/reference/rtl_traces/w4/full_core"
NORMALIZED="$REPORT/normalized_rtl_traces"
COMPARE="$REPORT/rtl_comparisons"
EVIDENCE="$REPORT/regression/w4e/product_full/hls_traces"
STATUS="$REPORT/full_core_rtl_status.csv"
MATRIX="$REPORT/full_core_rtl_matrix.csv"
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
[[ ${#CASES[@]} -eq 49 ]] || { printf '%s\n' 'ERROR: canonical full-core matrix is not 49 cases' >&2; exit 2; }
if ! "$VITIS_HLS_BIN" -version 2>&1 | python3 -c 'import sys; raise SystemExit("v2021.2" not in sys.stdin.read())'; then
  printf '%s\n' 'ERROR: Vitis HLS 2021.2 is required.' >&2; exit 2
fi

if [[ ${W4_SKIP_FOCUSED:-0} != 1 ]]; then
  "$ROOT/scripts/gate4_0/run_w4_rtl.sh" || exit $?
fi

rm -rf "$FULL" "$LOGS" "$NORMALIZED" "$COMPARE" "$TRACES"
mkdir -p "$FULL" "$LOGS/suite" "$TRACES" "$NORMALIZED" "$COMPARE"
for program in branch_not_taken branch_taken independent_alu nested_branch raw_chain load_store tohost; do
  for producer in hls_cpp hls_csim; do
    [[ -s "$EVIDENCE/${program}_${producer}_full.jsonl" ]] || {
      printf 'ERROR: missing approved W4E evidence %s\n' "$EVIDENCE/${program}_${producer}_full.jsonl" >&2; exit 1;
    }
  done
done

cat > "$FULL/run_hls.tcl" <<EOF
source $ROOT/scripts/create_project.tcl
source $ROOT/directives/baseline_directives.tcl
csynth_design
close_project
exit
EOF
(
  cd "$FULL" || exit 1
  BOOM_HLS_GATE=gate4_0_w4_full_core BOOM_HLS_TOP=boom_core_top \
    BOOM_HLS_PROJECT=hls_project BOOM_HLS_SOLUTION="$SOLUTION" \
    FPGA_PART=${FPGA_PART:-xczu7ev-ffvc1156-2-e} CLOCK_PERIOD=${CLOCK_PERIOD:-10} \
    "$VITIS_HLS_BIN" -f "$FULL/run_hls.tcl"
) > "$LOGS/hls_generation.log" 2>&1 || {
  printf '%s\n' "ERROR: full-core RTL generation failed; see $LOGS/hls_generation.log" >&2; exit 1;
}
[[ -s "$RTL/boom_core_top.v" ]] || { printf '%s\n' 'ERROR: boom_core_top.v was not generated' >&2; exit 1; }

GATE3_9_RTL_DIR="$RTL" GATE3_9_XSIM_BUILD="$XSIM_BUILD" \
  "$ROOT/scripts/gate3_9/build_xsim.sh" > "$LOGS/xsim_build.log" 2>&1 || {
  printf '%s\n' "ERROR: full-core XSim build failed; see $LOGS/xsim_build.log" >&2; exit 1;
}

printf '%s\n' 'test,program,run_status,runtime_seconds,log,trace' > "$STATUS"
printf '%s\n' 'test,program,xsim_status,normalize_status,architecture_status,runtime_seconds,cpp_records,csim_records,rtl_records,log,trace,normalized_trace,comparison' > "$MATRIX"
overall=0
for item in "${CASES[@]}"; do
  scenario=${item%%:*}; program=${item#*:}; stem="${program}_${scenario}"; start=$SECONDS
  log="$LOGS/$stem.log"; trace="$TRACES/$stem.jsonl"; normalized="$NORMALIZED/$stem.jsonl"
  comparison="$COMPARE/$stem.json"; stdout="$LOGS/suite/$stem.stdout.log"
  xs=FAIL; norm=FAIL; arch=FAIL
  if GATE3_9_XSIM_BUILD="$XSIM_BUILD" TRACE="$trace" LOG="$log" \
      "$ROOT/scripts/gate3_9/run_xsim.sh" "$scenario" "$program" > "$stdout" 2>&1; then
    xs=PASS
    if python3 "$ROOT/scripts/gate3_8/normalize_rtl_trace.py" "$trace" "$normalized" >> "$stdout" 2>&1; then
      norm=PASS
      if python3 "$ROOT/scripts/gate3_8/compare_cpp_csim_rtl.py" \
          --cpp "$EVIDENCE/${program}_hls_cpp_full.jsonl" \
          --csim "$EVIDENCE/${program}_hls_csim_full.jsonl" \
          --rtl "$trace" --output "$comparison" >> "$stdout" 2>&1; then arch=PASS; fi
    fi
  fi
  [[ "$xs$norm$arch" == PASSPASSPASS ]] || overall=1
  runtime=$((SECONDS-start))
  read -r cpp_records csim_records rtl_records < <(python3 - "$comparison" <<'PY'
import json, sys
from pathlib import Path
p=Path(sys.argv[1]); d=json.loads(p.read_text()) if p.is_file() else {}
print(d.get("cpp_records",0), d.get("csim_records",0), d.get("rtl_records",0))
PY
  )
  printf '%s,%s,%s,%s,%s,%s\n' "$scenario" "$program" "$xs" "$runtime" \
    "reports/gate4_0/w4/full_core_rtl_logs/$stem.log" "reference/rtl_traces/w4/full_core/$stem.jsonl" >> "$STATUS"
  printf '%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n' "$scenario" "$program" "$xs" "$norm" "$arch" \
    "$runtime" "$cpp_records" "$csim_records" "$rtl_records" \
    "reports/gate4_0/w4/full_core_rtl_logs/$stem.log" "reference/rtl_traces/w4/full_core/$stem.jsonl" \
    "reports/gate4_0/w4/normalized_rtl_traces/$stem.jsonl" \
    "reports/gate4_0/w4/rtl_comparisons/$stem.json" >> "$MATRIX"
  printf '%-44s XSIM=%s NORM=%s ARCH=%s (%ss)\n' "$scenario" "$xs" "$norm" "$arch" "$runtime"
done

python3 - "$ROOT" "$REPORT" "$RTL" "$MATRIX" <<'PY'
import csv, hashlib, json, re, sys
from pathlib import Path
root, report, rtl, matrix = map(Path, sys.argv[1:])
with matrix.open(newline="") as stream: rows=list(csv.DictReader(stream))
focused_path=report/"rtl_test_matrix.csv"; w3_path=report/"w3_current/rtl_test_matrix.csv"
with focused_path.open(newline="") as stream: focused=list(csv.DictReader(stream))
with w3_path.open(newline="") as stream: w3=list(csv.DictReader(stream))
for rows_to_check, fields in ((rows, ("log", "trace", "normalized_trace", "comparison")),
                              (focused, ("log", "trace")), (w3, ("log",))):
    for row in rows_to_check:
        for field in fields:
            path=root/row[field]
            if not path.is_file(): raise SystemExit(f"missing matrix-linked {field}: {row[field]}")
top=rtl/"boom_core_top.v"; text=top.read_text(errors="replace")
ports=(("io_success",1),("io_halted",1),("io_trap",1),("io_cycle_valid",1),("io_cycle",64),("io_instret",64))
status_rows=[]
for name,width in ports:
    pattern=rf"(?m)^output\s+(?:\[{width-1}:0\]\s+)?{name};$" if width>1 else rf"(?m)^output\s+{name};$"
    ok=bool(re.search(pattern,text)); status_rows.append((name,"output",width,ok));
    if not ok: raise SystemExit(f"missing generated status output {name}")
with (report/"full_core_status_ports.csv").open("w",newline="") as stream:
    w=csv.writer(stream); w.writerow(("port","direction","width","status"));
    for name,direction,width,ok in status_rows: w.writerow((name,direction,width,"PASS" if ok else "FAIL"))
two_we=all(name in text for name in
           ("state_int_rf_bank0_we0", "state_int_rf_bank1_we0",
            "state_int_rf_bank0_U", "state_int_rf_bank1_U"))
with (report/"full_core_prf_ports.csv").open("w",newline="") as stream:
    w=csv.writer(stream); w.writerow(("rtl","bank0_we0","bank1_we0","lvt","status"));
    w.writerow((top.relative_to(root),"present" if "state_int_rf_bank0_we0" in text else "missing",
                "present" if "state_int_rf_bank1_we0" in text else "missing",
                "present" if "state_int_rf_latest_bank" in text else "missing",
                "PASS" if two_we else "FAIL"))
rtl_paths=sorted(p for p in rtl.iterdir() if p.is_file() and p.suffix in (".v",".dat"))
with (report/"full_core_rtl_hashes.txt").open("w") as out:
    for p in rtl_paths: out.write(f"{hashlib.sha256(p.read_bytes()).hexdigest()}  {p.relative_to(root)}\n")
source_names=("boom_core_step.cpp","frontend.cpp","rvc.cpp","decode.cpp","rename.cpp","rob.cpp",
              "issue.cpp","execute.cpp","branch.cpp","lsu.cpp","completion.cpp","commit.cpp",
              "csr.cpp","reset.cpp","boom_core_top.cpp")
source_paths=sorted(root.glob("include/*.hpp"))+[root/"src"/name for name in source_names]
with (report/"full_core_source_hashes.txt").open("w") as out:
    for p in source_paths: out.write(f"{hashlib.sha256(p.read_bytes()).hexdigest()}  {p.relative_to(root)}\n")
full_pass=sum(r["xsim_status"]==r["normalize_status"]==r["architecture_status"]=="PASS" for r in rows)
summary={"status":"PASS" if full_pass==49 and sum(r["status"]=="PASS" for r in focused)==20 and
         sum(r["status"]=="PASS" for r in w3)==11 and two_we else "FAIL",
 "scope":"Generated Vitis HLS 2021.2 boom_core_top RTL; canonical 49-case XSim with normalized commit/tohost architecture comparison; W4 and W3-current focused diagnostic RTL. This is verification evidence, not a final synthesis or W4 signoff claim.",
 "final_claim":False,
 "full_core_cases":len(rows),"full_core_pass":full_pass,"full_core_fail":len(rows)-full_pass,
 "focused_cases":len(focused),"focused_pass":sum(r["status"]=="PASS" for r in focused),
 "w3_cases":len(w3),"w3_pass":sum(r["status"]=="PASS" for r in w3),
 "generated_rtl_files":len(rtl_paths),"two_prf_write_enables":two_we,
 "prf_structure":"two replicated 52x64 banks plus 52-bit LVT",
 "status_outputs":all(r[3] for r in status_rows),
 "product_source_manifest_sha256":hashlib.sha256((report/"full_core_source_hashes.txt").read_bytes()).hexdigest(),
 "generation_merged_source_sha256":hashlib.sha256((root/"src/boom_core_merged.cpp").read_bytes()).hexdigest(),
 "top_rtl_sha256":hashlib.sha256(top.read_bytes()).hexdigest(),
 "rtl_manifest_sha256":hashlib.sha256((report/"full_core_rtl_hashes.txt").read_bytes()).hexdigest(),
 "warnings":["Official Chipyard/FESVR/DRAMSim full-system flow is outside this HLS-subset RTL suite.",
             "This evidence is W4 verification input, not a W4 final claim."]}
(report/"full_core_rtl_summary.json").write_text(json.dumps(summary,indent=2,sort_keys=True)+"\n")
artifacts=[]
for p in sorted(report.rglob("*")):
    if p.is_file() and p.name!="full_core_artifact_hashes.csv":
        artifacts.append((p.relative_to(report),p.stat().st_size,hashlib.sha256(p.read_bytes()).hexdigest()))
with (report/"full_core_artifact_hashes.csv").open("w",newline="") as stream:
    w=csv.writer(stream); w.writerow(("path","bytes","sha256")); w.writerows(artifacts)
for manifest in (report/"full_core_rtl_hashes.txt", report/"full_core_source_hashes.txt"):
    for line in manifest.read_text().splitlines():
        digest,name=line.split(None,1); path=root/name
        if not path.is_file() or hashlib.sha256(path.read_bytes()).hexdigest()!=digest:
            raise SystemExit(f"manifest verification failed: {name}")
with (report/"full_core_artifact_hashes.csv").open(newline="") as stream:
    artifact_rows=list(csv.DictReader(stream))
if any(row["path"]=="full_core_artifact_hashes.csv" for row in artifact_rows):
    raise SystemExit("recursive artifact manifest entry")
for row in artifact_rows:
    path=report/row["path"]
    if not path.is_file() or path.stat().st_size!=int(row["bytes"]) or hashlib.sha256(path.read_bytes()).hexdigest()!=row["sha256"]:
        raise SystemExit(f"artifact manifest verification failed: {row['path']}")
if summary["status"]!="PASS": raise SystemExit(1)
PY
(( $? != 0 )) && overall=1

python3 - "$REPORT/full_core_rtl_summary.json" <<'PY'
import json,sys
d=json.load(open(sys.argv[1]))
print(f"Gate 4.0 W4 RTL: focused {d['focused_pass']}/{d['focused_cases']}; W3 {d['w3_pass']}/{d['w3_cases']}; full core {d['full_core_pass']}/{d['full_core_cases']}")
print(f"top_rtl_sha256={d['top_rtl_sha256']}")
print(f"rtl_manifest_sha256={d['rtl_manifest_sha256']}")
PY
exit "$overall"
