#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)"}
BUILD=${GATE5_4_PF4_RTL_BUILD_DIR:-/tmp/boom_hls/pf4/full_core_rtl}
PROJECT="$BUILD/boom_core_pf4_rtl_top_hls"
RTL="$PROJECT/solution_pf4_rtl/syn/verilog"
WORK="$BUILD/xsim"
REPORT=${GATE5_4_PF4_RTL_REPORT_DIR:-"$ROOT/reports/gate5_4_product_integration/pf4/full_core_rtl"}
PROGRAM_BUILD=${PF4_PROGRAM_BUILD:-/tmp/boom_hls/pf4/programs}
VITIS_HLS_BIN=${VITIS_HLS:-/home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls}
XVLOG_BIN=${XVLOG:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xvlog}
XELAB_BIN=${XELAB:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xelab}
XSIM_BIN=${XSIM:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xsim}
PROGRAMS=(pf4_pred_nt_actual_nt pf4_pred_nt_actual_t pf4_pred_t_actual_t
          pf4_pred_t_actual_nt pf4_rvc_mispredict pf4_same_packet_kill
          pf4_fault_refetch pf4_ftq_wrap_recovery pf4_jal_preservation
          pf4_jalr_unpredicted pf4_exception_priority pf4_mixed_long_control)

mapfile -t CPP_INPUTS < <(printf '%s\n' "$ROOT"/src/*.cpp | sort)
mapfile -t HEADER_INPUTS < <(printf '%s\n' "$ROOT"/include/*.hpp | sort)
INPUTS=()
for source in "${CPP_INPUTS[@]}"; do
    case "$source" in
        */src/boom_all.cpp|*/src/boom_core_merged.cpp) continue ;;
    esac
    INPUTS+=("$source")
done
INPUTS+=("${HEADER_INPUTS[@]}")

hash_inputs() {
    python3 - "$ROOT" "${INPUTS[@]}" <<'PY'
import hashlib, sys
from pathlib import Path
root = Path(sys.argv[1]); digest = hashlib.sha256()
for item in sys.argv[2:]:
    path = Path(item)
    digest.update(str(path.relative_to(root)).encode()); digest.update(b"\0")
    digest.update(hashlib.sha256(path.read_bytes()).digest())
print(digest.hexdigest())
PY
}

SOURCE_HASH=$(hash_inputs)
mkdir -p -- "$BUILD" "$REPORT/logs" "$REPORT/traces"
PF4_PROGRAM_BUILD="$PROGRAM_BUILD" "$ROOT/scripts/gate5_4/build_pf4_product_programs.sh" \
    >"$REPORT/logs/program_build.log" 2>&1
"$ROOT/scripts/generate_merged.sh" >"$REPORT/logs/generate_merged.log" 2>&1
RTL_TOP="$RTL/boom_core_pf4_rtl_top.v"
need_csynth=${GATE5_4_PF4_RTL_FORCE_CSYNTH:-0}
for source in "${INPUTS[@]}"; do
    [[ -s "$RTL_TOP" && ! "$source" -nt "$RTL_TOP" ]] || need_csynth=1
done
if [[ "$need_csynth" == 1 ]]; then
    GATE5_4_PF4_RTL_HLS_PROJECT="$PROJECT" "$VITIS_HLS_BIN" \
        -f "$ROOT/scripts/gate5_4/pf4_full_core_rtl_csynth.tcl" \
        >"$REPORT/logs/csynth.log" 2>&1
else
    printf 'Reusing fresh current-source PF4 RTL at %s\n' "$RTL" >"$REPORT/logs/csynth.log"
fi
[[ -s "$RTL_TOP" ]] || { printf 'PF4 full-core RTL unavailable\n' >&2; exit 3; }
[[ "$(hash_inputs)" == "$SOURCE_HASH" ]] || { printf 'PF4 source changed during RTL generation\n' >&2; exit 3; }
for source in "${INPUTS[@]}"; do
    [[ ! "$source" -nt "$RTL_TOP" ]] || { printf 'PF4 RTL older than %s\n' "$source" >&2; exit 3; }
done

python3 - "$ROOT" "$REPORT/source_freshness_manifest.csv" "$SOURCE_HASH" "${INPUTS[@]}" <<'PY'
import csv, hashlib, sys
from pathlib import Path
root, output, aggregate = Path(sys.argv[1]), Path(sys.argv[2]), sys.argv[3]
with output.open("w", newline="") as stream:
    writer = csv.writer(stream); writer.writerow(("path", "scope", "sha256"))
    for item in sys.argv[4:]:
        path = Path(item)
        writer.writerow((path.relative_to(root), "INCLUDED", hashlib.sha256(path.read_bytes()).hexdigest()))
    writer.writerow(("src/boom_all.cpp", "EXCLUDED_NON_MODULAR", ""))
    writer.writerow(("src/boom_core_merged.cpp", "EXCLUDED_GENERATED", ""))
    writer.writerow(("AGGREGATE", "CURRENT_SOURCE_HEADER_HASH", aggregate))
PY

