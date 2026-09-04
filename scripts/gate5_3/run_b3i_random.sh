#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
source "$ROOT/scripts/common/gate_workspace.sh"
gate_begin gate5_4 pf2_preservation
BUILD=$(gate_build_dir gate5_3_b3i_random)
REPORT="$ROOT/reports/gate5_3_fetch_buffer/b3i"
mkdir -p "$REPORT/logs"
trap 'gate_cleanup_success "$BUILD"' EXIT
CXXFLAGS=(-std=c++11 -O2 -Wall -Wextra -Werror
          -Wno-error=misleading-indentation -Wno-error=unused-label
          -Wno-unknown-pragmas -I"$ROOT/include")
SOURCES=("$ROOT/src/frontend.cpp" "$ROOT/src/fetch_buffer.cpp"
         "$ROOT/src/fetch_packet.cpp" "$ROOT/src/rvc.cpp"
         "$ROOT/src/predecode.cpp" "$ROOT/src/predictor.cpp"
         "$ROOT/src/decode.cpp" "$ROOT/src/divider.cpp")

g++ "${CXXFLAGS[@]}" "$ROOT/tb/differential/fetch_packet_2lane_random_tests.cpp" \
  "${SOURCES[@]}" -o "$BUILD/random" 2>"$REPORT/logs/random_compile.log"
"$BUILD/random" | tee "$REPORT/logs/random.log"
grep -q 'GATE5_3_B3I_FETCH_PACKET_2LANE_RANDOM_PASS seeds=256 cycles_per_seed=4096' \
  "$REPORT/logs/random.log"
for counter in packet_mask_error partial_enqueue bad_pc order_error drop duplicate \
               stale_side_effect atomicity_error; do
  grep -q "$counter=0" "$REPORT/logs/random.log"
done
