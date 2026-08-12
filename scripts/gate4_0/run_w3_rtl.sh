#!/usr/bin/env bash
set -uo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
VITIS_HLS_BIN=${VITIS_HLS:-/home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls}
XVLOG=${XVLOG:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xvlog}
XELAB=${XELAB:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xelab}
XSIM=${XSIM:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xsim}
BUILD_DIR=${W3_RTL_BUILD_DIR:-"$ROOT/build/gate4_0/w3_rtl"}
HLS_PROJECT="$BUILD_DIR/hls_project"
SOLUTION=solution_w3_rtl
RTL_DIR="$HLS_PROJECT/$SOLUTION/syn/verilog"
COMPLETION_PROJECT="$BUILD_DIR/hls_completion_project"
COMPLETION_RTL_DIR="$COMPLETION_PROJECT/$SOLUTION/syn/verilog"
DUAL_PENDING_PROJECT="$BUILD_DIR/hls_dual_pending_project"
DUAL_PENDING_RTL_DIR="$DUAL_PENDING_PROJECT/$SOLUTION/syn/verilog"
ROB_WRAP_PROJECT="$BUILD_DIR/hls_rob_wrap_project"
ROB_WRAP_RTL_DIR="$ROB_WRAP_PROJECT/$SOLUTION/syn/verilog"
BRANCH_KILL_PROJECT="$BUILD_DIR/hls_branch_kill_project"
BRANCH_KILL_RTL_DIR="$BRANCH_KILL_PROJECT/$SOLUTION/syn/verilog"
STAGE_DIR="$BUILD_DIR/report_stage"
HLS_TCL="$BUILD_DIR/run_w3_csynth.tcl"
WRAPPER_TEST="$ROOT/tb/differential/w3_rtl_wrapper_tests.cpp"
REPORT_DIR=${W3_RTL_REPORT_DIR:-"$ROOT/reports/gate4_0/w3"}
LOG_DIR="$REPORT_DIR/rtl_logs"
MATRIX="$REPORT_DIR/rtl_test_matrix.csv"

mkdir -p "$BUILD_DIR"
rm -rf "$STAGE_DIR" "$BUILD_DIR/xsim"
if [[ ${W3_REUSE_RTL:-0} != 1 ]]; then
    rm -rf "$HLS_PROJECT" "$COMPLETION_PROJECT" "$DUAL_PENDING_PROJECT" \
        "$ROB_WRAP_PROJECT" "$BRANCH_KILL_PROJECT"
fi
mkdir -p "$STAGE_DIR/logs" "$BUILD_DIR/xsim"

printf '%s\n' \
    "source $ROOT/scripts/create_project.tcl" \
    "add_files -tb -cflags \"-std=c++11 -I$ROOT/include\" $WRAPPER_TEST" \
    'csim_design -clean' \
    "source $ROOT/directives/baseline_directives.tcl" \
    'set_directive_inline "boom::lsu_module"' \
    'set_directive_inline "boom::rob_commit_module"' \
    'csynth_design' \
    'close_project' \
    'exit' > "$HLS_TCL"

if ! "$VITIS_HLS_BIN" -version 2>&1 | grep -q 'v2021.2'; then
    printf '%s\n' 'ERROR: Vitis HLS 2021.2 is required.' >&2
    exit 2
fi

COMMON=(
    "$ROOT/src/boom_core_step.cpp" "$ROOT/src/frontend.cpp" "$ROOT/src/rvc.cpp" "$ROOT/src/decode.cpp"
    "$ROOT/src/rename.cpp" "$ROOT/src/rob.cpp" "$ROOT/src/issue.cpp"
    "$ROOT/src/mul.cpp" "$ROOT/src/divider.cpp" "$ROOT/src/execute.cpp" "$ROOT/src/branch.cpp" "$ROOT/src/lsu.cpp"
    "$ROOT/src/completion.cpp" "$ROOT/src/commit.cpp" "$ROOT/src/csr.cpp" "$ROOT/src/reset.cpp"
    "$ROOT/src/synth_module_tops.cpp"
)
g++ -std=c++11 -DBOOM_HLS_W3_DIAGNOSTIC -DBOOM_HLS_W4A_COMPLETION_DIAGNOSTIC \
    -I"$ROOT/include" "${COMMON[@]}" "$WRAPPER_TEST" \
    -o "$BUILD_DIR/w3_rtl_wrapper_tests" > "$BUILD_DIR/wrapper_compile.log" 2>&1 &&
    "$BUILD_DIR/w3_rtl_wrapper_tests" > "$BUILD_DIR/wrapper_cpp.log" 2>&1 || {
    printf '%s\n' "ERROR: W3 C++ wrapper oracle failed; see $BUILD_DIR/wrapper_cpp.log" >&2
    exit 1
}

