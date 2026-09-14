#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)"}
source "$ROOT/scripts/common/gate_workspace.sh"
gate_begin gate5_4_product_integration pf5_random
BUILD=$(gate_build_dir native)
trap 'status=$?; if (( status != 0 )); then gate_preserve_failure; fi; gate_cleanup_success "$BUILD"' EXIT

sources=(boom_core_step frontend fetch_buffer fetch_packet predecode predictor rvc decode
         rename rob issue mul divider execute branch lsu completion commit csr reset ftq)
inputs=()
for source in "${sources[@]}"; do inputs+=("$ROOT/src/$source.cpp"); done
g++ -std=c++11 -O2 -Wall -Wextra -Werror \
  -Wno-error=misleading-indentation -Wno-error=unused-label -Wno-unknown-pragmas \
  -DBOOM_FTQ_STORAGE_LUTRAM -DBOOM_PREDICTOR_STORAGE_LUTRAM -I"$ROOT/include" \
  "${inputs[@]}" \
  "$ROOT/tb/differential/pf5_commit_bim_training_random_tests.cpp" \
  -o "$BUILD/pf5_commit_bim_training_random_tests"
"$BUILD/pf5_commit_bim_training_random_tests" | tee "$BUILD/pf5_random.log"
grep -q 'PF5_SMALL_STATE_EXHAUSTIVE combinations=128 checks=256 errors=0' "$BUILD/pf5_random.log"
grep -q 'PF5_RANDOM seeds=256 cycles_per_seed=8192 .* errors=0' "$BUILD/pf5_random.log"
grep -q 'PF5_LONG_RUN steps=1000000 .* errors=0' "$BUILD/pf5_random.log"
