#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)"}
HLS_WORK=${G6_L1R80_FULL_CORE_WORK:-/home/lab_726/opencode_tmp/g6_l1_path_b_full_core_retry}
SIM_WORK=${G6_L1R80_PATH_B_SIM_WORK:-/home/lab_726/opencode_tmp/g6_l1_path_b_xsim}
REPORT=${G6_L1R80_REPORT:-$ROOT/reports/gate6_0_full_lsu/l1/focused_rtl_redesign}
RTL=$HLS_WORK/boom_hls_g6_l1r_default_8_8_boom_core_top/solution_module/syn/verilog
TB=$ROOT/rtl_tb/gate6_0/g6_l1r_focused_80_path_b_tb.sv
CATALOG=$ROOT/scripts/gate6_0/focused_cases.csv
RESULTS=$REPORT/canonical_focused_rtl_acceptance.csv
EVIDENCE=$REPORT/path_b_case_evidence.csv
XVLOG=${XVLOG:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xvlog}
XELAB=${XELAB:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xelab}
XSIM=${XSIM:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xsim}
SOURCE_HASH=b3cd4bdb13dc00e4b0293bc506268005bb8633b0e0036fc5e18f39314bbab618
EXPECTED_RTL_HASH=c0165ccea43ccc41d1d2cc5514d8af17764b700b84a84af751c4b331be55fcbe
CASES=(13 28 29 36 38 39 40 47 48 49)

