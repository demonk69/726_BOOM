#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)"}
BUILD=${PF6_RESET_BUILD_DIR:-/tmp/boom_hls/pf6/reset_stress}
REPORT=${PF6_RESET_REPORT_DIR:-/tmp/boom_hls/pf6/results/reset_stress}
rm -rf -- "$BUILD"
mkdir -p -- "$BUILD" "$REPORT"
sources=(boom_core_step frontend fetch_buffer fetch_packet predecode predictor rvc decode
         rename rob issue mul divider execute branch lsu completion commit csr reset ftq)
inputs=()
for source in "${sources[@]}"; do inputs+=("$ROOT/src/$source.cpp"); done
g++ -std=c++11 -O2 -I"$ROOT/include" "${inputs[@]}" \
  "$ROOT/tb/differential/pf6_reset_stress_tests.cpp" \
  -o "$BUILD/pf6_reset_stress_tests"
"$BUILD/pf6_reset_stress_tests" | tee "$REPORT/pf6_reset_stress.log"
grep -q 'PF6_RESET_STRESS_PASS .* failures=0 scenarios=4' "$REPORT/pf6_reset_stress.log"
