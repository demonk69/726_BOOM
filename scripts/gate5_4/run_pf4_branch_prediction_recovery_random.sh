#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)"}
source "$ROOT/scripts/common/gate_workspace.sh"
gate_begin gate5_4_product_integration pf4
BUILD=$(gate_build_dir random)
trap 'status=$?; if (( status != 0 )); then gate_preserve_failure; fi; gate_cleanup_success "$BUILD"' EXIT

sources=(boom_core_step frontend fetch_buffer fetch_packet predecode predictor rvc decode
         rename rob issue mul divider execute branch lsu completion commit csr reset ftq)
inputs=()
for source in "${sources[@]}"; do inputs+=("$ROOT/src/$source.cpp"); done
g++ -std=c++11 -O2 -Wall -Wextra -Werror \
  -Wno-error=misleading-indentation -Wno-error=unused-label -Wno-unknown-pragmas \
  -DBOOM_FTQ_STORAGE_LUTRAM -I"$ROOT/include" "${inputs[@]}" \
  "$ROOT/tb/differential/pf4_branch_prediction_recovery_random_tests.cpp" \
  -o "$BUILD/pf4_branch_prediction_recovery_random_tests"
"$BUILD/pf4_branch_prediction_recovery_random_tests" | tee "$BUILD/pf4_random.log"
grep -q 'PF4_RANDOM_PASS seeds=256 cycles_per_seed=8192' "$BUILD/pf4_random.log"
grep -Eq 'PF4_RANDOM_PASS.*classification_error=0.*direction_error=0.*target_error=0.*stale_error=0.*redirect_error=0.*redirect_pc_error=0.*no_work_error=0.*rob_recovery_error=0.*ftq_reference_error=0.*generation_error=0.*cfi_mismatch_error=0.*lane_error=0.*rvc_error=0.*jal_error=0.*jalr_error=0.*rename_rollback_error=0.*predictor_training_error=0.*ordering_error=0' "$BUILD/pf4_random.log"
grep -Eq 'PF4_RANDOM_PASS.*correct_t=[1-9][0-9]*.*correct_nt=[1-9][0-9]*.*nt_to_t=[1-9][0-9]*.*t_to_nt=[1-9][0-9]*' "$BUILD/pf4_random.log"
grep -Eq 'PF4_LONG_RUN_PASS steps=1000000 errors=0 correct_t=[1-9][0-9]* correct_nt=[1-9][0-9]* nt_to_t=[1-9][0-9]* t_to_nt=[1-9][0-9]*' "$BUILD/pf4_random.log"