if [[ ${W3_REUSE_RTL:-0} != 1 ]]; then
(
    cd "$BUILD_DIR" || exit 1
    BOOM_HLS_GATE=gate4_0_w3 BOOM_HLS_TOP=synth_w3_diagnostic_top \
    BOOM_HLS_PROJECT=hls_project BOOM_HLS_SOLUTION="$SOLUTION" \
    BOOM_HLS_CFLAGS_EXTRA="-DBOOM_HLS_W3_DIAGNOSTIC -DBOOM_HLS_W4A_COMPLETION_DIAGNOSTIC" \
    FPGA_PART=${FPGA_PART:-xczu7ev-ffvc1156-2-e} CLOCK_PERIOD=${CLOCK_PERIOD:-10} \
    "$VITIS_HLS_BIN" -f "$HLS_TCL"
) > "$BUILD_DIR/hls.log" 2>&1 || {
    printf '%s\n' "ERROR: W3 HLS generation failed; see $BUILD_DIR/hls.log" >&2
    exit 1
}

(
    cd "$BUILD_DIR" || exit 1
    BOOM_HLS_GATE=gate4_0_w3 BOOM_HLS_TOP=synth_w3_completion_diagnostic_top \
    BOOM_HLS_PROJECT=hls_completion_project BOOM_HLS_SOLUTION="$SOLUTION" \
    BOOM_HLS_CFLAGS_EXTRA="-DBOOM_HLS_W3_DIAGNOSTIC -DBOOM_HLS_W4A_COMPLETION_DIAGNOSTIC" \
    FPGA_PART=${FPGA_PART:-xczu7ev-ffvc1156-2-e} CLOCK_PERIOD=${CLOCK_PERIOD:-10} \
    "$VITIS_HLS_BIN" -f "$HLS_TCL"
) > "$BUILD_DIR/hls_completion.log" 2>&1 || {
    printf '%s\n' "ERROR: W3 completion HLS generation failed; see $BUILD_DIR/hls_completion.log" >&2
    exit 1
}

for spec in "synth_w3_dual_pending_top:hls_dual_pending_project:hls_dual_pending.log" \
            "synth_w3_rob_wrap_top:hls_rob_wrap_project:hls_rob_wrap.log" \
            "synth_w3_branch_kill_top:hls_branch_kill_project:hls_branch_kill.log"; do
    IFS=: read -r extra_top extra_project extra_log <<< "$spec"
    (
        cd "$BUILD_DIR" || exit 1
        BOOM_HLS_GATE=gate4_0_w3 BOOM_HLS_TOP="$extra_top" \
        BOOM_HLS_PROJECT="$extra_project" BOOM_HLS_SOLUTION="$SOLUTION" \
        BOOM_HLS_CFLAGS_EXTRA="-DBOOM_HLS_W3_DIAGNOSTIC -DBOOM_HLS_W4A_COMPLETION_DIAGNOSTIC" \
        FPGA_PART=${FPGA_PART:-xczu7ev-ffvc1156-2-e} CLOCK_PERIOD=${CLOCK_PERIOD:-10} \
        "$VITIS_HLS_BIN" -f "$HLS_TCL"
    ) > "$BUILD_DIR/$extra_log" 2>&1 || {
        printf '%s\n' "ERROR: $extra_top HLS generation failed; see $BUILD_DIR/$extra_log" >&2
        exit 1
    }
done
fi

