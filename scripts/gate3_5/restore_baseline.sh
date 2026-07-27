#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
BASELINE_COMMIT=${GATE35_BASELINE_COMMIT:-"$(cat "$ROOT/reports/gate3_5/git_commit_before.txt")"}

git -C "$ROOT" restore --source "$BASELINE_COMMIT" -- \
  "$ROOT/include/boom_state.hpp" \
  "$ROOT/src/rename.cpp" \
  "$ROOT/src/branch.cpp" \
  "$ROOT/src/issue.cpp" \
  "$ROOT/src/synth_module_tops.cpp" \
  "$ROOT/src/boom_core_merged.cpp" \
  "$ROOT/tb/differential/branch_snapshot_tests.cpp"

echo "Restored Gate 3.5 structural source files to $BASELINE_COMMIT"