[[ "$SIM_WORK" == /home/lab_726/opencode_tmp/* ]] || {
    printf 'ERROR: simulation workspace must remain under /home/lab_726/opencode_tmp\n' >&2
    exit 2
}
[[ -s "$RTL/boom_core_top.v" ]] || {
    printf 'ERROR: bounded-retry RTL is missing; HLS retry must not be rerun\n' >&2
    exit 2
}
for tool in "$XVLOG" "$XELAB" "$XSIM" python3 timeout; do
    command -v "$tool" >/dev/null 2>&1 || { printf 'ERROR: missing tool %s\n' "$tool" >&2; exit 127; }
done
mkdir -p -- "$SIM_WORK" "$REPORT/logs"

read -r RTL_HASH TB_HASH < <(python3 - "$ROOT" "$RTL" "$TB" "$CATALOG" "$RESULTS" <<'PY'
import csv, hashlib, sys
from pathlib import Path
root, rtl, tb, catalog, results = map(Path, sys.argv[1:])
rows = list(csv.DictReader(catalog.open(newline="")))
if [int(row["case_id"]) for row in rows] != list(range(80)):
    raise SystemExit("canonical catalog IDs are not exact ordered 0..79")
path_b = {13, 28, 29, 36, 38, 39, 40, 47, 48, 49}
accepted = list(csv.DictReader(results.open(newline="")))
accepted_ids = [int(row["case_id"]) for row in accepted]
if accepted_ids != [case for case in range(80) if case not in path_b]:
    raise SystemExit("acceptance is not the exact frozen PATH_A 70-case set")
paths = sorted((root / "src").glob("*.cpp")) + sorted((root / "include").glob("*.hpp"))
paths = [path for path in paths if path.name not in ("boom_all.cpp", "boom_core_merged.cpp")]
digest = hashlib.sha256()
for path in paths:
    digest.update(str(path.relative_to(root)).encode() + b"\0")
    digest.update(bytes.fromhex(hashlib.sha256(path.read_bytes()).hexdigest()))
if digest.hexdigest() != "b3cd4bdb13dc00e4b0293bc506268005bb8633b0e0036fc5e18f39314bbab618":
    raise SystemExit("frozen product source mismatch")
digest = hashlib.sha256()
for path in sorted(rtl.glob("*.v")) + sorted(rtl.glob("*.dat")):
    digest.update(path.name.encode() + b"\0")
    digest.update(bytes.fromhex(hashlib.sha256(path.read_bytes()).hexdigest()))
print(digest.hexdigest(), hashlib.sha256(tb.read_bytes()).hexdigest())
PY
)
[[ "$RTL_HASH" == "$EXPECTED_RTL_HASH" ]] || {
    printf 'ERROR: generated RTL hash mismatch: %s\n' "$RTL_HASH" >&2
    exit 2
}

cp -- "$RTL"/*.dat "$SIM_WORK/" 2>/dev/null || true
mapfile -t rtl_files < <(printf '%s\n' "$RTL"/*.v | sort)
(
    cd -- "$SIM_WORK"
    timeout 300 "$XVLOG" "${rtl_files[@]}" >"$REPORT/logs/path_b_xvlog_rtl.log" 2>&1
    timeout 180 "$XVLOG" --sv "$ROOT/rtl_tb/axis_imem_model.sv" \
        "$ROOT/rtl_tb/axis_dmem_model.sv" "$TB" >"$REPORT/logs/path_b_xvlog_tb.log" 2>&1
    timeout 300 "$XELAB" g6_l1r_focused_80_path_b_tb -s path_b_snapshot -timescale 1ns/1ps \
        >"$REPORT/logs/path_b_xelab.log" 2>&1
)

declare -A PROGRAMS=(
    [13]=load_store [28]=load_store [29]=tohost [36]=packet_fault [38]=independent_alu
    [39]=independent_alu [40]=independent_alu [47]=load_store [48]=load_store [49]=load_store
)
declare -A STIMULUS=(
    [13]=actual_load_issue_with_forced_generation_mismatch
    [28]=actual_load_request_with_axis_tready_low
    [29]=committed_store_request_with_axis_tready_low
    [36]=seeded_lq_sq_pending_then_actual_exception_and_late_response
    [38]=power_on_staged_reset_from_empty
    [39]=seeded_partial_lq_sq_then_runtime_reset
    [40]=seeded_full_sq_nonempty_lq_then_runtime_reset
    [47]=actual_load_issue_with_forced_rob_allocation_mismatch
    [48]=committed_store_stall_followed_by_younger_load
    [49]=actual_pending_load_response_then_runtime_reset_and_stale_response
)

tmp_evidence=$SIM_WORK/path_b_case_evidence.csv
printf 'case_id,case_name,source_hash,rtl_hash,testbench_hash,stimulus,expected,actual,result,evidence\n' >"$tmp_evidence"
for case_id in "${CASES[@]}"; do
    case_name=$(python3 - "$CATALOG" "$case_id" <<'PY'
import csv, sys
with open(sys.argv[1], newline="") as stream:
    print(next(row["case_name"] for row in csv.DictReader(stream) if int(row["case_id"]) == int(sys.argv[2])))
PY
)
    expected=$(python3 - "$CATALOG" "$case_id" <<'PY'
import csv, sys
with open(sys.argv[1], newline="") as stream:
    print(next(row["asserted_requirement"] for row in csv.DictReader(stream) if int(row["case_id"]) == int(sys.argv[2])))
PY
)
    program_name=${PROGRAMS[$case_id]}
    if [[ "$program_name" == packet_fault ]]; then
        program=$ROOT/tb/programs/b3i_packet/build/packet_fault.hex
    else
        program=$ROOT/tb/programs/boom_reference/build/$program_name.hex
    fi
    clean_program=$SIM_WORK/${program_name}_${case_id}.readmemh
    python3 - "$program" "$clean_program" <<'PY'
import sys
from pathlib import Path
source, target = map(Path, sys.argv[1:])
words=[]
for line in source.read_text().splitlines():
    line=line.split('#', 1)[0].strip()
    if line: words.append(line)
target.write_text("\n".join(words)+"\n")
PY
    log=$REPORT/logs/case_${case_id}_${case_name}_path_b.log
    (
        cd -- "$SIM_WORK"
        timeout 300 "$XSIM" path_b_snapshot --runall --onerror quit \
            --testplusarg "CASE_ID=$case_id" --testplusarg "PROGRAM=$clean_program" \
            --testplusarg "PROGRAM_NAME=$program_name" --testplusarg "MAX_CYCLES=60000" \
            --log "$log" >"$log.stdout" 2>&1
    )
    marker=$(grep -F "G6_L1R80_PATH_B_PASS id=$case_id " "$log")
    actual=${marker#*actual=}
    python3 - "$tmp_evidence" "$case_id" "$case_name" "$SOURCE_HASH" "$RTL_HASH" "$TB_HASH" \
        "${STIMULUS[$case_id]}" "$expected" "$actual" "$log" <<'PY'
import csv, sys
path=sys.argv[1]
with open(path, "a", newline="") as stream:
    csv.writer(stream).writerow(sys.argv[2:10] + ["PASS", sys.argv[10]])
PY
done

python3 - "$RESULTS" "$tmp_evidence" "$CATALOG" "$SOURCE_HASH" "$RTL_HASH" <<'PY'
import csv, os, sys
results, evidence, catalog, source_hash, rtl_hash = sys.argv[1:]
path_b={13,28,29,36,38,39,40,47,48,49}
with open(evidence, newline="") as stream:
    evidence_rows=list(csv.DictReader(stream))
ids=[int(row["case_id"]) for row in evidence_rows]
if ids != sorted(path_b): raise SystemExit(f"PATH_B evidence IDs mismatch: {ids}")
with open(results, newline="") as stream:
    reader=csv.DictReader(stream); fields=reader.fieldnames; rows=list(reader)
if [int(row["case_id"]) for row in rows] != [case for case in range(80) if case not in path_b]:
    raise SystemExit("PATH_A acceptance changed during PATH_B run")
for row in evidence_rows:
    rows.append({
        "case_id": row["case_id"], "case_name": row["case_name"], "path": "PATH_B",
        "image": "full_core_bounded_retry", "top": "boom_core_top", "status": "PASS",
        "source_hash": source_hash, "rtl_hash": rtl_hash, "evidence": row["evidence"],
    })
rows.sort(key=lambda row: int(row["case_id"]))
if [int(row["case_id"]) for row in rows] != list(range(80)):
    raise SystemExit("combined acceptance is not exact ordered 0..79")
temp=results+".tmp"
with open(temp, "w", newline="") as stream:
    writer=csv.DictWriter(stream, fieldnames=fields); writer.writeheader(); writer.writerows(rows)
os.replace(temp, results)
PY
cp -- "$tmp_evidence" "$EVIDENCE"
printf 'G6_L1R80_PATH_B_PASS cases=10 rtl_hash=%s testbench_hash=%s\n' "$RTL_HASH" "$TB_HASH"
printf 'FOCUSED_RTL_STATUS=PASS_80_OF_80\n'
