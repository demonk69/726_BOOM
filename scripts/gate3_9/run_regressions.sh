#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
BUILD_DIR="$ROOT/build/gate3_9/final_regression"
REPORT_DIR="$ROOT/reports/gate3_9/regression_after_artifacts"
LOG_DIR="$REPORT_DIR/logs"
TRACE_DIR="$REPORT_DIR/hls_traces"
BASELINE_TRACES="$ROOT/reports/gate3_9/baseline_artifacts/hls_traces"
CXX_BIN=${CXX:-g++}
VITIS_HLS_BIN=${VITIS_HLS:-vitis_hls}

mkdir -p "$BUILD_DIR" "$LOG_DIR" "$TRACE_DIR"

COMMON_SRCS=(
  "$ROOT/src/boom_core_step.cpp" "$ROOT/src/frontend.cpp" "$ROOT/src/rvc.cpp" "$ROOT/src/decode.cpp"
  "$ROOT/src/rename.cpp" "$ROOT/src/rob.cpp" "$ROOT/src/issue.cpp"
  "$ROOT/src/divider.cpp" "$ROOT/src/execute.cpp" "$ROOT/src/branch.cpp" "$ROOT/src/lsu.cpp"
  "$ROOT/src/commit.cpp" "$ROOT/src/csr.cpp"
)

compile_test() {
  "$CXX_BIN" -std=c++11 -I"$ROOT/include" "${COMMON_SRCS[@]}" "$2" -o "$BUILD_DIR/$1"
}

"$ROOT/scripts/generate_merged.sh" > "$LOG_DIR/generate_merged.log" 2>&1
"$CXX_BIN" -std=c++11 -I"$ROOT/include" -c "$ROOT/src/boom_core_merged.cpp" \
  -o "$BUILD_DIR/boom_core_merged.o" > "$LOG_DIR/merged_compile.log" 2>&1

compile_test directed_tests "$ROOT/tb/differential/directed_tests.cpp"
compile_test gate1_regression_tests "$ROOT/tb/differential/gate1_regression_tests.cpp"
compile_test lsu_minimal_tests "$ROOT/tb/differential/lsu_minimal_tests.cpp"
compile_test branch_snapshot_tests "$ROOT/tb/differential/branch_snapshot_tests.cpp"
compile_test branch_snapshot_random_tests "$ROOT/tb/differential/branch_snapshot_random_tests.cpp"
compile_test iq_compaction_tests "$ROOT/tb/differential/iq_compaction_tests.cpp"
"$CXX_BIN" -std=c++11 -I"$ROOT/include" "${COMMON_SRCS[@]}" "$ROOT/src/reset.cpp" \
  "$ROOT/tb/differential/reset_architecture_tests.cpp" -o "$BUILD_DIR/reset_architecture_tests"

for test in directed_tests gate1_regression_tests lsu_minimal_tests branch_snapshot_tests iq_compaction_tests reset_architecture_tests; do
  "$BUILD_DIR/$test" > "$LOG_DIR/$test.log" 2>&1
done

: > "$LOG_DIR/branch_snapshot_random_tests.log"
for seed in default 0x13579bdf 0x2468ace0 0x10203040 0x55667788 0xdeadbeef 0x0badc0de 0xc001d00d 0x31415926 0x27182818 0xabcdef01 0x12345678 0x87654321 0xfeedface 0x600dcafe 0x5eed1234 0x01020304 0x89abcdef 0xf00df00d 0x55aa55aa 0xaa55aa55; do
  "$BUILD_DIR/branch_snapshot_random_tests" "$seed" >> "$LOG_DIR/branch_snapshot_random_tests.log" 2>&1
done

"$CXX_BIN" -std=c++11 -I"$ROOT/include" "${COMMON_SRCS[@]}" \
  "$ROOT/tb/differential/hls_prefix_trace_tb.cpp" -o "$BUILD_DIR/hls_prefix_trace_tb"
HLS_PROJECT_ROOT="$ROOT" HLS_TRACE_OUT_DIR="$TRACE_DIR" HLS_TRACE_SOURCE=hls_cpp HLS_TRACE_MODE=complete \
  "$BUILD_DIR/hls_prefix_trace_tb" > "$LOG_DIR/hls_cpp_trace.log" 2>&1

if ! command -v "$VITIS_HLS_BIN" >/dev/null 2>&1; then
  VITIS_HLS_BIN=/home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls
fi
FPGA_PART=${FPGA_PART:-xczu7ev-ffvc1156-2-e} CLOCK_PERIOD=${CLOCK_PERIOD:-10} HLS_PROJECT_ROOT="$ROOT" \
  HLS_TRACE_OUT_DIR="$TRACE_DIR" HLS_TRACE_SOURCE=hls_csim HLS_TRACE_MODE=complete \
  "$VITIS_HLS_BIN" -f "$ROOT/scripts/run_hls_prefix_trace_csim.tcl" > "$LOG_DIR/hls_csim_trace.log" 2>&1

python3 "$ROOT/scripts/gate3_7/compare_traces.py" --baseline "$BASELINE_TRACES" \
  --actual "$TRACE_DIR" --output "$REPORT_DIR/trace_diff.md"
python3 "$ROOT/scripts/run_full_program_arch_diff.py" --root "$ROOT" \
  --hls-source hls_cpp --hls-source hls_csim --hls-trace-dir "$TRACE_DIR" \
  --csv-output "$REPORT_DIR/full_program_architectural_diff.csv" \
  --md-output "$REPORT_DIR/full_program_architectural_diff.md" > "$LOG_DIR/full_program_arch_diff.log" 2>&1

printf '%s\n' 'Gate 3.9 accepted-source regressions complete.'
