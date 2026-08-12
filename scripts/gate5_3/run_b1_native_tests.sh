#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
BUILD="$ROOT/build/gate5_3_fetch_buffer/b1"
REPORT="$ROOT/reports/gate5_3_fetch_buffer/b1"
mkdir -p "$BUILD" "$REPORT/logs"

printf 'depth,directed,random,seeds,cycles_per_seed\n' > "$REPORT/native_test_matrix.csv"
for depth in 2 4 8 16; do
    common=(-std=c++11 -O2 -Wall -Wextra -Werror -Wno-unknown-pragmas
            -I"$ROOT/include" -DFETCH_BUFFER_DEPTH="$depth" "$ROOT/src/fetch_buffer.cpp")
    g++ "${common[@]}" "$ROOT/tb/differential/fetch_buffer_tests.cpp" \
        -o "$BUILD/fetch_buffer_tests_d$depth" \
        2>"$REPORT/logs/directed_d$depth.compile.log"
    "$BUILD/fetch_buffer_tests_d$depth" | tee "$REPORT/logs/directed_d$depth.log"
    grep -q "GATE5_3_B1_FETCH_BUFFER_DIRECTED_PASS depth=$depth" \
        "$REPORT/logs/directed_d$depth.log"

    g++ "${common[@]}" "$ROOT/tb/differential/fetch_buffer_random_tests.cpp" \
        -o "$BUILD/fetch_buffer_random_tests_d$depth" \
        2>"$REPORT/logs/random_d$depth.compile.log"
    "$BUILD/fetch_buffer_random_tests_d$depth" | tee "$REPORT/logs/random_d$depth.log"
    grep -q "GATE5_3_B1_FETCH_BUFFER_RANDOM_PASS depth=$depth seeds=256 cycles_per_seed=4096" \
        "$REPORT/logs/random_d$depth.log"
    printf '%s,PASS,PASS,256,4096\n' "$depth" >> "$REPORT/native_test_matrix.csv"
done

printf 'GATE5_3_B1_NATIVE_PASS depths=2,4,8,16 random=256x4096\n'
