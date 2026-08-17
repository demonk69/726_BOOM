#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
BUILD="$ROOT/build/gate5_3_fetch_buffer/b2/native"
REPORT="$ROOT/reports/gate5_3_fetch_buffer/b2"
CXXFLAGS=(-std=c++11 -O2 -Wall -Wextra -Werror -Wno-error=misleading-indentation
          -Wno-error=unused-label -Wno-unknown-pragmas -I"$ROOT/include")
mkdir -p "$BUILD" "$REPORT/logs"

FRONTEND=("$ROOT/src/frontend.cpp" "$ROOT/src/fetch_buffer.cpp" "$ROOT/src/rvc.cpp"
          "$ROOT/src/decode.cpp" "$ROOT/src/divider.cpp")
CORE=("$ROOT/src/boom_core_step.cpp" "$ROOT/src/frontend.cpp" "$ROOT/src/fetch_buffer.cpp"
      "$ROOT/src/rvc.cpp" "$ROOT/src/decode.cpp" "$ROOT/src/rename.cpp" "$ROOT/src/rob.cpp"
      "$ROOT/src/issue.cpp" "$ROOT/src/mul.cpp" "$ROOT/src/divider.cpp" "$ROOT/src/execute.cpp"
      "$ROOT/src/branch.cpp" "$ROOT/src/lsu.cpp" "$ROOT/src/completion.cpp" "$ROOT/src/commit.cpp"
      "$ROOT/src/csr.cpp" "$ROOT/src/reset.cpp")

g++ "${CXXFLAGS[@]}" "$ROOT/tb/differential/fetch_buffer_integration_tests.cpp" \
  "${FRONTEND[@]}" -o "$BUILD/focused" 2>"$REPORT/logs/focused_compile.log"
"$BUILD/focused" | tee "$REPORT/logs/focused.log"
grep -q 'GATE5_3_B2_FETCH_BUFFER_INTEGRATION_FOCUSED_PASS checks=169 failures=0' \
  "$REPORT/logs/focused.log"
for marker in upper_half_rvc_production rvc_pc_plus_2_start \
              cross_word_carry_creation partial_cross_word_excluded_from_fetch_buffer \
              faulted_cross_word_lower_half_start_pc; do
  grep -q "SEMANTIC_PASS,$marker" "$REPORT/logs/focused.log"
done

g++ "${CXXFLAGS[@]}" "$ROOT/tb/differential/fetch_buffer_integration_random_tests.cpp" \
  "${FRONTEND[@]}" -o "$BUILD/random" 2>"$REPORT/logs/random_compile.log"
"$BUILD/random" | tee "$REPORT/logs/random.log"

bash "$ROOT/scripts/gate5_2/build_rvc_programs.sh" >"$REPORT/logs/rvc_program_build.log" 2>&1
g++ "${CXXFLAGS[@]}" "${CORE[@]}" \
  "$ROOT/tb/differential/gate5_2_r2_full_core_rvc.cpp" -o "$BUILD/full_core_rvc" \
  2>"$REPORT/logs/full_core_rvc_compile.log"
"$BUILD/full_core_rvc" | tee "$REPORT/logs/full_core_rvc.log"
grep -q 'GATE5_2_R2_FULL_CORE_RVC 11/11 PASS' "$REPORT/logs/full_core_rvc.log"

"$ROOT/scripts/gate4_1/generate_m3c_programs.py" >"$REPORT/logs/m3c_program_build.log" 2>&1
g++ "${CXXFLAGS[@]}" "${CORE[@]}" \
  "$ROOT/tb/differential/rv64m_full_core_tests.cpp" -o "$BUILD/rv64m_full_core" \
  2>"$REPORT/logs/rv64m_full_core_compile.log"
"$BUILD/rv64m_full_core" | tee "$REPORT/logs/rv64m_full_core.log"
grep -q 'M3C native full-core RV64M programs: 15/15 PASS' "$REPORT/logs/rv64m_full_core.log"

printf '%s\n' 'GATE5_3_B2_NATIVE_PASS focused random rvc=11/11 rv64m=15/15'
