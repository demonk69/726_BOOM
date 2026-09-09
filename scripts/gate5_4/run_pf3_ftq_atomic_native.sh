#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)"}
source "$ROOT/scripts/common/gate_workspace.sh"
gate_begin gate5_4_product_integration pf3
BUILD=$(gate_build_dir directed)
trap 'gate_cleanup_success "$BUILD"' EXIT

"$ROOT/scripts/generate_merged.sh"
g++ -std=c++11 -O2 -Wall -Wextra -Werror \
  -Wno-error=misleading-indentation -Wno-error=unused-label \
  -Wno-unknown-pragmas -DBOOM_FTQ_STORAGE_LUTRAM -I"$ROOT/include" \
  "$ROOT/tb/differential/pf3_ftq_atomic_tests.cpp" \
  "$ROOT/src/boom_core_merged.cpp" -o "$BUILD/pf3_ftq_atomic_tests"
"$BUILD/pf3_ftq_atomic_tests" | tee "$BUILD/pf3_directed.log"
grep -q 'PF3_DIRECTED_PASS.*failures=0' "$BUILD/pf3_directed.log"
