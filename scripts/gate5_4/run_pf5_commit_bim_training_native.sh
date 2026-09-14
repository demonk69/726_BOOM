#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)"}
source "$ROOT/scripts/common/gate_workspace.sh"
gate_begin gate5_4_product_integration pf5
BUILD=$(gate_build_dir native)
trap 'status=$?; if (( status != 0 )); then gate_preserve_failure; fi; gate_cleanup_success "$BUILD"' EXIT

sources=(boom_core_step frontend fetch_buffer fetch_packet predecode predictor rvc decode
         rename rob issue mul divider execute branch lsu completion commit csr reset ftq)
inputs=()
for source in "${sources[@]}"; do inputs+=("$ROOT/src/$source.cpp"); done
g++ -std=c++11 -O2 -Wall -Wextra -Werror \
  -Wno-error=misleading-indentation -Wno-error=unused-label -Wno-unknown-pragmas \
  -DBOOM_FTQ_STORAGE_LUTRAM -DBOOM_PREDICTOR_STORAGE_LUTRAM \
  -I"$ROOT/include" "${inputs[@]}" \
  "$ROOT/tb/differential/pf5_commit_bim_training_tests.cpp" \
  -o "$BUILD/pf5_commit_bim_training_tests"
"$BUILD/pf5_commit_bim_training_tests" | tee "$BUILD/pf5_native.log"
grep -Fxq 'PF5_COMMIT_BIM_TRAINING_PASS checks=6205 failures=0 continuous_commits=64' \
  "$BUILD/pf5_native.log"
