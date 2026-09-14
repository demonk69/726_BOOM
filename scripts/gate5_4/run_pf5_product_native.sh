#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)"}
source "$ROOT/scripts/common/gate_workspace.sh"
gate_begin gate5_4_product_integration pf5_product_native
BUILD=$(gate_build_dir full_core)
PROGRAM_BUILD=${PF5_PROGRAM_BUILD:-/tmp/boom_hls/pf5/programs}
REPORT="$ROOT/reports/gate5_4_product_integration/pf5"
trap 'status=$?; if (( status != 0 )); then gate_preserve_failure; fi; gate_cleanup_success "$BUILD"' EXIT
PF5_PROGRAM_BUILD="$PROGRAM_BUILD" "$ROOT/scripts/gate5_4/build_pf5_product_programs.sh"
sources=(boom_core_step frontend fetch_buffer fetch_packet predecode predictor rvc decode
         rename rob issue mul divider execute branch lsu completion commit csr reset ftq)
inputs=()
for source in "${sources[@]}"; do inputs+=("$ROOT/src/$source.cpp"); done
g++ -std=c++11 -O2 -Wall -Wextra -Werror \
  -Wno-error=misleading-indentation -Wno-error=unused-label -Wno-unknown-pragmas \
  -DBOOM_FTQ_STORAGE_LUTRAM -DBOOM_PREDICTOR_STORAGE_LUTRAM \
  -DPF5_TRAINING_EXPECTED -I"$ROOT/include" "${inputs[@]}" \
  "$ROOT/tb/differential/pf4_product_programs.cpp" -o "$BUILD/pf5_product_programs"
PF5_PROGRAM_BUILD="$PROGRAM_BUILD" "$BUILD/pf5_product_programs" |
  tee "$BUILD/pf5_product_native.log" "$REPORT/pf5_product_native.log"
grep -Fxq 'PF5_PRODUCT_PROGRAMS 12/12 PASS' "$BUILD/pf5_product_native.log"
[[ $(grep -c '^PF5_COVERAGE_PROGRAM .*bim_preserved=false .*verdict=PASS$' \
  "$BUILD/pf5_product_native.log") -ge 6 ]]
[[ $(grep -c '^PF5_PROGRAM .*training_dropped=0 training_duplicate=0 stale_rejected=0 .*verdict=PASS$' \
  "$BUILD/pf5_product_native.log") -eq 12 ]]
printf '%s\n' 'PF5_PRODUCT_NATIVE_PASS 12/12 learning_visible=6/12'
