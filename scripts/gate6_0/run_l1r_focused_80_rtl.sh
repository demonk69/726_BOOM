#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)"}
WORK=${G6_L1R80_WORK:-/home/lab_726/opencode_tmp/g6_l1_focused_80}
REPORT=${G6_L1R80_REPORT:-$ROOT/reports/gate6_0_full_lsu/l1/focused_rtl_redesign}
VITIS_HLS=${VITIS_HLS:-/home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls}
XVLOG=${XVLOG:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xvlog}
XELAB=${XELAB:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xelab}
XSIM=${XSIM:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xsim}
CXX=${CXX:-g++}
IMAGE_TIMEOUT=${G6_L1R80_IMAGE_TIMEOUT:-900}
SOFT_KB=${G6_L1R80_SOFT_KB:-4194304}
HARD_KB=${G6_L1R80_HARD_KB:-8388608}
START_IMAGE=${G6_L1R80_START_IMAGE:-1}
STOP_IMAGE=${G6_L1R80_STOP_IMAGE:-6}
FORCE_IMAGE=${G6_L1R80_FORCE_REBUILD_IMAGE:-0}
CATALOG=$ROOT/scripts/gate6_0/focused_cases.csv
TB=$ROOT/rtl_tb/gate6_0/g6_l1r_focused_80_path_a_tb.sv
EXPECTED=$WORK/path_a_native_expected.csv
RESULTS=$REPORT/canonical_focused_rtl_acceptance.csv
RESOURCES=$REPORT/focused_rtl_80_resource_summary.csv