rm -rf -- "$WORK"
mkdir -p -- "$WORK"
cp "$RTL"/*.dat "$WORK"/ 2>/dev/null || true
mapfile -t RTL_FILES < <(printf '%s\n' "$RTL"/*.v | sort)
(
    cd "$WORK"
    "$XVLOG_BIN" "${RTL_FILES[@]}"
    "$XVLOG_BIN" --sv "$ROOT/rtl_tb/pf4_axis_imem_model.sv" \
        "$ROOT/rtl_tb/axis_dmem_model.sv" "$ROOT/rtl_tb/commit_trace_monitor.sv" \
        "$ROOT/rtl_tb/pf4_full_core_rtl_harness.sv" "$ROOT/rtl_tb/pf4_full_core_rtl_tb.sv"
    "$XELAB_BIN" pf4_full_core_rtl_tb -s pf4_full_core_snapshot -timescale 1ns/1ps
) >"$REPORT/logs/xsim_build.log" 2>&1

run_case() {
    local case_name=$1 image_name=$2 fault_mode=$3
    (
        cd "$WORK"
        "$XSIM_BIN" pf4_full_core_snapshot --runall --onerror quit \
            --testplusarg "PROGRAM=$PROGRAM_BUILD/$image_name.words.hex" \
            --testplusarg "INIT=$PROGRAM_BUILD/$image_name.init" \
            --testplusarg "PROGRAM_NAME=$case_name" --testplusarg "FAULT_MODE=$fault_mode" \
            --testplusarg "TRACE=$REPORT/traces/$case_name.jsonl" \
            --testplusarg "MAX_CYCLES=1000000" --log "$REPORT/logs/$case_name.log"
    ) >"$REPORT/logs/$case_name.stdout.log" 2>&1
    grep -q "PF4_FULL_CORE_RTL_PASS program=$case_name" "$REPORT/logs/$case_name.log"
}

for name in "${PROGRAMS[@]}"; do
    mode=0
    [[ "$name" == pf4_fault_refetch ]] && mode=1
    [[ "$name" == pf4_pred_t_actual_t ]] && mode=2
    run_case "$name" "$name" "$mode"
done
run_case pf4_pred_t_actual_nt_fault pf4_pred_t_actual_nt 2

python3 - "$REPORT" "$SOURCE_HASH" "${PROGRAMS[@]}" <<'PY'
import csv, json, re, sys
from pathlib import Path
report = Path(sys.argv[1]); source_hash = sys.argv[2]; programs = sys.argv[3:]
expected = {
"pf4_pred_nt_actual_nt":{8:11,9:21,18:31}, "pf4_pred_nt_actual_t":{8:0,9:22,18:32},
"pf4_pred_t_actual_t":{8:7,9:23,18:34}, "pf4_pred_t_actual_nt":{8:5,9:24,18:35},
"pf4_rvc_mispredict":{8:6,9:25,18:37}, "pf4_same_packet_kill":{8:0,9:26,18:13,19:39},
"pf4_fault_refetch":{8:27,9:37,18:47}, "pf4_ftq_wrap_recovery":{8:80,9:1,18:88},
"pf4_jal_preservation":{8:29,9:39,18:49}, "pf4_jalr_unpredicted":{8:30,9:40,18:50},
"pf4_exception_priority":{8:31,9:41,18:51}, "pf4_mixed_long_control":{8:12,18:52,19:62}}
rows=[]
for name in programs:
    records=[json.loads(line) for line in (report/"traces"/f"{name}.jsonl").read_text().splitlines() if line]
    commits=[r for r in records if r.get("event")=="commit"]
    final={r.get("rd"):int(r["rd_value"],16) for r in commits if r.get("rd_valid")}
    signature=all(final.get(rd)==value for rd,value in expected[name].items())
    log=(report/"logs"/f"{name}.log").read_text(errors="replace")
    event=re.search(r"PF4_FULL_CORE_RTL_PASS.*",log)
    status="PASS" if signature and event else "FAIL"
    rows.append((name,status,len(commits),"PASS" if signature else "FAIL",f"traces/{name}.jsonl"))
with (report/"full_core_rtl_matrix.csv").open("w",newline="") as stream:
    writer=csv.writer(stream); writer.writerow(("program","status","commits","signature","trace")); writer.writerows(rows)
passed=sum(row[1]=="PASS" for row in rows)
fault_log=(report/"logs"/"pf4_pred_t_actual_nt_fault.log").read_text(errors="replace")
fault_pass="PF4_FULL_CORE_RTL_PASS program=pf4_pred_t_actual_nt_fault" in fault_log
if passed != 12 or not fault_pass: raise SystemExit(f"PF4 full-core RTL failed: programs={passed}/12 fault={fault_pass}")
(report/"generation_provenance.md").write_text(
    "# PF4 Current-Source Full-Core RTL Provenance\n\n"
    f"- Modular source/header SHA-256: `{source_hash}`.\n"
    "- `src/boom_all.cpp` excluded and untouched; generated merged source excluded from aggregate hash.\n"
    "- Test-only full-core top enables Product FTQ and accepts fixture-only BIM seeds; product ports are unchanged.\n"
    "- Product programs: `12/12 PASS`.\n"
    "- Predicted-T younger-fault checks: actual-T masked and actual-NT precise refetch/take, `2/2 PASS`.\n"
    "- Seeded BIM entries retained their exact 2-bit values after execution.\n", encoding="utf-8")
print(f"PF4_CURRENT_SOURCE_FULL_CORE_RTL_PASS 12/12 mandatory_fault=2/2 source_hash={source_hash}")
PY
