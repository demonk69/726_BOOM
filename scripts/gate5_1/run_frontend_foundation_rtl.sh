#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
REPORT_DIR="$ROOT/reports/gate5_1_frontend/rtl"
BUILD_DIR=${GATE5_1_RTL_BUILD_DIR:-"$ROOT/build/gate5_1_frontend_rtl"}
RTL_DIR=${GATE5_1_FRONTEND_RTL_DIR:-"$ROOT/boom_hls_gate5_1_frontend_synth_frontend_top/solution_module/syn/verilog"}
XVLOG=${XVLOG:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xvlog}
XELAB=${XELAB:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xelab}
XSIM=${XSIM:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xsim}

for tool in "$XVLOG" "$XELAB" "$XSIM"; do
  [[ -x "$tool" ]] || { printf 'ERROR: missing executable %s\n' "$tool" >&2; exit 2; }
done
[[ -s "$RTL_DIR/synth_frontend_top.v" ]] || {
  printf 'ERROR: canonical generated synth_frontend_top RTL is missing: %s\n' "$RTL_DIR" >&2
  exit 2
}

mkdir -p "$REPORT_DIR/logs" "$BUILD_DIR/xsim"
rm -rf "$BUILD_DIR/xsim"/*
mapfile -t RTL_FILES < <(printf '%s\n' "$RTL_DIR"/*.v | sort)
(
  cd "$BUILD_DIR/xsim"
  "$XVLOG" "${RTL_FILES[@]}"
  "$XVLOG" --sv "$ROOT/rtl_tb/frontend_foundation_rtl_tb.sv"
  "$XELAB" frontend_foundation_rtl_tb -s frontend_foundation_rtl_snapshot -timescale 1ns/1ps
) > "$REPORT_DIR/logs/xsim_build.log" 2>&1
(
  cd "$BUILD_DIR/xsim"
  "$XSIM" frontend_foundation_rtl_snapshot --runall --onerror quit \
    --log "$REPORT_DIR/logs/xsim.log"
) > "$REPORT_DIR/logs/xsim.stdout.log" 2>&1

grep -q 'FRONTEND_RTL_PARTIAL_PASS' "$REPORT_DIR/logs/xsim.log"

python3 - "$REPORT_DIR/logs/xsim.log" "$REPORT_DIR/rtl_test_matrix.csv" \
  "$REPORT_DIR/request_cycle_trace.csv" <<'PY'
import csv
import re
import sys
from pathlib import Path

log_path, matrix_path, trace_path = map(Path, sys.argv[1:])
lines = log_path.read_text(errors="replace").splitlines()
passes = []
requests = []
responses = []
for line in lines:
    if "CASE_PASS," in line:
        name, requirement = line.split("CASE_PASS,", 1)[1].split(",", 1)
        passes.append((name.strip(), requirement.strip(), "PASS", str(log_path)))
    elif "REQUEST_TRACE," in line:
        fields = line.split("REQUEST_TRACE,", 1)[1].split(",")
        requests.append(fields)
    elif "RESPONSE_TRACE," in line:
        fields = line.split("RESPONSE_TRACE,", 1)[1].split(",")
        responses.append(fields)

blocked = [
    ("redirect_priority", "reset > architectural > branch > generic flush > response"),
    ("redirect_over_response", "redirect consumes old response without publish"),
    ("architectural_ownership", "ROB index and allocation ID ownership validation"),
    ("redirect_epoch", "redirect increments epoch and old epoch cannot reactivate"),
    ("decode_hold", "held instruction remains stable under decode stall"),
    ("stale_drain_during_decode_stall", "stale response drains without overwriting held entry"),
    ("fault_to_decode", "instruction access fault PC and cause propagate to Decode"),
    ("fault_hold", "fault remains stable under Decode backpressure and publishes once"),
    ("misaligned_arch_target", "misaligned architectural target faults without masking"),
    ("misaligned_branch_target", "misaligned branch target faults without masking"),
    ("misaligned_generic_flush_target", "generic flush target alignment behavior"),
    ("runtime_reset", "runtime reset clears held and pending frontend state"),
    ("reset_priority", "runtime reset wins redirects and responses"),
]
rows = passes + [(name, requirement, "BLOCKED", "synth_frontend_top port not exposed")
                 for name, requirement in blocked]
with matrix_path.open("w", newline="") as stream:
    writer = csv.writer(stream)
    writer.writerow(("case", "asserted_requirement", "status", "evidence"))
    writer.writerows(rows)

with trace_path.open("w", newline="") as stream:
    writer = csv.writer(stream)
    writer.writerow(("request_index", "request_accept_cycle", "address", "fetch_id", "epoch",
                     "matching_response_accept_cycle", "next_request_interval"))
    for index, fields in enumerate(requests):
        _, cycle, address, fetch_id, epoch = fields
        next_cycle = int(requests[index + 1][1]) if index + 1 < len(requests) else 1 << 60
        matching_cycles = [int(f[1]) for f in responses
                           if (f[2], f[3], f[4]) == (address, fetch_id, epoch)
                           and int(cycle) < int(f[1]) < next_cycle]
        response_cycle = min(matching_cycles) if matching_cycles else ""
        interval = ""
        if index + 1 < len(requests):
            interval = int(requests[index + 1][1]) - int(cycle)
        writer.writerow((index, cycle, address, fetch_id, epoch, response_cycle, interval))

if len(passes) < 18:
    raise SystemExit(f"expected at least 18 observable generated-RTL PASS checks, found {len(passes)}")
PY

printf 'Gate 5.1 observable focused RTL subset passed; unobservable requirements remain BLOCKED.\n'
