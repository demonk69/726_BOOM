#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)"}
WORK=${G6_L1R_WORK:-/tmp/boom_hls/g6_l1_closure/focused}
REPORT=${G6_L1R_REPORT_DIR:-$WORK/report}
NATIVE_ONLY=0
NATIVE_TIMEOUT=${G6_L1R_NATIVE_TIMEOUT:-300}
SYNTH_TIMEOUT=${G6_L1R_SYNTH_TIMEOUT:-7200}
COMPILE_TIMEOUT=${G6_L1R_COMPILE_TIMEOUT:-600}
ELAB_TIMEOUT=${G6_L1R_ELAB_TIMEOUT:-600}
XSIM_TIMEOUT=${G6_L1R_XSIM_TIMEOUT:-3600}
while (($#)); do
    case "$1" in
        --report) REPORT=$2; shift 2 ;;
        --work) WORK=$2; shift 2 ;;
        --native-only) NATIVE_ONLY=1; shift ;;
        --native-timeout) NATIVE_TIMEOUT=$2; shift 2 ;;
        --synth-timeout) SYNTH_TIMEOUT=$2; shift 2 ;;
        --compile-timeout) COMPILE_TIMEOUT=$2; shift 2 ;;
        --elab-timeout) ELAB_TIMEOUT=$2; shift 2 ;;
        --xsim-timeout) XSIM_TIMEOUT=$2; shift 2 ;;
        *) printf 'usage: %s [--report DIR] [--work DIR] [--native-only] [--native-timeout SEC] [--synth-timeout SEC] [--compile-timeout SEC] [--elab-timeout SEC] [--xsim-timeout SEC]\n' "$0" >&2; exit 2 ;;
    esac
done

VITIS_HLS=${VITIS_HLS:-/home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls}
XVLOG=${XVLOG:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xvlog}
XELAB=${XELAB:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xelab}
XSIM=${XSIM:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xsim}
CXX=${CXX:-g++}
PART=${FPGA_PART:-xczu7ev-ffvc1156-2-e}
PERIOD=${CLOCK_PERIOD:-10}
LQ=${LQ_DEPTH:-8}
SQ=${SQ_DEPTH:-8}
CATALOG="$ROOT/scripts/gate6_0/focused_cases.csv"
TOP=g6_l1r_focused_top
TB="$ROOT/rtl_tb/gate6_0/g6_l1r_focused_tb.sv"
TIMING="$REPORT/focused_phase_timing.csv"

[[ "$PART" == xczu7ev-ffvc1156-2-e && "$PERIOD" == 10 ]] || {
    printf 'ERROR: Gate 6.0 L1R requires xczu7ev-ffvc1156-2-e at 10 ns\n' >&2; exit 2;
}
[[ "$LQ" == 8 && "$SQ" == 8 ]] || {
    printf 'ERROR: focused closure requires LQ_DEPTH=8 SQ_DEPTH=8\n' >&2; exit 2;
}
for value in "$NATIVE_TIMEOUT" "$SYNTH_TIMEOUT" "$COMPILE_TIMEOUT" "$ELAB_TIMEOUT" "$XSIM_TIMEOUT"; do
    [[ "$value" =~ ^[1-9][0-9]*$ ]] || { printf 'ERROR: phase timeouts must be positive integer seconds\n' >&2; exit 2; }
done
command -v timeout >/dev/null 2>&1 || { printf 'ERROR: GNU timeout is required\n' >&2; exit 127; }
command -v "$CXX" >/dev/null 2>&1 || { printf 'ERROR: missing tool %s\n' "$CXX" >&2; exit 127; }

python3 - "$CATALOG" <<'PY'
import csv
import sys
from pathlib import Path
rows = list(csv.DictReader(Path(sys.argv[1]).open(newline="")))
ids = [int(row["case_id"]) for row in rows]
if ids != list(range(80)) or len({r["case_name"] for r in rows}) != 80:
    raise SystemExit("catalog must contain exactly unique ordered IDs 0..79")
if len({(r["variant_dimensions"], r["asserted_requirement"]) for r in rows}) != 80:
    raise SystemExit("catalog scenarios are not genuinely distinct")
if sum(r["category"] == "variant" for r in rows) != 30:
    raise SystemExit("catalog must contain exactly 30 variants")
