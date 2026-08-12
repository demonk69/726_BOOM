#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
BASELINE=/tmp/opencode/boom_gate41_baseline
REPORT="$ROOT/reports/gate5_1_frontend/repair/r1"
BUILD="$ROOT/build/gate5_1_frontend/repair/r1"
CXX_BIN=${CXX:-g++}

[[ "$(git -C "$BASELINE" rev-parse HEAD)" == c67ce8bf8e3d81a620dd923537b1cb1f485fdd01 ]] || {
  printf '%s\n' 'ERROR: accepted baseline worktree is not c67ce8b' >&2
  exit 2
}

mkdir -p "$REPORT" "$BUILD/current" "$BUILD/baseline"

COMMON_SRCS=(
  src/boom_core_step.cpp src/frontend.cpp src/rvc.cpp src/decode.cpp src/rename.cpp src/rob.cpp
  src/issue.cpp src/completion.cpp src/mul.cpp src/divider.cpp src/execute.cpp
  src/branch.cpp src/lsu.cpp src/commit.cpp src/csr.cpp src/reset.cpp
)

compile_trace() {
  local tree=$1
  local output=$2
  local mode=${3:-current}
  local sources=()
  local source
  for source in "${COMMON_SRCS[@]}"; do sources+=("$tree/$source"); done
  local flags=()
  [[ "$mode" == baseline ]] && flags+=(-DGATE5_1_R1_LEGACY_FRONTEND)
  "$CXX_BIN" -std=c++11 -O0 -g -I"$tree/include" "${flags[@]}" \
    "${sources[@]}" "$ROOT/tb/differential/gate5_1_r1_rob_fill_trace.cpp" -o "$output"
}

compile_trace "$ROOT" "$BUILD/current/r1_trace" current
compile_trace "$BASELINE" "$BUILD/baseline/r1_trace" baseline

"$BUILD/current/r1_trace" "$REPORT/rob_fill_cycle_trace.csv" "$REPORT/frontend_cycle_trace.csv"
"$BUILD/baseline/r1_trace" "$REPORT/baseline_rob_fill_cycle_trace.csv" \
  "$REPORT/baseline_frontend_cycle_trace.csv"

printf '%s\n' 'R1 current/baseline native traces generated.'