[[ "$WORK" == /home/lab_726/opencode_tmp/* ]] || {
    printf 'ERROR: workspace must remain under /home/lab_726/opencode_tmp\n' >&2
    exit 2
}
[[ "$START_IMAGE" =~ ^[1-6]$ && "$STOP_IMAGE" =~ ^[1-6]$ && "$START_IMAGE" -le "$STOP_IMAGE" ]] || {
    printf 'ERROR: image range must satisfy 1 <= start <= stop <= 6\n' >&2
    exit 2
}
[[ "$FORCE_IMAGE" =~ ^[0-6]$ ]] || { printf 'ERROR: force image must be 0..6\n' >&2; exit 2; }
for tool in "$VITIS_HLS" "$XVLOG" "$XELAB" "$XSIM" "$CXX" setsid timeout /usr/bin/time python3; do
    command -v "$tool" >/dev/null 2>&1 || { printf 'ERROR: missing tool %s\n' "$tool" >&2; exit 127; }
done
mkdir -p -- "$WORK" "$REPORT/logs"

python3 - "$ROOT" "$CATALOG" <<'PY'
import csv, hashlib, sys
from pathlib import Path
root, catalog = map(Path, sys.argv[1:])
rows = list(csv.DictReader(catalog.open(newline="")))
if [int(row["case_id"]) for row in rows] != list(range(80)):
    raise SystemExit("canonical catalog IDs must be exact ordered 0..79")
if len({row["case_name"] for row in rows}) != 80:
    raise SystemExit("canonical catalog names must be unique")
paths = sorted((root / "src").glob("*.cpp")) + sorted((root / "include").glob("*.hpp"))
paths = [path for path in paths if path.name not in ("boom_all.cpp", "boom_core_merged.cpp")]
digest = hashlib.sha256()
for path in paths:
    digest.update(str(path.relative_to(root)).encode())
    digest.update(b"\0")
    digest.update(bytes.fromhex(hashlib.sha256(path.read_bytes()).hexdigest()))
actual = digest.hexdigest()
expected = "b3cd4bdb13dc00e4b0293bc506268005bb8633b0e0036fc5e18f39314bbab618"
if actual != expected:
    raise SystemExit(f"frozen product source mismatch: {actual}")
merged = hashlib.sha256((root / "src/boom_core_merged.cpp").read_bytes()).hexdigest()
if merged != "76d78e9052344a0b1e06c0e723c473d37e0b2dd7679a2ab3b8fbbeed9f1dff9c":
    raise SystemExit(f"frozen merged source mismatch: {merged}")
print("G6_L1R80_FREEZE_PASS cases=80 source_files=%d" % len(paths))
PY

native_flags=(-std=c++11 -O1 -Wno-unknown-pragmas -DLQ_DEPTH=8 -DSQ_DEPTH=8
              -DBOOM_FTQ_STORAGE_LUTRAM -DBOOM_PREDICTOR_STORAGE_LUTRAM -I"$ROOT/include")
"$CXX" "${native_flags[@]}" "$ROOT/src/boom_core_merged.cpp" \
    "$ROOT/tb/differential/g6_l1r_focused_80_tops.cpp" \
    "$ROOT/tb/differential/g6_l1r_focused_80_native.cpp" \
    -o "$WORK/path_a_native"
"$CXX" "${native_flags[@]}" \
    "$ROOT/tb/differential/g6_l1r_focused_80_older_store_top.cpp" \
    "$ROOT/tb/differential/g6_l1r_focused_80_older_store_native.cpp" \
    -o "$WORK/path_a_older_store_native"
"$WORK/path_a_native" >"$WORK/path_a_native_main.csv"
"$WORK/path_a_older_store_native" >"$WORK/path_a_native_older.csv"
python3 - "$WORK/path_a_native_main.csv" "$WORK/path_a_native_older.csv" "$EXPECTED" <<'PY'
import csv, sys
inputs = sys.argv[1:3]
rows = []
for source in inputs:
    with open(source, newline="") as stream:
        rows.extend(csv.DictReader(stream))
rows.sort(key=lambda row: int(row["case_id"]))
ids = [int(row["case_id"]) for row in rows]
path_b = {13,28,29,36,38,39,40,47,48,49}
if ids != [case for case in range(80) if case not in path_b]:
    raise SystemExit(f"PATH_A native IDs mismatch: {ids}")
with open(sys.argv[3], "w", newline="") as stream:
    writer = csv.DictWriter(stream, fieldnames=rows[0].keys())
    writer.writeheader()
    writer.writerows(rows)
print("G6_L1R80_PATH_A_NATIVE_PASS cases=70")
PY

if [[ ! -s "$RESULTS" ]]; then
    printf 'case_id,case_name,path,image,top,status,source_hash,rtl_hash,evidence\n' >"$RESULTS"
fi
if [[ ! -s "$RESOURCES" ]]; then
    printf 'image,family,top,timeout_seconds,duration_seconds,peak_workspace_kb,peak_rss_kb,soft_limit_crossed,hard_limit_crossed,exit_code,rtl_hash\n' >"$RESOURCES"
fi

declare -a FAMILIES=(queue_allocate queue_lifecycle load_response branch_recovery global_flush older_store)
declare -a TOPS=(g6_l1r80_queue_allocate g6_l1r80_queue_lifecycle g6_l1r80_load_response g6_l1r80_branch_recovery g6_l1r80_global_flush g6_l1r80_older_store)
declare -a DEFINES=(F80_QUEUE_ALLOCATE F80_QUEUE_LIFECYCLE F80_LOAD_RESPONSE F80_BRANCH_RECOVERY F80_GLOBAL_FLUSH F80_OLDER_STORE)
declare -a CASES=(
    "0 1 2 3 4 5 6 9 10 11 12"
    "7 8 14 15 41 42 43 44 46 51 55 57 61 63 67 69 73 75 79"
    "16 17 18 19 20 21 22 23 24 25 45 50 52 56 58 62 64 68 70 74 76"
    "30 31 32 33 34 35 54 60 66 72 78"
    "37 53 59 65 71 77"
    "26 27"
)

run_hls() {
    local image=$1 family=$2 top=$3 image_work=$4 wrapper=$5 product=$6 log=$7
    local start end pid rc current_kb rss_kb peak_kb=0 peak_rss=0 soft=0 hard=0
    start=$(date +%s)
    setsid timeout --signal=TERM --kill-after=30s "$IMAGE_TIMEOUT" /usr/bin/time -v \
        -o "$REPORT/logs/${family}_time.log" env HLS_BOOM_ROOT="$ROOT" \
        G6_L1R80_IMAGE_WORK="$image_work" G6_L1R80_TOP="$top" \
        G6_L1R80_WRAPPER="$wrapper" G6_L1R80_PRODUCT="$product" \
        LQ_DEPTH=8 SQ_DEPTH=8 FPGA_PART=xczu7ev-ffvc1156-2-e CLOCK_PERIOD=10 \
        "$VITIS_HLS" -f "$ROOT/scripts/gate6_0/l1r_focused_80_csynth.tcl" \
        >"$log" 2>&1 &
    pid=$!
    while kill -0 "$pid" 2>/dev/null; do
        current_kb=$(du -sk -- "$image_work" 2>/dev/null | awk '{print $1}')
        rss_kb=$(ps -eo pgid=,rss= | awk -v group="$pid" '$1 == group {sum += $2} END {print sum + 0}')
        ((current_kb > peak_kb)) && peak_kb=$current_kb
        ((rss_kb > peak_rss)) && peak_rss=$rss_kb
        ((current_kb >= SOFT_KB || rss_kb >= SOFT_KB)) && soft=1
        if ((current_kb >= HARD_KB || rss_kb >= HARD_KB)); then
            hard=1
            kill -TERM -- "-$pid" 2>/dev/null || true
            sleep 5
            kill -KILL -- "-$pid" 2>/dev/null || true
            break
        fi
        sleep 5
    done
    set +e; wait "$pid"; rc=$?; set -e
    end=$(date +%s)
    current_kb=$(du -sk -- "$image_work" 2>/dev/null | awk '{print $1}')
    ((current_kb > peak_kb)) && peak_kb=$current_kb
    if [[ -s "$REPORT/logs/${family}_time.log" ]]; then
        rss_kb=$(awk -F: '/Maximum resident set size/ {gsub(/^[[:space:]]+/, "", $2); print $2}' "$REPORT/logs/${family}_time.log")
        rss_kb=${rss_kb:-0}
        ((rss_kb > peak_rss)) && peak_rss=$rss_kb
    fi
    HLS_DURATION=$((end-start)); HLS_PEAK_KB=$peak_kb; HLS_PEAK_RSS=$peak_rss
    HLS_SOFT=$soft; HLS_HARD=$hard; HLS_RC=$rc
    return "$rc"
}

for index in "${!FAMILIES[@]}"; do
    image=$((index + 1))
    ((image < START_IMAGE || image > STOP_IMAGE)) && continue
    family=${FAMILIES[$index]}
    top=${TOPS[$index]}
    define=${DEFINES[$index]}
    image_work=$WORK/image_${image}_${family}
    sim=$image_work/sim
    rtl=$image_work/hls_project/solution/syn/verilog
    wrapper=tb/differential/g6_l1r_focused_80_tops.cpp
    product=merged
    if ((image == 6)); then
        wrapper=tb/differential/g6_l1r_focused_80_older_store_top.cpp
        product=lsu_translation_unit
    fi
    if ((image == FORCE_IMAGE)); then
        rm -rf -- "$image_work"
        python3 - "$RESULTS" "$RESOURCES" "$family" "$image" <<'PY'
import csv, os, sys
for path, key, value in ((sys.argv[1], "image", sys.argv[3]),
                         (sys.argv[2], "image", sys.argv[4])):
    with open(path, newline="") as stream:
        reader=csv.DictReader(stream); fields=reader.fieldnames
        rows=[row for row in reader if row[key] != value]
    temp=path+".tmp"
    with open(temp, "w", newline="") as stream:
        writer=csv.DictWriter(stream, fieldnames=fields); writer.writeheader(); writer.writerows(rows)
    os.replace(temp, path)
PY
    fi
    mkdir -p -- "$image_work" "$sim"
    if [[ ! -s "$rtl/$top.v" ]]; then
        hls_log=$REPORT/logs/${family}_csynth.log
        if ! run_hls "$image" "$family" "$top" "$image_work" "$wrapper" "$product" "$hls_log"; then
            printf '%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,\n' "$image" "$family" "$top" \
                "$IMAGE_TIMEOUT" "$HLS_DURATION" "$HLS_PEAK_KB" "$HLS_PEAK_RSS" \
                "$HLS_SOFT" "$HLS_HARD" "$HLS_RC" >>"$RESOURCES"
            printf 'ERROR: HLS failed for image=%s family=%s\n' "$image" "$family" >&2
            exit 1
        fi
    else
        HLS_DURATION=0; HLS_PEAK_KB=$(du -sk -- "$image_work" | awk '{print $1}')
        HLS_PEAK_RSS=0; HLS_SOFT=0; HLS_HARD=0; HLS_RC=0
    fi
    mapfile -t rtl_files < <(printf '%s\n' "$rtl"/*.v | sort)
    rtl_hash=$(python3 - "$rtl" <<'PY'
import hashlib, sys
from pathlib import Path
root=Path(sys.argv[1]); digest=hashlib.sha256()
for path in sorted(root.glob("*.v"))+sorted(root.glob("*.dat")):
    digest.update(path.name.encode()+b"\0")
    digest.update(bytes.fromhex(hashlib.sha256(path.read_bytes()).hexdigest()))
print(digest.hexdigest())
PY
)
    if ! grep -q "^${image},${family},${top}," "$RESOURCES"; then
        printf '%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n' "$image" "$family" "$top" \
            "$IMAGE_TIMEOUT" "$HLS_DURATION" "$HLS_PEAK_KB" "$HLS_PEAK_RSS" \
            "$HLS_SOFT" "$HLS_HARD" "$HLS_RC" "$rtl_hash" >>"$RESOURCES"
    fi
    cp -- "$rtl"/*.dat "$sim/" 2>/dev/null || true
    (
        cd -- "$sim"
        timeout 180 "$XVLOG" "${rtl_files[@]}" >"$REPORT/logs/${family}_xvlog_rtl.log" 2>&1
        timeout 180 "$XVLOG" --sv -d "DUT_TOP=$top" -d "$define" "$TB" \
            >"$REPORT/logs/${family}_xvlog_tb.log" 2>&1
        timeout 180 "$XELAB" g6_l1r_focused_80_path_a_tb -s "${family}_snapshot" -timescale 1ns/1ps \
            >"$REPORT/logs/${family}_xelab.log" 2>&1
    )
    for case_id in ${CASES[$index]}; do
        if grep -q "^${case_id},[^,]*,PATH_A,.*PASS," "$RESULTS"; then continue; fi
        IFS=, read -r _ _ exp0 exp1 exp2 exp3 exp4 exp5 exp6 exp7 < <(
            python3 - "$EXPECTED" "$case_id" <<'PY'
import csv, sys
with open(sys.argv[1], newline="") as stream:
    row = next(row for row in csv.DictReader(stream) if int(row["case_id"]) == int(sys.argv[2]))
print(",".join(row.values()))
PY
        )
        case_name=$(python3 - "$CATALOG" "$case_id" <<'PY'
import csv, sys
with open(sys.argv[1], newline="") as stream:
    print(next(row["case_name"] for row in csv.DictReader(stream) if int(row["case_id"]) == int(sys.argv[2])))
PY
        )
        log=$REPORT/logs/case_${case_id}_${case_name}.log
        (
            cd -- "$sim"
            timeout 300 "$XSIM" "${family}_snapshot" --runall --onerror quit \
                --testplusarg "CASE_ID=$case_id" \
                --testplusarg "EXP0=$exp0" --testplusarg "EXP1=$exp1" \
                --testplusarg "EXP2=$exp2" --testplusarg "EXP3=$exp3" \
                --testplusarg "EXP4=$exp4" --testplusarg "EXP5=$exp5" \
                --testplusarg "EXP6=$exp6" --testplusarg "EXP7=$exp7" \
                --log "$log" >"$log.stdout" 2>&1
        )
        grep -q "G6_L1R80_CASE_PASS id=$case_id " "$log"
        printf '%s,%s,PATH_A,%s,%s,PASS,b3cd4bdb13dc00e4b0293bc506268005bb8633b0e0036fc5e18f39314bbab618,%s,%s\n' \
            "$case_id" "$case_name" "$family" "$top" "$rtl_hash" "$log" >>"$RESULTS"
    done
done

python3 - "$RESULTS" <<'PY'
import csv, os, sys
with open(sys.argv[1], newline="") as stream:
    reader=csv.DictReader(stream); fields=reader.fieldnames
    all_rows=list(reader)
rows=[row for row in all_rows if row["path"] == "PATH_A" and row["status"] == "PASS"]
ids=[int(row["case_id"]) for row in rows]
expected=set(range(80))-{13,28,29,36,38,39,40,47,48,49}
if len(ids) != len(set(ids)): raise SystemExit("duplicate PATH_A result IDs")
if not set(ids).issubset(expected): raise SystemExit("unexpected PATH_A result IDs")
all_rows.sort(key=lambda row: int(row["case_id"]))
temp=sys.argv[1]+".tmp"
with open(temp, "w", newline="") as stream:
    writer=csv.DictWriter(stream, fieldnames=fields); writer.writeheader(); writer.writerows(all_rows)
os.replace(temp, sys.argv[1])
print(f"G6_L1R80_PATH_A_PROGRESS pass={len(ids)}/70")
PY
