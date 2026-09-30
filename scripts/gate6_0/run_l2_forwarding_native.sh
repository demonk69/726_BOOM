#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)"}
BUILD_ROOT=${BOOM_BUILD_ROOT:-/home/lab_726/opencode_tmp/g6_l2/native}
CXX_BIN=${CXX:-g++}
PRODUCT_SOURCES=(
    "$ROOT/src/fetch_buffer.cpp" "$ROOT/src/fetch_packet.cpp"
    "$ROOT/src/rvc.cpp" "$ROOT/src/predecode.cpp" "$ROOT/src/predictor.cpp"
    "$ROOT/src/ftq.cpp" "$ROOT/src/frontend.cpp" "$ROOT/src/decode.cpp"
    "$ROOT/src/rename.cpp" "$ROOT/src/issue.cpp" "$ROOT/src/mul.cpp"
    "$ROOT/src/divider.cpp" "$ROOT/src/execute.cpp" "$ROOT/src/completion.cpp"
    "$ROOT/src/rob.cpp" "$ROOT/src/branch.cpp" "$ROOT/src/lsu.cpp"
    "$ROOT/src/commit.cpp" "$ROOT/src/csr.cpp" "$ROOT/src/reset.cpp"
    "$ROOT/src/boom_core_step.cpp"
)

mkdir -p "$BUILD_ROOT"
for config in 4_4 8_8 16_16 4_16 16_4; do
    lq_depth=${config%_*}
    sq_depth=${config#*_}
    binary="$BUILD_ROOT/g6_l2_$config"
    log="$BUILD_ROOT/$config.log"
    "$CXX_BIN" -std=c++11 -O2 -Wall -Wextra -Werror \
        -Wno-error=misleading-indentation -Wno-error=unused-label \
        -Wno-error=unused-variable \
        -Wno-unknown-pragmas -I"$ROOT/include" \
        -DLQ_DEPTH="$lq_depth" -DSQ_DEPTH="$sq_depth" \
        "$ROOT/tb/differential/g6_l2_forwarding_tests.cpp" \
        "${PRODUCT_SOURCES[@]}" -o "$binary"
    "$binary" | tee "$log"
    grep -q "G6_L2_FORWARDING_PASS LQ=$lq_depth SQ=$sq_depth failures=0" "$log"
    printf 'G6_L2_NATIVE_CONFIG_%s=PASS\n' "$config"
done
printf 'G6_L2_NATIVE_ALL=PASS\n'