text = " ".join(" ".join(r.values()).lower() for r in rows)
required = ("empty", "single", "full", "wrap", "reuse", "fffe", "ffff", "stale lq",
            "stale sq", "correct response", "response mismatch", "same-address", "no forwarding",
            "actual full request", "committed store", "branch clear", "branch squash", "pending kill",
            "late response", "exception flush", "reset", "count lifecycle", "full-free-allocate",
            "owner tuple", "rob reuse", "simultaneous ordering", "physical register")
missing = [item for item in required if item not in text]
if missing:
    raise SystemExit("catalog missing required coverage: " + ", ".join(missing))
print("G6_L1R_CATALOG_PASS cases=80 variants=30")
PY

mkdir -p -- "$WORK" "$REPORT/logs" "$WORK/xsim"
printf 'phase,timeout_seconds,start_epoch_ns,end_epoch_ns,duration_seconds,status,exit_code,evidence\n' >"$TIMING"

record_phase() {
    local phase=$1 limit=$2 start=$3 end=$4 rc=$5 evidence=$6 status
    if [[ $rc -eq 0 ]]; then status=PASS; elif [[ $rc -eq 124 ]]; then status=TIMEOUT; else status=FAIL; fi
    python3 - "$TIMING" "$phase" "$limit" "$start" "$end" "$status" "$rc" "$evidence" <<'PY'
import csv, sys
path, phase, limit, start, end, status, rc, evidence = sys.argv[1:]
with open(path, "a", newline="") as stream:
    csv.writer(stream).writerow((phase, limit, start, end,
        f"{(int(end)-int(start))/1e9:.6f}", status, rc, evidence))
PY
}

readarray -t AGGREGATES < <(python3 - "$ROOT" <<'PY'
import hashlib, sys
from pathlib import Path
root = Path(sys.argv[1])
def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()
def pf6(paths):
    digest = hashlib.sha256()
    for path in paths:
        digest.update(str(path.relative_to(root)).encode())
        digest.update(b"\0")
        digest.update(bytes.fromhex(sha(path)))
    return digest.hexdigest()
canonical = sorted((root/"src").glob("*.cpp")) + sorted((root/"include").glob("*.hpp"))
canonical = [p for p in canonical if p.name not in ("boom_all.cpp", "boom_core_merged.cpp")]
focused = sorted((root/"include").glob("*.hpp")) + [
    root/"src/boom_core_merged.cpp", root/"tb/differential/g6_l1r_focused_top.cpp"]
build_digest = hashlib.sha256()
for path in focused:
    build_digest.update(str(path.relative_to(root)).encode() + b"\0")
    build_digest.update(bytes.fromhex(sha(path)))
build_digest.update(b"-std=c++11 -DLQ_DEPTH=8 -DSQ_DEPTH=8 -DBOOM_FTQ_STORAGE_LUTRAM -DBOOM_PREDICTOR_STORAGE_LUTRAM")
print(pf6(canonical))
print(build_digest.hexdigest())
PY
)
CANONICAL_PRODUCT_AGGREGATE=${AGGREGATES[0]}
FOCUSED_BUILD_INPUT_AGGREGATE=${AGGREGATES[1]}
printf '%s\n' "$CANONICAL_PRODUCT_AGGREGATE" >"$REPORT/canonical_product_source_aggregate.sha256"
printf '%s\n' "$FOCUSED_BUILD_INPUT_AGGREGATE" >"$REPORT/focused_build_input_aggregate.sha256"

native_compile_log="$REPORT/logs/native_compile.log"
start=$(date +%s%N); set +e
timeout --signal=TERM --kill-after=300s "$NATIVE_TIMEOUT" "$CXX" -std=c++11 -O1 -Wno-unknown-pragmas \
    -DLQ_DEPTH=8 -DSQ_DEPTH=8 -DBOOM_FTQ_STORAGE_LUTRAM -DBOOM_PREDICTOR_STORAGE_LUTRAM \
    -DG6_L1R_NATIVE_DIAGNOSTIC -I"$ROOT/include" "$ROOT/src/boom_core_merged.cpp" \
    "$ROOT/tb/differential/g6_l1r_focused_top.cpp" -o "$WORK/g6_l1r_native" \
    >"$native_compile_log" 2>&1
