#!/usr/bin/env bash
set -uo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
REPORT="$ROOT/reports/gate5_3_fetch_buffer/b2/regression"
BUILD="$ROOT/build/gate5_3_fetch_buffer/b2/regression"
RTL="$ROOT/build/gate5_3_fetch_buffer/b2/phase_d/retry_20260816/boom_core_top_hls/solution_phase_d/syn/verilog"
XSIM_BUILD="$BUILD/gate3_9/xsim"
FRONTEND_TAG=gate5_3_b2_phase_e_frontend
FRONTEND_RTL="$ROOT/boom_hls_${FRONTEND_TAG}_synth_frontend_verify_top/solution_module/syn/verilog"
REAL_PATH=$PATH
SHIM_DIR="$BUILD/compiler_shim"
STATUS="$REPORT/run_status.csv"
COMMANDS="$REPORT/commands.log"

mkdir -p "$REPORT" "$REPORT/logs" "$BUILD" "$SHIM_DIR"
ln -sf "$ROOT/scripts/gate5_3/b2_g++" "$SHIM_DIR/g++"
printf '%s\n' 'requirement,status,exit_code,log' > "$STATUS"
: > "$COMMANDS"

run_check() {
    local name=$1
    shift
    local log="$REPORT/logs/$name.log"
    printf '$' >> "$COMMANDS"
    printf ' %q' "$@" >> "$COMMANDS"
    printf ' > %q 2>&1\n' "$log" >> "$COMMANDS"
    "$@" > "$log" 2>&1
    local rc=$?
    local result=PASS
    (( rc == 0 )) || result=FAIL
    printf '%s,%s,%s,%s\n' "$name" "$result" "$rc" "logs/$name.log" >> "$STATUS"
    return 0
}

freshness() {
    python3 - "$ROOT" "$ROOT/reports/gate5_3_fetch_buffer/b2/phase_d/source_freshness_manifest.csv" "$RTL/boom_core_top.v" <<'PY'
import csv, hashlib, sys
from pathlib import Path
root, manifest, rtl = map(Path, sys.argv[1:])
if not rtl.is_file():
    raise SystemExit(f"missing phase D RTL: {rtl}")
with manifest.open(newline="") as stream:
    rows = list(csv.DictReader(stream))
checked = 0
for row in rows:
    if row["freshness_scope"] != "INCLUDED" or row["path"] == "AGGREGATE":
        continue
    path = root / row["path"]
    actual = hashlib.sha256(path.read_bytes()).hexdigest()
    if actual != row["sha256"]:
        raise SystemExit(f"phase D RTL source mismatch: {row['path']} {actual} != {row['sha256']}")
    checked += 1
print(f"B2_PHASE_D_RTL_FRESHNESS_PASS inputs={checked} rtl={rtl}")
PY
}

rvc() {
    local dir="$BUILD/rvc"
    mkdir -p "$dir"
    /usr/bin/g++ -std=c++11 -O2 -Wall -Wextra -Werror \
        -Wno-error=misleading-indentation -Wno-unknown-pragmas -I"$ROOT/include" \
        "$ROOT/src/rvc.cpp" "$ROOT/tb/differential/rvc_decompress_tests.cpp" \
        -o "$dir/decompress" 2>"$REPORT/logs/rvc_decompress_compile.log" &&
    "$dir/decompress" >"$REPORT/logs/rvc_decompress.log" 2>&1 &&
    grep -qx GATE5_2_R1_RVC_DECOMPRESS_PASS "$REPORT/logs/rvc_decompress.log" &&
    /usr/bin/g++ -std=c++11 -O2 -Wall -Wextra -Werror \
        -Wno-error=misleading-indentation -Wno-unknown-pragmas -I"$ROOT/include" \
        "$ROOT/src/rvc.cpp" "$ROOT/src/decode.cpp" "$ROOT/src/divider.cpp" \
        "$ROOT/tb/differential/rvc_decode_cross_tests.cpp" -o "$dir/decode_cross" \
        2>"$REPORT/logs/rvc_decode_cross_compile.log" &&
    "$dir/decode_cross" >"$REPORT/logs/rvc_decode_cross.log" 2>&1 &&
    grep -qx GATE5_2_R1_RVC_DECODE_CROSS_PASS "$REPORT/logs/rvc_decode_cross.log"
}

