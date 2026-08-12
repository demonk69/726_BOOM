#!/usr/bin/env bash
set -euo pipefail
ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
REPORT="$ROOT/reports/gate5_2_rvc/r2"
BUILD="$ROOT/tb/differential/gate5_2_r2_build"
mkdir -p "$REPORT/logs" "$BUILD"
bash "$ROOT/scripts/gate5_2/build_rvc_programs.sh" > "$REPORT/logs/r2_program_build.log" 2>&1
SOURCES=(frontend decode rename issue execute branch lsu commit csr completion rob reset divider mul rvc boom_core_step)
ARGS=()
for source in "${SOURCES[@]}"; do ARGS+=("$ROOT/src/$source.cpp"); done
g++ -std=c++11 -O2 -I"$ROOT/include" "${ARGS[@]}" \
  "$ROOT/tb/differential/gate5_2_r2_full_core_rvc.cpp" -o "$BUILD/full_core_rvc" \
  > "$REPORT/logs/r2_native_compile.log" 2>&1
"$BUILD/full_core_rvc" > "$REPORT/logs/r2_native.log" 2>&1
grep -q 'GATE5_2_R2_FULL_CORE_RVC 10/10 PASS' "$REPORT/logs/r2_native.log"
printf '%s\n' 'Gate 5.2 R2 native full-core mixed RVC: 10/10 PASS'