rc=$?; set -e; end=$(date +%s%N)
record_phase native_compile "$NATIVE_TIMEOUT" "$start" "$end" "$rc" "$native_compile_log"
[[ $rc -eq 0 ]] || { printf 'ERROR: native compile failed or timed out\n' >&2; exit "$rc"; }

native_log="$REPORT/logs/native_diagnostic.log"
start=$(date +%s%N); set +e
timeout --signal=TERM --kill-after=300s "$NATIVE_TIMEOUT" "$WORK/g6_l1r_native" >"$native_log" 2>&1
rc=$?; set -e; end=$(date +%s%N)
record_phase native_diagnostic "$NATIVE_TIMEOUT" "$start" "$end" "$rc" "$native_log"
[[ $rc -eq 0 ]] && python3 - "$native_log" <<'PY'
import sys
text=open(sys.argv[1],errors="replace").read()
if text.count("G6_L1R_COMMAND_ENGINE_PASS commands=8") != 1:
    raise SystemExit("native command engine marker missing or duplicated")
PY
[[ $rc -eq 0 ]] || { printf 'ERROR: native diagnostic failed or timed out\n' >&2; exit "$rc"; }
if ((NATIVE_ONLY)); then
    printf 'G6_L1R_COMMAND_ENGINE_PASS commands=8 report=%s\n' "$REPORT"; exit 0
fi

for tool in "$VITIS_HLS" "$XVLOG" "$XELAB" "$XSIM"; do
    command -v "$tool" >/dev/null 2>&1 || { printf 'ERROR: missing tool %s\n' "$tool" >&2; exit 127; }
    version_output=$("$tool" -version 2>&1 || true)
    [[ "$version_output" == *2021.2* ]] || { printf 'ERROR: 2021.2 required: %s\n' "$tool" >&2; exit 2; }
done

start=$(date +%s%N); set +e
timeout --signal=TERM --kill-after=300s "$SYNTH_TIMEOUT" env HLS_BOOM_ROOT="$ROOT" G6_L1R_WORK="$WORK" FPGA_PART="$PART" \
    CLOCK_PERIOD="$PERIOD" LQ_DEPTH="$LQ" SQ_DEPTH="$SQ" "$VITIS_HLS" \
    -f "$ROOT/scripts/gate6_0/l1r_focused_csynth.tcl" >"$REPORT/logs/csynth.log" 2>&1
rc=$?; set -e; end=$(date +%s%N)
record_phase hls_synthesis "$SYNTH_TIMEOUT" "$start" "$end" "$rc" "$REPORT/logs/csynth.log"
[[ $rc -eq 0 ]] || { printf 'ERROR: HLS synthesis failed or timed out\n' >&2; exit "$rc"; }

