#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)"}
RTL=${G6_L2_RTL_DIR:-/home/lab_726/opencode_tmp/g6_l2/full_core/8_8/hls_project/solution/syn/verilog}
BUILD=${G6_L2_RVC_RTL_BUILD:-/home/lab_726/opencode_tmp/g6_l2/preservation/rvc/rtl}
REPORT=${G6_L2_RVC_RTL_REPORT:-$ROOT/reports/gate6_0_full_lsu/l2/preservation/rvc/rtl}
XVLOG=${XVLOG:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xvlog}
XELAB=${XELAB:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xelab}
XSIM=${XSIM:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xsim}
PROGRAMS=(rvc_addi rvc_load_store rvc_branch rvc_jump rvc_word_ops rvc_mixed_16_32 rvc_cross_boundary rvc_rv64m_mix rvc_redirect_halfword rvc_tohost rvc_decode_gaps)

mkdir -p "$BUILD" "$REPORT/logs" "$REPORT/traces"
cp "$RTL"/*.dat "$BUILD/" 2>/dev/null || true
mapfile -t rtl_files < <(printf '%s\n' "$RTL"/*.v | sort)
(
    cd "$BUILD"
    "$XVLOG" "${rtl_files[@]}"
    "$XVLOG" --sv "$ROOT/rtl_tb/axis_imem_model.sv" \
        "$ROOT/rtl_tb/axis_dmem_model.sv" "$ROOT/rtl_tb/commit_trace_monitor.sv" \
        "$ROOT/tb/differential/gate5_2_r2_rtl_harness.sv" \
        "$ROOT/rtl_tb/boom_core_rtl_tb.sv"
    "$XELAB" boom_core_rtl_tb -s g6_l2_rvc_snapshot -timescale 1ns/1ps
) >"$REPORT/logs/xsim_build.log" 2>&1

for name in "${PROGRAMS[@]}"; do
    (
        cd "$BUILD"
        "$XSIM" g6_l2_rvc_snapshot --runall --onerror quit \
            --testplusarg "PROGRAM=$ROOT/tb/programs/rvc_fetch/build/$name.hex" \
            --testplusarg "PROGRAM_NAME=$name" \
            --testplusarg "SCENARIO=R0_POWER_ON_RESET" \
            --testplusarg "MAX_CYCLES=1000000" \
            --testplusarg "TRACE=$REPORT/traces/$name.jsonl" \
            --log "$REPORT/logs/$name.log"
    ) >"$REPORT/logs/$name.stdout.log" 2>&1
    grep -q 'GATE3_8_PASS scenario=R0_POWER_ON_RESET' "$REPORT/logs/$name.log"
done
printf 'G6_L2_RVC_RTL_11_11=PASS\n'
