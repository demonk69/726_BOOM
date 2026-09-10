#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)"}
source "$ROOT/scripts/common/gate_workspace.sh"
gate_begin gate5_4_product_integration pf4_product_native
BUILD=$(gate_build_dir full_core)
PROGRAM_BUILD=${PF4_PROGRAM_BUILD:-/tmp/boom_hls/pf4/programs}
trap 'status=$?; if (( status != 0 )); then gate_preserve_failure; fi; gate_cleanup_success "$BUILD"' EXIT
PF4_PROGRAM_BUILD="$PROGRAM_BUILD" "$ROOT/scripts/gate5_4/build_pf4_product_programs.sh"
sources=(boom_core_step frontend fetch_buffer fetch_packet predecode predictor rvc decode
         rename rob issue mul divider execute branch lsu completion commit csr reset ftq)
inputs=()
for source in "${sources[@]}"; do inputs+=("$ROOT/src/$source.cpp"); done
g++ -std=c++11 -O2 -Wall -Wextra -Werror \
  -Wno-error=misleading-indentation -Wno-error=unused-label -Wno-unknown-pragmas \
  -DBOOM_FTQ_STORAGE_LUTRAM -I"$ROOT/include" "${inputs[@]}" \
  "$ROOT/tb/differential/pf4_product_programs.cpp" -o "$BUILD/pf4_product_programs"
PF4_PROGRAM_BUILD="$PROGRAM_BUILD" "$BUILD/pf4_product_programs" | tee "$BUILD/pf4_product_native.log"
grep -Fxq 'PF4_PRODUCT_PROGRAMS 12/12 PASS' "$BUILD/pf4_product_native.log"
grep -Eq '^PF4_PROGRAM .*bim_preserved=true .*verdict=PASS$' "$BUILD/pf4_product_native.log"
