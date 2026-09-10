#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
BUILD_ROOT=${BOOM_BUILD_ROOT:-/tmp/boom_hls/pf3a}
RTL=${PF3_PRODUCT_RTL_DIR:?set PF3_PRODUCT_RTL_DIR to current canonical Verilog directory}
PROGRAM_BUILD=${PF3_PROGRAM_BUILD:-$BUILD_ROOT/programs}
WORK="$BUILD_ROOT/product_rtl"
LOGS="$WORK/logs"
TRACES="$WORK/traces"
PROGRAMS=(pf3_straight_commit pf3_rvc_packets pf3_jal_mask pf3_conditional_shadow
          pf3_branch_squash pf3_exception_flush pf3_ftq_wrap pf3_generation_reuse
          pf3_rv64m pf3_mixed_control pf3_long_stream pf3_reset_midstream)
if [[ $# -gt 0 ]]; then PROGRAMS=("$@"); fi
mkdir -p "$WORK" "$LOGS" "$TRACES"
PF3_PROGRAM_BUILD="$PROGRAM_BUILD" bash "$ROOT/scripts/gate5_4/build_pf3_product_programs.sh" >"$LOGS/program_build.log"
mapfile -t RTL_FILES < <(printf '%s\n' "$RTL"/*.v | sort)
EXTRA_RTL=()
if [[ -n "${PF3_PRODUCT_RTL_COMPAT:-}" ]]; then EXTRA_RTL+=("$PF3_PRODUCT_RTL_COMPAT"); fi
cp "$RTL"/*.dat "$WORK"/ 2>/dev/null || true
(
  cd "$WORK"
  /home/lab_726/Xilinx/Vivado/2021.2/bin/xvlog "${RTL_FILES[@]}"
  /home/lab_726/Xilinx/Vivado/2021.2/bin/xvlog --sv \
    "$ROOT/rtl_tb/axis_imem_model.sv" "$ROOT/rtl_tb/axis_dmem_model.sv" \
    "$ROOT/rtl_tb/commit_trace_monitor.sv" "${EXTRA_RTL[@]}" \
    "$ROOT/rtl_tb/pf3_product_rtl_harness.sv" \
    "$ROOT/rtl_tb/pf3_product_rtl_tb.sv"
  /home/lab_726/Xilinx/Vivado/2021.2/bin/xelab pf3_product_rtl_tb \
    -s pf3_product_snapshot -timescale 1ns/1ps
) >"$LOGS/build.log" 2>&1
for name in "${PROGRAMS[@]}"; do
  trap=0; reset=0
  [[ "$name" == pf3_exception_flush ]] && trap=1
  [[ "$name" == pf3_reset_midstream ]] && reset=1
  (
    cd "$WORK"
    /home/lab_726/Xilinx/Vivado/2021.2/bin/xsim pf3_product_snapshot --runall --onerror quit \
      --testplusarg "PROGRAM=$PROGRAM_BUILD/$name.hex" --testplusarg "PROGRAM_NAME=$name" \
      --testplusarg "EXPECT_TRAP=$trap" --testplusarg "RESET_MIDSTREAM=$reset" \
      --testplusarg "TRACE=$TRACES/$name.jsonl" --log "$LOGS/$name.log"
  ) >"$LOGS/$name.stdout.log" 2>&1
  grep -q "PF3_PRODUCT_RTL_PASS program=$name" "$LOGS/$name.log"
done
printf 'PF3_PRODUCT_RTL_PASS cases=%u\n' "${#PROGRAMS[@]}"
