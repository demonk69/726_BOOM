#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
BUILD="$ROOT/build/gate5_3_fetch_buffer/b3i"
REPORT="$ROOT/reports/gate5_3_fetch_buffer/b3i"
RTL="$BUILD/fetch_packet_hls/solution_b3i_packet/syn/verilog"
TB="$ROOT/rtl_tb/fetch_packet_2lane_rtl_tb.sv"
FRONTEND_RTL="$BUILD/fetch_packet_frontend_hls/solution_b3i_packet_frontend/syn/verilog"
FRONTEND_TB="$ROOT/rtl_tb/fetch_packet_frontend_rtl_tb.sv"
VITIS_HLS=${VITIS_HLS:-/home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls}
XVLOG=${XVLOG:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xvlog}
XELAB=${XELAB:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xelab}
XSIM=${XSIM:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xsim}

for tool in "$VITIS_HLS" "$XVLOG" "$XELAB" "$XSIM"; do
    [[ -x "$tool" ]] || { printf 'ERROR: missing %s\n' "$tool" >&2; exit 2; }
done
mkdir -p "$BUILD/xsim" "$REPORT/logs"

HLS_BOOM_ROOT="$ROOT" "$VITIS_HLS" -f "$ROOT/scripts/gate5_3/b3i_packet_csynth.tcl" \
    >"$REPORT/logs/packet_csynth.log" 2>&1
[[ -s "$RTL/synth_fetch_packet_top.v" ]] || {
    printf 'ERROR: canonical synth_fetch_packet_top RTL was not generated\n' >&2; exit 2;
}
HLS_BOOM_ROOT="$ROOT" "$VITIS_HLS" -f "$ROOT/scripts/gate5_3/b3i_packet_frontend_csynth.tcl" \
    >"$REPORT/logs/packet_frontend_csynth.log" 2>&1
[[ -s "$FRONTEND_RTL/synth_fetch_packet_frontend_top.v" ]] || {
    printf 'ERROR: canonical synth_fetch_packet_frontend_top RTL was not generated\n' >&2; exit 2;
}

