#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)"}
: "${BOOM_BUILD_ROOT:=/tmp/boom_hls}"
export BOOM_BUILD_ROOT
# shellcheck source=../common/gate_workspace.sh
source "$ROOT/scripts/common/gate_workspace.sh"

gate_begin gate5_4_ftq f1_rtl
gate_build_dir canonical_lutram >/dev/null
BUILD=$BOOM_BUILD_DIR
on_exit() {
    local rc=$?
    trap - EXIT
    if ((rc == 0)); then
        gate_cleanup_success "$BUILD"
    else
        gate_preserve_failure
    fi
    exit "$rc"
}
trap on_exit EXIT

REPORT="$ROOT/reports/gate5_4_ftq/f1"
SOURCE="$BUILD/source"
SIM="$BUILD/sim"
TAG=gate5_4_ftq_f1_d32_lutram
TOP=synth_ftq_32_top
XVLOG=${XVLOG:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xvlog}
XELAB=${XELAB:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xelab}
XSIM=${XSIM:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xsim}

mkdir -p -- "$SOURCE/scripts" "$SIM"
cp -a -- "$ROOT/include" "$ROOT/src" "$ROOT/directives" "$SOURCE/"
cp -p -- "$ROOT/scripts/run_module_csynth.sh" \
    "$ROOT/scripts/module_csynth.tcl" "$ROOT/scripts/create_project.tcl" \
    "$ROOT/scripts/generate_merged.sh" "$SOURCE/scripts/"

RTL="$SOURCE/boom_hls_${TAG}_${TOP}/solution_module/syn/verilog"
if [[ ! -f "$RTL/$TOP.v" ]]; then
    (
        cd -- "$SOURCE"
        HLS_BOOM_ROOT="$SOURCE" BOOM_HLS_GATE="$TAG" \
            BOOM_HLS_CFLAGS_EXTRA=-DBOOM_FTQ_STORAGE_LUTRAM \
            "$SOURCE/scripts/run_module_csynth.sh" "$TOP"
    ) >"$BUILD/csynth_runner.log" 2>&1
fi
[[ -f "$RTL/$TOP.v" ]] || { printf 'missing generated RTL: %s\n' "$RTL/$TOP.v" >&2; exit 1; }

shopt -s nullglob
RTL_FILES=("$RTL"/*.v)
DATA_FILES=("$RTL"/*.dat)
((${#RTL_FILES[@]} > 0)) || { printf 'no generated Verilog in %s\n' "$RTL" >&2; exit 1; }
((${#DATA_FILES[@]} == 0)) || cp -- "${DATA_FILES[@]}" "$SIM/"
(
    cd -- "$SIM"
    "$XVLOG" --sv "$ROOT/rtl_tb/ftq_rtl_tb.sv" "${RTL_FILES[@]}" \
        >"$BUILD/xvlog.log" 2>&1
    "$XELAB" ftq_rtl_tb -s ftq_rtl_sim >"$BUILD/xelab.log" 2>&1
    "$XSIM" ftq_rtl_sim -R >"$BUILD/xsim.log" 2>&1
)
grep -q 'GATE5_4_F1_FTQ_RTL_PASS' "$BUILD/xsim.log"

mkdir -p -- "$REPORT/logs/rtl"
cp -- "$BUILD/xsim.log" "$REPORT/logs/rtl/xsim.log"
printf '%s\n' GATE5_4_F1_FTQ_RTL_PASS
