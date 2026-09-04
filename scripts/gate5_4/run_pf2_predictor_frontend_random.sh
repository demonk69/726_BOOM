#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)"}
source "$ROOT/scripts/common/gate_workspace.sh"
gate_begin gate5_4 pf2
BUILD=$(gate_build_dir random)
trap 'gate_cleanup_success "$BUILD"' EXIT

g++ -std=c++11 -O2 -Wall -Wextra -Werror \
  -Wno-error=misleading-indentation -Wno-error=unused-label \
  -Wno-unknown-pragmas -I"$ROOT/include" \
  "$ROOT/tb/differential/pf2_predictor_frontend_random_tests.cpp" \
  "$ROOT/src/frontend.cpp" "$ROOT/src/fetch_buffer.cpp" \
  "$ROOT/src/fetch_packet.cpp" "$ROOT/src/predecode.cpp" \
  "$ROOT/src/predictor.cpp" "$ROOT/src/rvc.cpp" "$ROOT/src/divider.cpp" \
  -o "$BUILD/pf2_predictor_frontend_random_tests"

"$BUILD/pf2_predictor_frontend_random_tests" | tee "$BUILD/pf2_random.log"
grep -Fxq 'PF2_RANDOM_PASS seeds=256 cycles_per_seed=8192 predecode_error=0 predictor_request_error=0 predictor_response_error=0 latency_error=0 stale_response_error=0 packet_drop=0 packet_duplicate=0 packet_mask_error=0 jal_target_error=0 conditional_shadow_error=0 jalr_prediction_error=0 frontend_pc_error=0 fetch_buffer_ordering_error=0 reset_error=0 redirect_priority_error=0 errors=0' \
  "$BUILD/pf2_random.log"
