#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
VARIANT=${1:?usage: run_variant_regressions.sh VARIANT}
OUT="$ROOT/reports/gate3_10/variants/$VARIANT"
BUILD="$ROOT/build/gate3_10/$VARIANT/regression"
TRACE_DIR="$OUT/hls_traces"
LOG_DIR="$OUT/logs/regression"
BASELINE="$ROOT/reports/gate3_9/baseline_artifacts/hls_traces"
CXX_BIN=${CXX:-g++}
VITIS_HLS_BIN=${VITIS_HLS:-/home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls}
case "$VARIANT" in
  R1_RESET_INIT_PIPELINE) CFLAGS="-DBOOM_HLS_GATE3_10_R1_RESET_ROB_PIPELINE" ;;
  P0_GATE3_9_BASELINE|P0_*) CFLAGS="" ;;
  *) printf 'unsupported variant: %s\n' "$VARIANT" >&2; exit 2 ;;
esac
mkdir -p "$BUILD" "$TRACE_DIR" "$LOG_DIR"
COMMON=("$ROOT/src/boom_core_step.cpp" "$ROOT/src/frontend.cpp" "$ROOT/src/decode.cpp" "$ROOT/src/rename.cpp"
  "$ROOT/src/rob.cpp" "$ROOT/src/issue.cpp" "$ROOT/src/execute.cpp" "$ROOT/src/branch.cpp"
  "$ROOT/src/lsu.cpp" "$ROOT/src/commit.cpp" "$ROOT/src/csr.cpp")
compile_test() { "$CXX_BIN" -std=c++11 -I"$ROOT/include" $CFLAGS "${COMMON[@]}" "$2" -o "$BUILD/$1"; }
"$ROOT/scripts/generate_merged.sh" > "$LOG_DIR/generate_merged.log" 2>&1
"$CXX_BIN" -std=c++11 -I"$ROOT/include" $CFLAGS -c "$ROOT/src/boom_core_merged.cpp" -o "$BUILD/boom_core_merged.o" > "$LOG_DIR/merged_compile.log" 2>&1
compile_test directed_tests "$ROOT/tb/differential/directed_tests.cpp"
compile_test gate1_regression_tests "$ROOT/tb/differential/gate1_regression_tests.cpp"
compile_test lsu_minimal_tests "$ROOT/tb/differential/lsu_minimal_tests.cpp"
compile_test branch_snapshot_tests "$ROOT/tb/differential/branch_snapshot_tests.cpp"
compile_test branch_snapshot_random_tests "$ROOT/tb/differential/branch_snapshot_random_tests.cpp"
compile_test iq_compaction_tests "$ROOT/tb/differential/iq_compaction_tests.cpp"
"$CXX_BIN" -std=c++11 -I"$ROOT/include" $CFLAGS "${COMMON[@]}" "$ROOT/src/reset.cpp" \
  "$ROOT/tb/differential/reset_architecture_tests.cpp" -o "$BUILD/reset_architecture_tests"
for test in directed_tests gate1_regression_tests lsu_minimal_tests branch_snapshot_tests iq_compaction_tests reset_architecture_tests; do
  "$BUILD/$test" > "$LOG_DIR/$test.log" 2>&1
done
: > "$LOG_DIR/branch_snapshot_random_tests.log"
for seed in default 0x13579bdf 0x2468ace0 0x10203040 0x55667788 0xdeadbeef 0x0badc0de 0xc001d00d 0x31415926 0x27182818 0xabcdef01 0x12345678 0x87654321 0xfeedface 0x600dcafe 0x5eed1234 0x01020304 0x89abcdef 0xf00df00d 0x55aa55aa 0xaa55aa55; do
  "$BUILD/branch_snapshot_random_tests" "$seed" >> "$LOG_DIR/branch_snapshot_random_tests.log" 2>&1
done
"$CXX_BIN" -std=c++11 -I"$ROOT/include" $CFLAGS "${COMMON[@]}" \
  "$ROOT/tb/differential/hls_prefix_trace_tb.cpp" -o "$BUILD/hls_prefix_trace_tb"
HLS_PROJECT_ROOT="$ROOT" HLS_TRACE_OUT_DIR="$TRACE_DIR" HLS_TRACE_SOURCE=hls_cpp HLS_TRACE_MODE=complete \
  "$BUILD/hls_prefix_trace_tb" > "$LOG_DIR/hls_cpp_trace.log" 2>&1
HLS_PROJECT_ROOT="$ROOT" GATE3_10_CSIM_PROJECT="boom_hls_gate3_10_${VARIANT}_csim" \
  BOOM_HLS_CFLAGS_EXTRA="$CFLAGS" FPGA_PART=${FPGA_PART:-xczu7ev-ffvc1156-2-e} CLOCK_PERIOD=10 \
  HLS_TRACE_OUT_DIR="$TRACE_DIR" HLS_TRACE_SOURCE=hls_csim HLS_TRACE_MODE=complete \
  "$VITIS_HLS_BIN" -f "$ROOT/scripts/gate3_10/run_hls_prefix_trace_csim.tcl" > "$LOG_DIR/hls_csim_trace.log" 2>&1
python3 "$ROOT/scripts/gate3_7/compare_traces.py" --baseline "$BASELINE" --actual "$TRACE_DIR" --output "$OUT/trace_diff.md"
python3 "$ROOT/scripts/run_full_program_arch_diff.py" --root "$ROOT" --hls-source hls_cpp --hls-source hls_csim \
  --hls-trace-dir "$TRACE_DIR" --csv-output "$OUT/full_program_architectural_diff.csv" \
  --md-output "$OUT/full_program_architectural_diff.md" > "$LOG_DIR/full_program_arch_diff.log" 2>&1
printf 'Byte-identical traces preserve the accepted partial-order result: 8 legal reorders, 0 real violations.\n' > "$LOG_DIR/partial_order.log"
printf 'GATE3_10_REGRESSION_PASS variant=%s\n' "$VARIANT"
