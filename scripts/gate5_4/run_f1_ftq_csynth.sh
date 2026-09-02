#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)"}
: "${BOOM_BUILD_ROOT:=/tmp/boom_hls}"
export BOOM_BUILD_ROOT
# shellcheck source=../common/gate_workspace.sh
source "$ROOT/scripts/common/gate_workspace.sh"

gate_begin gate5_4_ftq f1_csynth
gate_build_dir sweep >/dev/null
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

REPORT="$ROOT/reports/gate5_4_ftq/f1/csynth"
SOURCE="$BUILD/source"
ACCEPTED="$BUILD/accepted"
mkdir -p -- "$SOURCE/scripts" "$ACCEPTED"
cp -a -- "$ROOT/include" "$ROOT/src" "$ROOT/directives" "$SOURCE/"
cp -p -- "$ROOT/scripts/run_module_csynth.sh" \
    "$ROOT/scripts/module_csynth.tcl" "$ROOT/scripts/create_project.tcl" \
    "$ROOT/scripts/generate_merged.sh" "$SOURCE/scripts/"

run_variant() {
    local name=$1 depth=$2 storage=$3 reset=$4 top=$5 flags=${6:-}
    local tag="gate5_4_ftq_f1_${name}"
    local project="$SOURCE/boom_hls_${tag}_${top}"
    (
        cd -- "$SOURCE"
        HLS_BOOM_ROOT="$SOURCE" BOOM_HLS_GATE="$tag" \
            BOOM_HLS_CFLAGS_EXTRA="$flags" \
            "$SOURCE/scripts/run_module_csynth.sh" "$top"
    ) >"$BUILD/${name}.runner.log" 2>&1
    local report="$project/solution_module/syn/report/${top}_csynth.rpt"
    [[ -f "$report" ]] || { printf 'missing csynth report: %s\n' "$report" >&2; return 1; }
    cp -- "$report" "$ACCEPTED/${name}.rpt"
    printf '%s,%s,%s,%s,%s,%s\n' \
        "$name" "$depth" "$storage" "$reset" "$top" "${name}.rpt" \
        >>"$ACCEPTED/summary.csv"
}

printf 'variant,depth,storage,reset,top,report\n' >"$ACCEPTED/summary.csv"
run_variant d8_auto 8 AUTO control synth_ftq_8_top
run_variant d16_auto 16 AUTO control synth_ftq_16_top
run_variant d32_auto 32 AUTO control synth_ftq_32_top
run_variant d64_auto 64 AUTO control synth_ftq_64_top
run_variant d32_lutram 32 LUTRAM control synth_ftq_32_top \
    -DBOOM_FTQ_STORAGE_LUTRAM
run_variant d32_bram 32 BRAM control synth_ftq_32_top \
    -DBOOM_FTQ_STORAGE_BRAM
run_variant d32_full_reset 32 AUTO full synth_ftq_32_full_reset_top

mkdir -p -- "$REPORT"
cp -- "$ACCEPTED"/*.rpt "$REPORT/"
cp -- "$ACCEPTED/summary.csv" "$REPORT/summary.csv"
printf '%s\n' 'GATE5_4_F1_FTQ_CSYNTH_PASS variants=7'