RTL="$WORK/hls_project/solution/syn/verilog"
[[ -s "$RTL/$TOP.v" ]] || { printf 'ERROR: missing generated top RTL\n' >&2; exit 1; }
mapfile -t RTL_FILES < <(printf '%s\n' "$RTL"/*.v | sort)
GENERATED_RTL_AGGREGATE=$(python3 - "$RTL" <<'PY'
import hashlib, sys
from pathlib import Path
root=Path(sys.argv[1]); paths=sorted(root.glob("*.v"))+sorted(root.glob("*.dat"))
if not paths: raise SystemExit("no generated RTL")
d=hashlib.sha256()
for p in paths: d.update(p.name.encode()+b"\0"); d.update(bytes.fromhex(hashlib.sha256(p.read_bytes()).hexdigest()))
print(d.hexdigest())
PY
)
printf '%s\n' "$GENERATED_RTL_AGGREGATE" >"$REPORT/focused_generated_rtl_aggregate.sha256"
cp -- "$RTL"/*.dat "$WORK/xsim/" 2>/dev/null || true

start=$(date +%s%N); set +e
(cd -- "$WORK/xsim" && timeout --signal=TERM --kill-after=300s "$COMPILE_TIMEOUT" "$XVLOG" "${RTL_FILES[@]}" >"$REPORT/logs/xvlog_rtl.log" 2>&1 &&
 timeout --signal=TERM --kill-after=300s "$COMPILE_TIMEOUT" "$XVLOG" --sv "$TB" >"$REPORT/logs/xvlog_tb.log" 2>&1)
rc=$?; set -e; end=$(date +%s%N)
record_phase rtl_compile "$COMPILE_TIMEOUT" "$start" "$end" "$rc" "$REPORT/logs/xvlog_rtl.log;$REPORT/logs/xvlog_tb.log"
[[ $rc -eq 0 ]] || { printf 'ERROR: RTL compile failed or timed out\n' >&2; exit "$rc"; }

start=$(date +%s%N); set +e
(cd -- "$WORK/xsim" && timeout --signal=TERM --kill-after=300s "$ELAB_TIMEOUT" "$XELAB" g6_l1r_focused_tb \
    -s g6_l1r_focused_snapshot -timescale 1ns/1ps >"$REPORT/logs/xelab.log" 2>&1)
rc=$?; set -e; end=$(date +%s%N)
record_phase rtl_elaboration "$ELAB_TIMEOUT" "$start" "$end" "$rc" "$REPORT/logs/xelab.log"
[[ $rc -eq 0 ]] || { printf 'ERROR: RTL elaboration failed or timed out\n' >&2; exit "$rc"; }

start=$(date +%s%N); set +e
(cd -- "$WORK/xsim" && timeout --signal=TERM --kill-after=300s "$XSIM_TIMEOUT" "$XSIM" g6_l1r_focused_snapshot \
    --runall --onerror quit --log "$REPORT/logs/focused_xsim.log" \
    >"$REPORT/logs/focused_xsim.stdout.log" 2>&1)
rc=$?; set -e; end=$(date +%s%N)
record_phase rtl_cases "$XSIM_TIMEOUT" "$start" "$end" "$rc" "$REPORT/logs/focused_xsim.log"
[[ $rc -eq 0 ]] || { printf 'ERROR: focused XSim failed or timed out\n' >&2; exit "$rc"; }
XSIM_RUNTIME=$(python3 -c "print(f'{($end-$start)/1e9:.6f}')")

python3 - "$CATALOG" "$REPORT/logs/focused_xsim.log" "$REPORT" "$XSIM_RUNTIME" \
    "$CANONICAL_PRODUCT_AGGREGATE" "$FOCUSED_BUILD_INPUT_AGGREGATE" "$GENERATED_RTL_AGGREGATE" <<'PY'
import csv, re, sys
from pathlib import Path
catalog, log, report = map(Path, sys.argv[1:4])
runtime, canonical, focused, rtl = sys.argv[4:8]
rows=list(csv.DictReader(catalog.open(newline=""))); text=log.read_text(errors="replace")
ids=[int(x) for x in re.findall(r"G6_L1R_CASE_PASS id=(\d+)",text)]
if ids != list(range(80)) or "G6_L1R_CASE_FAIL" in text:
    raise SystemExit(f"expected ordered PASS IDs 0..79 exactly once; observed {ids}")
if text.count("G6_L1R_FOCUSED_RTL_PASS cases=80") != 1:
    raise SystemExit("suite completion marker missing or duplicated")
manifest=[]; matrix=[]
for row in rows:
    manifest.append((row["case_id"],row["case_name"],row["category"],row["variant_dimensions"],
        row["asserted_requirement"],"PASS","PASS",runtime,canonical,focused,rtl,str(log)))
    matrix.append((row["case_id"],row["case_name"],row["category"],"PASS","PASS","PASS",str(log)))
with (report/"focused_rtl_case_manifest.csv").open("w",newline="") as f:
    w=csv.writer(f); w.writerow(("case_id","scenario","category","variant_dimensions","asserted_requirement",
        "expected_status","observed_status","suite_runtime_seconds","canonical_product_aggregate",
        "focused_build_input_aggregate","generated_rtl_aggregate","evidence")); w.writerows(manifest)
with (report/"focused_rtl_matrix.csv").open("w",newline="") as f:
    w=csv.writer(f); w.writerow(("case_id","scenario","category","expected","observed","status","evidence")); w.writerows(matrix)
print("G6_L1R_FOCUSED_RTL_PASS cases=80")
PY
printf 'G6_L1R_FOCUSED_RTL_PASS cases=80 report=%s\n' "$REPORT"
