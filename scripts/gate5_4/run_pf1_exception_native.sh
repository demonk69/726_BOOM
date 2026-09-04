#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)"}
: "${BOOM_BUILD_ROOT:=/tmp/boom_hls}"
BUILD="$BOOM_BUILD_ROOT/gate5_4_product_integration/pf1/native"
REPORT="$ROOT/reports/gate5_4_product_integration/pf1"
mkdir -p -- "$BUILD" "$REPORT/logs"
sources=(boom_core_step frontend fetch_packet fetch_buffer predecode predictor rvc decode rename rob issue
         mul divider execute branch lsu completion commit csr reset)
inputs=()
for source in "${sources[@]}"; do inputs+=("$ROOT/src/$source.cpp"); done

for test in exception_recovery_tests exception_recovery_random_tests exception_recovery_program_tests; do
    g++ -std=c++11 -O2 -Wno-unknown-pragmas -I"$ROOT/include" "${inputs[@]}" \
        "$ROOT/tb/differential/$test.cpp" -o "$BUILD/$test" \
        >"$REPORT/logs/${test}_compile.log" 2>&1
    "$BUILD/$test" >"$REPORT/logs/$test.log" 2>&1
done

grep -q 'PF1_DIRECTED checks=1817 failures=0' "$REPORT/logs/exception_recovery_tests.log"
grep -q 'PF1_RANDOM seeds=256 cycles_per_seed=8192.*ordering_error=0.*failures\|PF1_RANDOM seeds=256 cycles_per_seed=8192.*errors=0' \
    "$REPORT/logs/exception_recovery_random_tests.log"
grep -q 'PF1_PROGRAMS cases=8 failures=0' "$REPORT/logs/exception_recovery_program_tests.log"
printf '%s\n' 'PF1_EXCEPTION_NATIVE_PASS directed=1817 random_seeds=256 programs=8'
