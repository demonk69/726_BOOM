#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)"}
source "$ROOT/scripts/common/gate_workspace.sh"
gate_begin gate5_4 pf2
BUILD=$(gate_build_dir directed)
trap 'gate_cleanup_success "$BUILD"' EXIT

g++ -std=c++11 -O2 -Wall -Wextra -Werror \
  -Wno-error=misleading-indentation -Wno-error=unused-label \
  -Wno-unknown-pragmas -I"$ROOT/include" \
  "$ROOT/tb/differential/pf2_predictor_frontend_tests.cpp" \
  "$ROOT/src/frontend.cpp" "$ROOT/src/fetch_buffer.cpp" \
  "$ROOT/src/fetch_packet.cpp" "$ROOT/src/predecode.cpp" \
  "$ROOT/src/predictor.cpp" "$ROOT/src/rvc.cpp" "$ROOT/src/reset.cpp" \
  "$ROOT/src/divider.cpp" \
  -o "$BUILD/pf2_predictor_frontend_tests"

"$BUILD/pf2_predictor_frontend_tests" | tee "$BUILD/pf2_directed.log"
grep -Fxq 'PF2_DIRECTED_PASS checks=2239 failures=0 conditional_mode=SHADOW_ONLY' \
  "$BUILD/pf2_directed.log"
