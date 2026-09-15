#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)"}
BUILD=${PF6_RANDOM_BUILD_DIR:-/tmp/boom_hls/pf6/integrated_random}
REPORT=${PF6_RANDOM_REPORT_DIR:-/tmp/boom_hls/pf6/results/integrated_random}

rm -rf -- "$BUILD"
mkdir -p -- "$BUILD" "$REPORT"
sources=(boom_core_step frontend fetch_buffer fetch_packet predecode predictor rvc decode
         rename rob issue mul divider execute branch lsu completion commit csr reset ftq)
inputs=()
for source in "${sources[@]}"; do inputs+=("$ROOT/src/$source.cpp"); done
g++ -std=c++11 -O2 -Wall -Wextra -Werror \
  -Wno-error=misleading-indentation -Wno-error=unused-label -Wno-unknown-pragmas \
  -DBOOM_FTQ_STORAGE_LUTRAM -DBOOM_PREDICTOR_STORAGE_LUTRAM -I"$ROOT/include" \
  "${inputs[@]}" "$ROOT/tb/differential/pf6_integrated_random_tests.cpp" \
  -o "$BUILD/pf6_integrated_random_tests"
"$BUILD/pf6_integrated_random_tests" | tee "$REPORT/pf6_integrated_random.log"
grep -q 'PF6_SMALL_STATE_EXHAUSTIVE combinations=128 checks=256 errors=0' "$REPORT/pf6_integrated_random.log"
grep -q 'PF6_INTEGRATED_RANDOM seeds=256 cycles_per_seed=8192 .* errors=0' "$REPORT/pf6_integrated_random.log"
grep -q 'PF6_LONG_RUN steps=2000000 .* errors=0' "$REPORT/pf6_integrated_random.log"
grep -q 'PF6_INTEGRATED_RANDOM_COVERAGE .*FTQ_allocations=.*BIM_updates=.*max_ftq_occupancy=1' "$REPORT/pf6_integrated_random.log"
grep -q 'PF6_LONG_RUN_COVERAGE steps=2000000 .*FTQ_allocations=.*BIM_updates=.*max_ftq_occupancy=1' "$REPORT/pf6_integrated_random.log"

g++ -std=c++11 -O2 -Wall -Wextra -Werror \
  -Wno-error=misleading-indentation -Wno-error=unused-label -Wno-unknown-pragmas \
  -DBOOM_FTQ_STORAGE_LUTRAM -DBOOM_PREDICTOR_STORAGE_LUTRAM -I"$ROOT/include" \
  "${inputs[@]}" "$ROOT/tb/differential/pf2_predictor_frontend_random_tests.cpp" \
  -o "$BUILD/pf6_frontend_reference_tests"
"$BUILD/pf6_frontend_reference_tests" | tee "$REPORT/pf6_frontend_reference.log"
grep -q 'PF2_RANDOM_PASS seeds=256 cycles_per_seed=8192 .* errors=0' \
  "$REPORT/pf6_frontend_reference.log"

g++ -std=c++11 -O2 -Wall -Wextra -Werror \
  -Wno-error=misleading-indentation -Wno-error=unused-label -Wno-unknown-pragmas \
  -DBOOM_FTQ_STORAGE_LUTRAM -DBOOM_PREDICTOR_STORAGE_LUTRAM -I"$ROOT/include" \
  "${inputs[@]}" "$ROOT/tb/differential/pf4_branch_prediction_recovery_random_tests.cpp" \
  -o "$BUILD/pf6_recovery_reference_tests"
"$BUILD/pf6_recovery_reference_tests" | tee "$REPORT/pf6_recovery_reference.log"
grep -q 'PF4_RANDOM_PASS seeds=256 cycles_per_seed=8192' "$REPORT/pf6_recovery_reference.log"
grep -q 'PF4_LONG_RUN_PASS steps=1000000 errors=0' "$REPORT/pf6_recovery_reference.log"