for generated_dir in "$RTL_DIR" "$COMPLETION_RTL_DIR" "$DUAL_PENDING_RTL_DIR" \
                     "$ROB_WRAP_RTL_DIR" "$BRANCH_KILL_RTL_DIR"; do
    cp "$generated_dir"/*.dat "$BUILD_DIR/xsim"/ 2>/dev/null || true
done
mapfile -t RTL_FILES < <(printf '%s\n' "$RTL_DIR"/*.v "$COMPLETION_RTL_DIR"/*.v \
    "$DUAL_PENDING_RTL_DIR"/*.v "$ROB_WRAP_RTL_DIR"/*.v | sort)
RTL_FILES+=("$BRANCH_KILL_RTL_DIR"/*.v)
(
    cd "$BUILD_DIR/xsim" || exit 1
    "$XVLOG" "${RTL_FILES[@]}" &&
    "$XVLOG" --sv "$ROOT/rtl_tb/gate4_0/w3_dual_execute_tb.sv" &&
    "$XELAB" w3_dual_execute_tb -s w3_dual_execute_snapshot -timescale 1ns/1ps
) > "$BUILD_DIR/xsim_build.log" 2>&1 || {
    printf '%s\n' "ERROR: W3 XSim build failed; see $BUILD_DIR/xsim_build.log" >&2
    exit 1
}

CASES=(
    "dual_accept:0:000c000b0000030b:both fixed lanes accept and execute"
    "mem_blocked_int_forward:1:000c001300000217:held MEM completion blocks only MEM"
    "int_blocked_mem_forward:2:001d000b00000117:held INT completion blocks only INT"
    "dual_pending:3:0020000008d00002:oldest of two completions is serviced"
    "branch_kill:4:0000000008100400:mispredict kills younger held completion"
    "reset_clear:5:0000000000000000:full reset clears held completions and LSU pending"
    "load_response_int_conflict:6:003e000000d00002:load response owns writeback and holds INT"
    "trace_backpressure:7:000000000a100000:full trace FIFO leaves commit atomic"
    "dmem_backpressure:8:000000000c110000:full DMEM FIFO leaves store commit atomic"
    "rob_wrap_multiple_identity:9:0000005cf8d00001:wrap-safe age selects ROB 31 and preserves ID 92"
    "stale_completion_identity:10:0000000000300000:stale allocation ID has no ROB side effect"
)

printf '%s\n' 'case,scenario,requirement,status,expected,observed,log' > "$STAGE_DIR/rtl_test_matrix.csv"
overall=0
for item in "${CASES[@]}"; do
    IFS=: read -r name scenario expected requirement <<< "$item"
    raw="$STAGE_DIR/logs/$name.stdout.log"
    simlog="$STAGE_DIR/logs/$name.log"
    (
        cd "$BUILD_DIR/xsim" || exit 1
        "$XSIM" w3_dual_execute_snapshot --runall --onerror quit \
            --testplusarg "SCENARIO=$scenario" --testplusarg "EXPECT=$expected" \
            --log "$simlog"
    ) > "$raw" 2>&1
    rc=$?
    observed=$(python3 - "$simlog" <<'PY'
import re
import sys
from pathlib import Path
text = Path(sys.argv[1]).read_text(errors="replace") if Path(sys.argv[1]).exists() else ""
match = re.search(r"observed=([0-9a-fA-F]+)", text)
print(match.group(1).lower() if match else "unavailable")
PY
)
    if [[ $rc -eq 0 && "$observed" == "$expected" ]]; then
        status=PASS
    else
        status=FAIL
        overall=1
    fi
    printf '%s,%s,"%s",%s,%s,%s,%s\n' "$name" "$scenario" "$requirement" \
        "$status" "$expected" "$observed" "${LOG_DIR#$ROOT/}/$name.log" \
        >> "$STAGE_DIR/rtl_test_matrix.csv"
    printf '%-32s %s expected=%s observed=%s\n' "$name" "$status" "$expected" "$observed"
done

cp "$BUILD_DIR/hls.log" "$STAGE_DIR/logs/hls_generation.log"
cp "$BUILD_DIR/hls_completion.log" "$STAGE_DIR/logs/hls_completion_generation.log"
cp "$BUILD_DIR/hls_dual_pending.log" "$STAGE_DIR/logs/hls_dual_pending_generation.log"
cp "$BUILD_DIR/hls_rob_wrap.log" "$STAGE_DIR/logs/hls_rob_wrap_generation.log"
cp "$BUILD_DIR/hls_branch_kill.log" "$STAGE_DIR/logs/hls_branch_kill_generation.log"
cp "$BUILD_DIR/wrapper_cpp.log" "$STAGE_DIR/logs/wrapper_cpp.log"
cp "$BUILD_DIR/xsim_build.log" "$STAGE_DIR/logs/xsim_build.log"
mkdir -p "$LOG_DIR"
cp "$STAGE_DIR/logs"/* "$LOG_DIR"/
cp "$STAGE_DIR/rtl_test_matrix.csv" "$MATRIX"

passed=$(python3 - "$MATRIX" <<'PY'
import csv
import sys
with open(sys.argv[1], newline="") as stream:
    print(sum(row["status"] == "PASS" for row in csv.DictReader(stream)))
PY
)
printf 'Gate 4.0 W3 generated-RTL diagnostics: %s/%s PASS.\n' "$passed" "${#CASES[@]}"
exit "$overall"
