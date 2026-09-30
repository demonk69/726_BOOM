#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)"}
WORK=${G6_L2_FOCUSED_WORK:-/home/lab_726/opencode_tmp/g6_l2/path_a}
REPORT=${G6_L2_FOCUSED_REPORT:-$ROOT/reports/gate6_0_full_lsu/l2/focused_rtl}
CATALOG=$ROOT/reports/gate6_0_full_lsu/l2/focused_rtl_case_catalog.csv
ACCEPTANCE=$ROOT/reports/gate6_0_full_lsu/l2/focused_rtl_acceptance.csv
VITIS_HLS=${VITIS_HLS:-/home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls}
XVLOG=${XVLOG:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xvlog}
XELAB=${XELAB:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xelab}
XSIM=${XSIM:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xsim}
TIMEOUT=${G6_L2_FOCUSED_TIMEOUT:-900}
HARD_KB=${G6_L2_FOCUSED_HARD_KB:-8388608}

[[ "$WORK" == /home/lab_726/opencode_tmp/* ]] || {
    printf 'ERROR: workspace must be under /home/lab_726/opencode_tmp\n' >&2
    exit 2
}
for tool in "$VITIS_HLS" "$XVLOG" "$XELAB" "$XSIM" setsid timeout python3; do
    command -v "$tool" >/dev/null 2>&1 || exit 127
done
mkdir -p "$WORK" "$REPORT/logs"
python3 "$ROOT/scripts/gate6_0/generate_l2_focused_cases.py" "$CATALOG"

source_hash=$(python3 - "$ROOT" <<'PY'
import hashlib, sys
from pathlib import Path
root=Path(sys.argv[1]); digest=hashlib.sha256()
paths=sorted((root/"src").glob("*.cpp"))+sorted((root/"include").glob("*.hpp"))
for path in paths:
    if path.name in ("boom_all.cpp", "boom_core_merged.cpp"): continue
    digest.update(str(path.relative_to(root)).encode()+b"\0")
    digest.update(bytes.fromhex(hashlib.sha256(path.read_bytes()).hexdigest()))
print(digest.hexdigest())
PY
)

rtl=$WORK/hls_project/solution/syn/verilog
if [[ ! -s "$rtl/g6_l2_focused_top.v" ]]; then
    log=$REPORT/logs/path_a_csynth.log
    start=$(date +%s)
    setsid timeout --signal=TERM --kill-after=30s "$TIMEOUT" env \
        HLS_BOOM_ROOT="$ROOT" G6_L2_FOCUSED_WORK="$WORK" \
        "$VITIS_HLS" -f "$ROOT/scripts/gate6_0/l2_focused_csynth.tcl" \
        >"$log" 2>&1 &
    pid=$!
    peak_kb=0
    while kill -0 "$pid" 2>/dev/null; do
        current_kb=$(du -sk "$WORK" 2>/dev/null | awk '{print $1+0}')
        ((current_kb > peak_kb)) && peak_kb=$current_kb
        if ((current_kb >= HARD_KB)); then
            kill -TERM -- "-$pid" 2>/dev/null || true
            sleep 5
            kill -KILL -- "-$pid" 2>/dev/null || true
            wait "$pid" || true
            printf 'ERROR: PATH_A workspace exceeded 8 GiB\n' >&2
            exit 1
        fi
        sleep 5
    done
    wait "$pid"
    duration=$(( $(date +%s) - start ))
else
    duration=0
    peak_kb=$(du -sk "$WORK" | awk '{print $1+0}')
fi

sim=$WORK/sim
mkdir -p "$sim"
mapfile -t rtl_files < <(printf '%s\n' "$rtl"/*.v | sort)
cp "$rtl"/*.dat "$sim/" 2>/dev/null || true
(
    cd "$sim"
    "$XVLOG" "${rtl_files[@]}" >"$REPORT/logs/path_a_xvlog_rtl.log" 2>&1
    "$XVLOG" --sv "$ROOT/rtl_tb/gate6_0/g6_l2_focused_tb.sv" \
        >"$REPORT/logs/path_a_xvlog_tb.log" 2>&1
    "$XELAB" g6_l2_focused_tb -s g6_l2_focused_snapshot -timescale 1ns/1ps \
        >"$REPORT/logs/path_a_xelab.log" 2>&1
)

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
printf 'case_id,case_name,path,image,top,status,source_hash,rtl_hash,evidence\n' >"$ACCEPTANCE"

python3 - "$CATALOG" <<'PY' >"$WORK/cases.tsv"
import csv, sys
for row in csv.DictReader(open(sys.argv[1], newline="")):
    vals=[row["case_id"], row["case_name"]]
    meta=int(row["head"]) | (int(row["load_rob"])<<8) | \
         (int(row["load_size"])<<16) | (int(row["load_signed"])<<18)
    vals += [f"{meta:x}", f'{int(row["load_allocation"]):x}',
             f'{int(row["load_address"]):x}']
    for i in range(3):
        vals += [f'{int(row[f"store{i}_{key}"]):x}'
                 for key in ("rob","sq","address","data","mask")]
    vals += [f'{int(row[key]):x}' for key in
             ("expected_decision","expected_selected_sq","expected_value",
              "expected_coverage")]
    print("\t".join(vals))
PY

while IFS=$'\t' read -r id name load_meta load_alloc load_addr \
        s0_rob s0_sq s0_addr s0_data s0_mask \
        s1_rob s1_sq s1_addr s1_data s1_mask \
        s2_rob s2_sq s2_addr s2_data s2_mask \
        exp_decision exp_selected exp_value exp_coverage; do
    log=$REPORT/logs/case_${id}_${name}.log
    (
        cd "$sim"
        timeout 300 "$XSIM" g6_l2_focused_snapshot --runall --onerror quit \
            --testplusarg "CASE_ID=$id" \
            --testplusarg "LOAD_META=$load_meta" \
            --testplusarg "LOAD_ALLOC=$load_alloc" \
            --testplusarg "LOAD_ADDR=$load_addr" \
            --testplusarg "S0_ROB=$s0_rob" --testplusarg "S0_SQ=$s0_sq" \
            --testplusarg "S0_ADDR=$s0_addr" --testplusarg "S0_DATA=$s0_data" \
            --testplusarg "S0_MASK=$s0_mask" \
            --testplusarg "S1_ROB=$s1_rob" --testplusarg "S1_SQ=$s1_sq" \
            --testplusarg "S1_ADDR=$s1_addr" --testplusarg "S1_DATA=$s1_data" \
            --testplusarg "S1_MASK=$s1_mask" \
            --testplusarg "S2_ROB=$s2_rob" --testplusarg "S2_SQ=$s2_sq" \
            --testplusarg "S2_ADDR=$s2_addr" --testplusarg "S2_DATA=$s2_data" \
            --testplusarg "S2_MASK=$s2_mask" \
            --testplusarg "EXP_DECISION=$exp_decision" \
            --testplusarg "EXP_SELECTED=$exp_selected" \
            --testplusarg "EXP_VALUE=$exp_value" \
            --testplusarg "EXP_COVERAGE=$exp_coverage" \
            --log "$log" >"$log.stdout" 2>&1
    )
    grep -q "G6_L2_FOCUSED_CASE_PASS id=$id " "$log"
    printf '%s,%s,PATH_A,forwarding_plan,g6_l2_focused_top,PASS,%s,%s,%s\n' \
        "$id" "$name" "$source_hash" "$rtl_hash" \
        "reports/gate6_0_full_lsu/l2/focused_rtl/logs/case_${id}_${name}.log" \
        >>"$ACCEPTANCE"
done <"$WORK/cases.tsv"

count=$(($(wc -l <"$ACCEPTANCE") - 1))
[[ "$count" -eq 48 ]]
printf 'G6_L2_FOCUSED_RTL_PASS cases=%s duration_seconds=%s peak_workspace_kb=%s rtl_hash=%s\n' \
    "$count" "$duration" "$peak_kb" "$rtl_hash"
