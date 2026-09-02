#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)"}
: "${BOOM_BUILD_ROOT:=/tmp/boom_hls}"
export BOOM_BUILD_ROOT
# shellcheck source=../common/gate_workspace.sh
source "$ROOT/scripts/common/gate_workspace.sh"

gate_begin gate5_4_ftq f1_native
gate_build_dir tests >/dev/null
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
CXX=${CXX:-g++}
CXXFLAGS=(-std=c++11 -O2 -Wall -Wextra -Werror -Wno-unknown-pragmas
          -I"$ROOT/include")

"$CXX" "${CXXFLAGS[@]}" \
    "$ROOT/tb/differential/ftq_tests.cpp" "$ROOT/src/ftq.cpp" \
    -o "$BUILD/ftq_tests" 2>"$BUILD/ftq_directed_compile.log"
"$BUILD/ftq_tests" | tee "$BUILD/ftq_directed.log"
grep -Eq 'FTQ_SUMMARY .*failures=0' "$BUILD/ftq_directed.log"

"$CXX" "${CXXFLAGS[@]}" \
    "$ROOT/tb/differential/predictor_ftq_composition_tests.cpp" \
    "$ROOT/src/ftq.cpp" "$ROOT/src/predictor.cpp" "$ROOT/src/predecode.cpp" \
    -o "$BUILD/predictor_ftq_composition_tests" \
    2>"$BUILD/ftq_composition_compile.log"
"$BUILD/predictor_ftq_composition_tests" | tee "$BUILD/ftq_composition.log"
grep -Eq 'PREDICTOR_FTQ_COMPOSITION,.*failures=0' \
    "$BUILD/ftq_composition.log"

mkdir -p -- "$REPORT/logs"
cp -- "$BUILD/ftq_directed.log" "$REPORT/logs/ftq_directed.log"
cp -- "$BUILD/ftq_composition.log" "$REPORT/logs/ftq_composition.log"
printf '%s\n' GATE5_4_F1_FTQ_NATIVE_PASS
