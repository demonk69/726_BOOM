#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)"}
source "$ROOT/scripts/common/gate_workspace.sh"
gate_begin gate5_4_product_integration pf3
BUILD=$(gate_build_dir random)
trap 'gate_cleanup_success "$BUILD"' EXIT

g++ -std=c++11 -O2 -Wall -Wextra -Werror -Wno-unknown-pragmas \
  -DBOOM_FTQ_STORAGE_LUTRAM -I"$ROOT/include" \
  "$ROOT/tb/differential/pf3_ftq_atomic_random_tests.cpp" \
  -o "$BUILD/pf3_ftq_atomic_random_tests"
"$BUILD/pf3_ftq_atomic_random_tests" | tee "$BUILD/pf3_random.log"
grep -q 'PF3_RANDOM_PASS seeds=256 cycles_per_seed=8192' "$BUILD/pf3_random.log"
grep -q 'PF3_LONG_RUN_STEPS=1000000 FTQ_LEAK_ERRORS=0' "$BUILD/pf3_random.log"
