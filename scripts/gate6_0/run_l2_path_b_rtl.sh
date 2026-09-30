#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)"}
RTL=${G6_L2_RTL:-/home/lab_726/opencode_tmp/g6_l2/full_core/8_8/hls_project/solution/syn/verilog}
WORK=${G6_L2_PATH_B_WORK:-/home/lab_726/opencode_tmp/g6_l2/path_b/xsim}
REPORT=${G6_L2_REPORT:-$ROOT/reports/gate6_0_full_lsu/l2}/path_b
XVLOG=${XVLOG:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xvlog}
XELAB=${XELAB:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xelab}
XSIM=${XSIM:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xsim}

[[ "$WORK" == /home/lab_726/opencode_tmp/g6_l2/* ]] || {
    printf 'ERROR: PATH_B workspace must remain under /home/lab_726/opencode_tmp/g6_l2\n' >&2
    exit 2
}
[[ -s "$RTL/boom_core_top.v" ]] || { printf 'ERROR: canonical L2 RTL is missing\n' >&2; exit 2; }
mkdir -p -- "$WORK" "$REPORT/logs"
rm -rf -- "$WORK/xsim.dir"
cp -- "$RTL"/*.dat "$WORK/" 2>/dev/null || true
mapfile -t rtl_files < <(printf '%s\n' "$RTL"/*.v | sort)
(
    cd -- "$WORK"
    timeout 300 "$XVLOG" "${rtl_files[@]}"
    timeout 180 "$XVLOG" --sv "$ROOT/rtl_tb/gate6_0/g6_l2_path_b_tb.sv"
    timeout 300 "$XELAB" g6_l2_path_b_tb -s g6_l2_path_b_snapshot -timescale 1ns/1ps
) >"$REPORT/logs/build.log" 2>&1
(
    cd -- "$WORK"
    timeout 1200 "$XSIM" g6_l2_path_b_snapshot --runall --onerror quit \
        --log "$REPORT/logs/simulation.log"
) >"$REPORT/logs/simulation.stdout.log" 2>&1
grep -q 'G6_L2_PATH_B_PASS cases=6' "$REPORT/logs/simulation.log"
printf 'G6_L2_PATH_B_RTL=PASS cases=6\n'