rm -rf "$BUILD/xsim"/*
mapfile -t RTL_FILES < <(printf '%s\n' "$RTL"/*.v | sort)
(
    cd "$BUILD/xsim"
    "$XVLOG" "${RTL_FILES[@]}"
    "$XVLOG" --sv "$TB"
    "$XELAB" fetch_packet_2lane_rtl_tb -s b3i_packet_snapshot -timescale 1ns/1ps
) >"$REPORT/logs/packet_rtl_build.log" 2>&1
(
    cd "$BUILD/xsim"
    "$XSIM" b3i_packet_snapshot --runall --onerror quit \
        --log "$REPORT/logs/packet_rtl.log"
) >"$REPORT/logs/packet_rtl.stdout.log" 2>&1

rm -rf "$BUILD/xsim_frontend"
mkdir -p "$BUILD/xsim_frontend"
cp "$FRONTEND_RTL"/*.dat "$BUILD/xsim_frontend"/ 2>/dev/null || true
mapfile -t FRONTEND_RTL_FILES < <(printf '%s\n' "$FRONTEND_RTL"/*.v | sort)
(
    cd "$BUILD/xsim_frontend"
    "$XVLOG" "${FRONTEND_RTL_FILES[@]}"
    "$XVLOG" --sv "$FRONTEND_TB"
    "$XELAB" fetch_packet_frontend_rtl_tb -s b3i_packet_frontend_snapshot -timescale 1ns/1ps
) >"$REPORT/logs/packet_frontend_rtl_build.log" 2>&1
(
    cd "$BUILD/xsim_frontend"
    "$XSIM" b3i_packet_frontend_snapshot --runall --onerror quit \
        --log "$REPORT/logs/packet_frontend_rtl.log"
) >"$REPORT/logs/packet_frontend_rtl.stdout.log" 2>&1

python3 - "$REPORT/logs/packet_rtl.log" "$REPORT/logs/packet_frontend_rtl.log" \
    "$REPORT/rtl_test_matrix.csv" <<'PY'
import csv
import sys
from pathlib import Path

helper_log, frontend_log, matrix = map(Path, sys.argv[1:])
helper_text = helper_log.read_text(errors="replace")
frontend_text = frontend_log.read_text(errors="replace")
text = helper_text + "\n" + frontend_text
if "CASE_FAIL," in text:
    raise SystemExit(next(line for line in text.splitlines() if "CASE_FAIL," in line))
if "BLOCKED" in text:
    raise SystemExit("BLOCKED is not a valid B3I generated RTL status")
if "GATE5_3_B3I_FETCH_PACKET_RTL_PASS cases=" not in helper_text:
    raise SystemExit("missing B3I helper RTL completion marker")
if "GATE5_3_B3I_PACKET_FRONTEND_RTL_PASS cases=" not in frontend_text:
    raise SystemExit("missing B3I frontend integration RTL completion marker")

rows = []
for scope, log, scope_text in (
    ("fetch_packet_helper", helper_log, helper_text),
    ("fetch_packet_frontend", frontend_log, frontend_text),
):
    for line in scope_text.splitlines():
        if "CASE_PASS," not in line:
            continue
        name, requirement = line.split("CASE_PASS,", 1)[1].split(",", 1)
        rows.append((scope, name.strip(), requirement.strip(), "PASS", str(log)))

names = [row[1] for row in rows]
if len(rows) < 40:
    raise SystemExit(f"insufficient combined mandatory observable RTL cases: {len(rows)}")
if len(names) != len(set(names)):
    raise SystemExit("combined RTL case names are not unique")
scope_counts = {
    scope: sum(row[0] == scope for row in rows)
    for scope in ("fetch_packet_helper", "fetch_packet_frontend")
}
if scope_counts["fetch_packet_helper"] < 40:
    raise SystemExit("helper scope has fewer than forty observable cases")
if scope_counts["fetch_packet_frontend"] < 30:
    raise SystemExit("frontend scope has fewer than thirty observable cases")

required = {
    "cc_packet_mask3", "cc_lane_pc_order", "cc_lane0_rvc_metadata",
    "cc_lane1_rvc_metadata", "cc_lane_expansions", "cc_fetch_id_both_lanes",
    "ordinary32_mask1", "ordinary32_payload", "ordinary32_no_partial_lane",
    "c_partial_mask1", "c_partial_carry_value", "c_partial_no_lane1",
    "carry_c_mask3", "carry_c_lane0_assembly", "carry_c_lane1_metadata",
    "carry_new_carry_mask1", "carry_new_carry_value", "carry_new_carry_no_partial_lane",
    "upper_start_c_mask1", "upper_start_32_no_packet", "upper_start_32_carry",
    "illegal_lower_mask1", "illegal_lower_metadata", "illegal_lower_subtype",
    "illegal_upper_lane1", "illegal_upper_metadata", "long_lower_fault",
    "long_lower_metadata", "long_upper_fault", "long_upper_metadata",
    "carry_illegal_upper_lane1", "carry_long_upper_lane1", "access_fault_mask1",
    "access_fault_metadata", "access_fault_subtype", "carry_access_fault_attribution",
    "cause0_access_not_misaligned", "mask_domain_never2", "upper_mask_bits_unavailable",
    "high_pc_metadata", "helper_determinism_first", "helper_determinism_repeat",
}
missing = sorted(required - set(names))
if missing:
    raise SystemExit("missing mandatory helper cases: " + ", ".join(missing))

for forbidden in ("atomic", "redirect", "stale response", "stale_response"):
    if any(row[0] == "fetch_packet_helper" and forbidden in row[2].lower()
           for row in rows):
        raise SystemExit(f"integration-only claim in helper matrix: {forbidden}")

frontend_required = {
    "fe_cc_pending_mask3", "fe_cc_pending_lane_pcs", "fe_cc_fifo_lane0",
    "fe_cc_fifo_lane1", "fe_ordinary32_pending_mask1",
    "fe_c_partial_carry", "fe_c_partial_no_partial_enqueue",
    "fe_carry_c_pending_mask3", "fe_fill_depth8_two_lane",
    "fe_full_two_lane_pending", "fe_full_pending_stability",
    "fe_one_free_atomic_reject", "fe_pop_frees_second_slot_admit",
    "fe_atomic_admission_fifo_order", "fe_redirect_kills_frontend_state",
    "fe_runtime_reset_kills_frontend_state", "fe_generic_flush_kills_frontend_state",
    "fe_stale_id_drained", "fe_stale_epoch_drained", "fe_stale_address_drained",
    "fe_redirect_over_response", "fe_fault_packet_pending", "fe_pending_fault_killed",
}
missing = sorted(frontend_required - set(names))
if missing:
    raise SystemExit("missing mandatory frontend cases: " + ", ".join(missing))

with matrix.open("w", newline="") as stream:
    writer = csv.writer(stream)
    writer.writerow(("scope", "case", "asserted_requirement", "status", "evidence"))
    writer.writerows(rows)
print(
    "GATE5_3_B3I_COMBINED_RTL_MATRIX_PASS "
    f"cases={len(rows)} helper={scope_counts['fetch_packet_helper']} "
    f"frontend={scope_counts['fetch_packet_frontend']}"
)
PY
