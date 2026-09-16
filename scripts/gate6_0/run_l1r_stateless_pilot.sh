#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)"}
WORK=${G6_L1R_PILOT_WORK:-/home/lab_726/opencode_tmp/g6_l1_focused_stateless_pilot}
REPORT=${G6_L1R_PILOT_REPORT:-$ROOT/reports/gate6_0_full_lsu/l1/focused_rtl_redesign}
VITIS_HLS=${VITIS_HLS:-/home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls}
XVLOG=${XVLOG:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xvlog}
XELAB=${XELAB:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xelab}
XSIM=${XSIM:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xsim}
IMAGE_TIMEOUT=${G6_L1R_PILOT_TIMEOUT:-900}
START_IMAGE=${G6_L1R_PILOT_START_IMAGE:-2}
SOFT_KB=${G6_L1R_PILOT_SOFT_KB:-4194304}
HARD_KB=${G6_L1R_PILOT_HARD_KB:-8388608}
MATRIX=$REPORT/pilot_results.csv
RESOURCE=$REPORT/pilot_resources.csv
TB=$ROOT/rtl_tb/gate6_0/g6_l1r_stateless_pilot_tb.sv

[[ "$WORK" == /home/lab_726/opencode_tmp/* ]] || {
    printf 'ERROR: pilot workspace must remain under /home/lab_726/opencode_tmp\n' >&2
    exit 2
}
for tool in "$VITIS_HLS" "$XVLOG" "$XELAB" "$XSIM" setsid timeout /usr/bin/time; do
    command -v "$tool" >/dev/null 2>&1 || { printf 'ERROR: missing tool %s\n' "$tool" >&2; exit 127; }
done

mkdir -p -- "$WORK" "$REPORT/logs"
if ((START_IMAGE == 2)); then
    printf 'image,top,lq_depth,sq_depth,status,rtl_sentinel,evidence\n' >"$MATRIX"
    printf 'image,timeout_seconds,duration_seconds,peak_workspace_kb,peak_rss_kb,soft_limit_crossed,hard_limit_crossed,exit_code\n' >"$RESOURCE"
    printf '1,boom_core_top,4,16,REUSED_PASS,ASYMMETRIC_RESET_RTL_STATUS=PASS,%s\n' \
        "$ROOT/reports/gate6_0_full_lsu/l1/asymmetric_reset_rtl_matrix.csv" >>"$MATRIX"
    printf 'reset_wrapper_rejected,900,135,137272,8821180,1,1,143\n' >>"$RESOURCE"
    printf 'older_store_composite_rejected,900,141,8896420,1956928,1,1,143\n' >>"$RESOURCE"
else
    [[ -s "$MATRIX" && -s "$RESOURCE" ]] || {
        printf 'ERROR: resume requires existing pilot result files\n' >&2
        exit 2
    }
fi

declare -a NAMES=(lq_reuse stale_response branch_squash sq_identity older_store_block)
declare -a TOPS=(g6_l1r_pilot_lq_reuse g6_l1r_pilot_stale_response g6_l1r_pilot_branch_squash g6_l1r_pilot_sq_identity g6_l1r_pilot_older_store_block)
declare -a DEFINES=(PILOT_LQ_REUSE PILOT_STALE_RESPONSE PILOT_BRANCH_SQUASH PILOT_SQ_IDENTITY PILOT_OLDER_STORE)

run_hls() {
    local image=$1 name=$2 top=$3 lq=$4 sq=$5 image_work=$6 log=$7
    local start end pid rc current_kb rss_kb peak_kb=0 peak_rss=0 soft=0 hard=0
    start=$(date +%s)
    local wrapper=tb/differential/g6_l1r_stateless_pilot_tops.cpp product=merged
    if ((image == 6)); then
        wrapper=tb/differential/g6_l1r_stateless_older_store_top.cpp
        product=lsu_translation_unit
    fi
    setsid timeout --signal=TERM --kill-after=30s "$IMAGE_TIMEOUT" /usr/bin/time -v \
        -o "$REPORT/logs/${name}_time.log" env HLS_BOOM_ROOT="$ROOT" \
        G6_L1R_PILOT_WORK="$image_work" G6_L1R_PILOT_TOP="$top" \
        G6_L1R_PILOT_WRAPPER="$wrapper" G6_L1R_PILOT_PRODUCT="$product" \
        LQ_DEPTH="$lq" SQ_DEPTH="$sq" FPGA_PART=xczu7ev-ffvc1156-2-e \
        CLOCK_PERIOD=10 "$VITIS_HLS" -f "$ROOT/scripts/gate6_0/l1r_stateless_pilot_csynth.tcl" \
        >"$log" 2>&1 &
    pid=$!
    while kill -0 "$pid" 2>/dev/null; do
        current_kb=$(du -sk -- "$image_work" 2>/dev/null | awk '{print $1}')
        rss_kb=$(ps -eo pgid=,rss= | awk -v group="$pid" '$1 == group {sum += $2} END {print sum + 0}')
        ((current_kb > peak_kb)) && peak_kb=$current_kb
        ((rss_kb > peak_rss)) && peak_rss=$rss_kb
        if ((current_kb >= SOFT_KB || rss_kb >= SOFT_KB)); then soft=1; fi
        if ((current_kb >= HARD_KB || rss_kb >= HARD_KB)); then
            hard=1
            kill -TERM -- "-$pid" 2>/dev/null || true
            sleep 5
            kill -KILL -- "-$pid" 2>/dev/null || true
            break
        fi
        sleep 5
    done
    set +e
    wait "$pid"
    rc=$?
    set -e
    end=$(date +%s)
    current_kb=$(du -sk -- "$image_work" 2>/dev/null | awk '{print $1}')
    ((current_kb > peak_kb)) && peak_kb=$current_kb
    if [[ -s "$REPORT/logs/${name}_time.log" ]]; then
        rss_kb=$(awk -F: '/Maximum resident set size/ {gsub(/^[[:space:]]+/, "", $2); print $2}' "$REPORT/logs/${name}_time.log")
        rss_kb=${rss_kb:-0}
        ((rss_kb > peak_rss)) && peak_rss=$rss_kb
    fi
    printf '%s,%s,%s,%s,%s,%s,%s,%s\n' "$image" "$IMAGE_TIMEOUT" "$((end-start))" \
        "$peak_kb" "$peak_rss" "$soft" "$hard" "$rc" >>"$RESOURCE"
    return "$rc"
}

for index in "${!NAMES[@]}"; do
    image=$((index + 2))
    ((image < START_IMAGE)) && continue
    name=${NAMES[$index]}
    top=${TOPS[$index]}
    define=${DEFINES[$index]}
    lq=8
    sq=8
    image_work=$WORK/image_${image}_${name}
    sim=$image_work/sim
    rm -rf -- "$image_work"
    mkdir -p -- "$sim"
    hls_log=$REPORT/logs/${name}_csynth.log
    if ! run_hls "$image" "$name" "$top" "$lq" "$sq" "$image_work" "$hls_log"; then
        printf '%s,%s,%s,%s,FAIL,,%s\n' "$image" "$top" "$lq" "$sq" "$hls_log" >>"$MATRIX"
        printf 'ERROR: stateless pilot HLS failed: image=%s top=%s\n' "$image" "$top" >&2
        exit 1
    fi
    rtl=$image_work/hls_project/solution/syn/verilog
    [[ -s "$rtl/$top.v" ]] || { printf 'ERROR: missing RTL for %s\n' "$top" >&2; exit 1; }
    mapfile -t rtl_files < <(printf '%s\n' "$rtl"/*.v | sort)
    cp -- "$rtl"/*.dat "$sim/" 2>/dev/null || true
    (
        cd -- "$sim"
        timeout 180 "$XVLOG" "${rtl_files[@]}" >"$REPORT/logs/${name}_xvlog_rtl.log" 2>&1
        timeout 180 "$XVLOG" --sv -d "DUT_TOP=$top" -d "PILOT_IMAGE=$image" -d "$define" "$TB" \
            >"$REPORT/logs/${name}_xvlog_tb.log" 2>&1
        timeout 180 "$XELAB" g6_l1r_stateless_pilot_tb -s "${name}_snapshot" -timescale 1ns/1ps \
            >"$REPORT/logs/${name}_xelab.log" 2>&1
        timeout 300 "$XSIM" "${name}_snapshot" --runall --onerror quit \
            --log "$REPORT/logs/${name}_xsim.log" >"$REPORT/logs/${name}_xsim.stdout.log" 2>&1
    )
    sentinel="G6_L1R_STATELESS_PILOT_PASS image=$image"
    grep -Fq "$sentinel" "$REPORT/logs/${name}_xsim.log"
    printf '%s,%s,%s,%s,PASS,%s,%s\n' "$image" "$top" "$lq" "$sq" \
        "$sentinel" "$REPORT/logs/${name}_xsim.log" >>"$MATRIX"
done

printf 'G6_L1R_STATELESS_PILOT_PASS new_images=5 reused_path_b=1 scenarios=6\n'
