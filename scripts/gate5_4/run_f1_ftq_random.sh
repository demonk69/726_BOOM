#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)"}
: "${BOOM_BUILD_ROOT:=/tmp/boom_hls}"
export BOOM_BUILD_ROOT
# shellcheck source=../common/gate_workspace.sh
source "$ROOT/scripts/common/gate_workspace.sh"

gate_begin gate5_4_ftq f1_random
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
"$CXX" -std=c++11 -O2 -Wall -Wextra -Werror -Wno-unknown-pragmas \
    -I"$ROOT/include" "$ROOT/tb/differential/ftq_random_tests.cpp" \
    "$ROOT/src/ftq.cpp" -o "$BUILD/ftq_random_tests" \
    2>"$BUILD/ftq_random_compile.log"
"$BUILD/ftq_random_tests" | tee "$BUILD/ftq_random.log"
grep -q '^FTQ_RANDOM_PASS ' "$BUILD/ftq_random.log"

mkdir -p -- "$REPORT/logs"
cp -- "$BUILD/ftq_random.log" "$REPORT/logs/ftq_random.log"
printf '%s\n' GATE5_4_F1_FTQ_RANDOM_PASS