gate5_1_rtl() {
    BOOM_HLS_GATE="$FRONTEND_TAG" "$ROOT/scripts/run_module_csynth.sh" \
        synth_frontend_verify_top >"$REPORT/logs/gate5_1_csynth.log" 2>&1 || return
    local work="$BUILD/gate5_1/xsim"
    mkdir -p "$work"
    rm -rf "$work"/*
    cp "$FRONTEND_RTL"/*.dat "$work"/ 2>/dev/null || true
    mapfile -t files < <(printf '%s\n' "$FRONTEND_RTL"/*.v | sort)
    (
        cd "$work" || exit
        /home/lab_726/Xilinx/Vivado/2021.2/bin/xvlog "${files[@]}" &&
        /home/lab_726/Xilinx/Vivado/2021.2/bin/xvlog --sv "$ROOT/rtl_tb/frontend_verify_rtl_tb.sv" &&
        /home/lab_726/Xilinx/Vivado/2021.2/bin/xelab frontend_verify_rtl_tb \
            -s gate5_3_b2_phase_e_frontend -timescale 1ns/1ps &&
        /home/lab_726/Xilinx/Vivado/2021.2/bin/xsim gate5_3_b2_phase_e_frontend \
            --runall --onerror quit --log "$REPORT/logs/gate5_1_xsim.log"
    ) >"$REPORT/logs/gate5_1_xsim.stdout.log" 2>"$REPORT/logs/gate5_1_xsim_build.log" &&
    grep -q 'FRONTEND_VERIFY_RTL_PASS cases=33' "$REPORT/logs/gate5_1_xsim.log"
}

gate3_9() {
    rm -rf "$XSIM_BUILD"
    GATE3_9_RTL_DIR="$RTL" GATE3_9_XSIM_BUILD="$XSIM_BUILD" \
        "$ROOT/scripts/gate3_9/build_xsim.sh" >"$REPORT/logs/gate3_9_build.log" 2>&1 || return
    GATE3_9_REPORT_ROOT="$REPORT/gate3_9" \
    GATE3_9_RESULTS="$REPORT/gate3_9/rtl_run_status.csv" \
    GATE3_9_SUITE_LOG_DIR="$REPORT/gate3_9/logs/suite" \
    GATE3_9_MATRIX="$REPORT/gate3_9/rtl_test_matrix.csv" \
    GATE3_9_RESET_LATENCY="$REPORT/gate3_9/reset_latency.csv" \
    GATE3_9_XSIM_BUILD="$XSIM_BUILD" \
        "$ROOT/scripts/gate3_9/run_rtl_suite.sh"
}

m3c_full_rtl() {
    BOOM_M3C_FULL_RTL_BUILD_DIR="$BUILD/m3c_full_rtl" \
    BOOM_M3C_FULL_RTL_REPORT_DIR="$REPORT/m3c/full_core_rtl" \
    BOOM_M3C_REUSE_XSIM_BUILD="$XSIM_BUILD" \
    BOOM_M3C_REUSE_XSIM_SNAPSHOT=gate3_9_snapshot \
        "$ROOT/scripts/gate4_1/run_m3c_full_core_rtl.sh"
}

summary() {
    python3 - "$STATUS" "$REPORT/../regression_after.md" <<'PY'
import csv, sys
from pathlib import Path
status, output = map(Path, sys.argv[1:])
rows = {row["requirement"]: row for row in csv.DictReader(status.open(newline=""))}
specs = (
 ("Gate 5.1 focused generated RTL", "gate5_1_focused_rtl", "33/33", "regression/logs/gate5_1_xsim.log"),
 ("Gate 5.2 RVC exhaustive and Decode cross", "gate5_2_rvc", "65,536/65,536; 38,551/38,551", "regression/logs/rvc_decompress.log; regression/logs/rvc_decode_cross.log"),
 ("W3 canonical software", "w4e", "400/400", "regression/w4e/regression/w4e/product_full/regression_after.md"),
 ("W4E", "w4e", "95/95 directed; 128/128 random seeds", "regression/w4e/regression_after.md"),
 ("Gate 3.9 generated RTL", "gate3_9", "49/49", "regression/gate3_9/rtl_test_matrix.csv"),
 ("M3C/RV64M native", "m3c_native", "directed/random and 15/15", "regression/m3c/native/logs"),
 ("M3C/RV64M csim", "m3c_csim", "15/15", "regression/m3c/csim/vitis_csim.log"),
 ("M3C/RV64M full-core RTL", "m3c_full_rtl", "15/15", "regression/m3c/full_core_rtl/full_core_rtl_matrix.csv"),
 ("M3C focused RTL", "m3c_focused_rtl", "30/30", "regression/m3c/focused_rtl/rtl_test_matrix.csv"),
 ("W3 focused RTL", "w3_w4_focused_rtl", "11/11", "regression/w3_w4_focused/w3_current/rtl_test_matrix.csv"),
 ("W4 focused RTL", "w3_w4_focused_rtl", "20/20", "regression/w3_w4_focused/rtl_test_matrix.csv"),
 ("Full-program architectural diff", "w4e", "10/10", "regression/w4e/regression/w4e/product_full/full_program_architectural_diff.csv"),
 ("Partial-order", "w4e", "7/7", "regression/w4e/regression/w4e/product_full/logs/partial_order.log"),
)
lines = ["# Gate 5.3 B2 Phase E Preservation Regressions", "",
         "All results below are based only on this current B2 modular-source run.", "",
         "| Requirement | Status | Current-run result | Evidence |", "|---|---:|---:|---|"]
blocked_on_w3 = {"W4E", "Full-program architectural diff", "Partial-order"}
for label, key, result, evidence in specs:
    row = rows.get(key)
    state = row["status"] if row else "BLOCKED"
    if label in blocked_on_w3 and state == "FAIL":
        state = "BLOCKED"
    shown = result if state == "PASS" else f"exit {row['exit_code']}" if row else "not run"
    lines.append(f"| {label} | {state} | {shown} | `{evidence}` |")
closed = all((rows.get(key) or {}).get("status") == "PASS" for _, key, _, _ in specs)
lines += ["", f"Phase E closed: **{'YES' if closed else 'NO'}**.",
          "Canonical csynth phase F was not run."]
output.write_text("\n".join(lines) + "\n")
PY
}

run_check phase_d_rtl_freshness freshness
run_check gate5_1_focused_rtl gate5_1_rtl
run_check gate5_2_rvc rvc
run_check w4e env PATH="$SHIM_DIR:$REAL_PATH" CXX="$ROOT/scripts/gate5_3/b2_g++" \
    VITIS_HLS="$ROOT/scripts/gate5_3/b2_vitis_hls" \
    BOOM_W4E_BUILD_DIR="$BUILD/w4e" BOOM_W4E_REPORT_DIR="$REPORT/w4e" \
    "$ROOT/scripts/gate4_0/run_w4e_regressions.sh"
run_check gate3_9 gate3_9
run_check m3c_native env PATH="$SHIM_DIR:$REAL_PATH" CXX="$ROOT/scripts/gate5_3/b2_g++" \
    BOOM_M3C_BUILD_DIR="$BUILD/m3c_native" BOOM_M3C_REPORT_DIR="$REPORT/m3c/native" \
    "$ROOT/scripts/gate4_1/run_m3c_tests.sh"
run_check m3c_csim env BOOM_M3C_CSIM_BUILD_DIR="$BUILD/m3c_csim" \
    BOOM_M3C_CSIM_REPORT_DIR="$REPORT/m3c/csim" "$ROOT/scripts/gate4_1/run_m3c_csim.sh"
run_check m3c_full_rtl m3c_full_rtl
run_check m3c_focused_rtl env PATH="$SHIM_DIR:$REAL_PATH" \
    BOOM_M3C_RTL_BUILD_DIR="$BUILD/m3c_focused_rtl" \
    BOOM_M3C_RTL_REPORT_DIR="$REPORT/m3c/focused_rtl" \
    "$ROOT/scripts/gate4_1/run_m3c_rv64m_rtl.sh"
run_check w3_w4_focused_rtl env PATH="$SHIM_DIR:$REAL_PATH" \
    W4_RERUN_W3_FOCUSED=1 W4_RTL_BUILD_DIR="$BUILD/w3_w4_focused" \
    W4_RTL_REPORT_DIR="$REPORT/w3_w4_focused" W4_RTL_TRACE_DIR="$REPORT/w3_w4_focused/traces" \
    "$ROOT/scripts/gate4_0/run_w4_rtl.sh"
summary

python3 - "$STATUS" <<'PY'
import csv, sys
rows = list(csv.DictReader(open(sys.argv[1], newline="")))
raise SystemExit(0 if rows and all(row["status"] == "PASS" for row in rows) else 1)
PY
