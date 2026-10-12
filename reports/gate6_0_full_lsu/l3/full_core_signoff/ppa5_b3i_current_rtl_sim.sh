#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-/home/lab_726/boom/hls_boom}
RTL=${G6_L3_PPA5_RTL_DIR:-/home/lab_726/opencode_tmp/g6_l3r2_r5_preflight_4_4_20261009_155117/hls_project/solution/syn/verilog}
OUT=${G6_L3_PPA5_WORKLOAD_OUT:-/home/lab_726/opencode_tmp/g6_l3r5_ppa5_b3i_current_rtl_20261010}
XVLOG=${XVLOG:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xvlog}
XELAB=${XELAB:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xelab}
XSIM=${XSIM:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xsim}

PROGRAMS=(packet_two_rvc packet_rvc_rvc_long packet_carry_plus_rvc packet_atomic_backpressure packet_redirect packet_fault)

[[ "$OUT" == /home/lab_726/opencode_tmp/* ]] || {
  printf 'ERROR: output directory must remain under /home/lab_726/opencode_tmp\n' >&2
  exit 2
}
[[ -s "$RTL/boom_core_top.v" ]] || { printf 'ERROR: boom_core_top.v missing in %s\n' "$RTL" >&2; exit 2; }

mkdir -p "$OUT/logs" "$OUT/traces" "$OUT/xsim"
cp "$RTL"/*.dat "$OUT/xsim/" 2>/dev/null || true
mapfile -t RTL_FILES < <(printf '%s\n' "$RTL"/*.v | sort)

(
  cd "$OUT/xsim"
  "$XVLOG" "${RTL_FILES[@]}"
  "$XVLOG" --sv "$ROOT/rtl_tb/axis_imem_model.sv" "$ROOT/rtl_tb/axis_dmem_model.sv" \
    "$ROOT/rtl_tb/commit_trace_monitor.sv" \
    "$ROOT/tb/differential/gate5_3_b3i_rtl_harness.sv" \
    "$ROOT/tb/differential/gate5_3_b3i_rtl_tb.sv"
  "$XELAB" gate5_3_b3i_rtl_tb -s gate5_3_b3i_snapshot -timescale 1ns/1ps
) >"$OUT/logs/xsim_build.log" 2>&1

for name in "${PROGRAMS[@]}"; do
  trap_expected=0
  scenario=R0_POWER_ON_RESET
  [[ "$name" == packet_fault ]] && trap_expected=1
  [[ "$name" == packet_atomic_backpressure ]] && scenario=I3_IMEM_RESPONSE_DELAY_4
  (
    cd "$OUT/xsim"
    "$XSIM" gate5_3_b3i_snapshot --runall --onerror quit \
      --testplusarg "PROGRAM=$ROOT/tb/programs/b3i_packet/build/$name.hex" \
      --testplusarg "PROGRAM_NAME=$name" --testplusarg "SCENARIO=$scenario" \
      --testplusarg "EXPECT_TRAP=$trap_expected" --testplusarg "MAX_CYCLES=1000000" \
      --testplusarg "TRACE=$OUT/traces/$name.jsonl" \
      --log "$OUT/logs/$name.log"
  ) >"$OUT/logs/$name.stdout.log" 2>&1
  grep -q "GATE5_3_B3I_RTL_PASS program=$name" "$OUT/logs/$name.log"
done

python3 - "$OUT" "$RTL/boom_core_top.v" "${PROGRAMS[@]}" <<'PY'
import csv
import hashlib
import json
import sys
from pathlib import Path

out = Path(sys.argv[1])
rtl_top = Path(sys.argv[2])
programs = sys.argv[3:]
expected = {
    "packet_two_rvc": {8: 12, 9: 21},
    "packet_rvc_rvc_long": {8: 3, 9: 4, 10: 7},
    "packet_carry_plus_rvc": {8: 6, 9: 17, 10: 18},
    "packet_atomic_backpressure": {8: 40, 9: 19, 10: 59},
    "packet_redirect": {8: 3, 9: 7, 10: 10},
    "packet_fault": {8: 13},
}

rows = []
for name in programs:
    trace_path = out / "traces" / f"{name}.jsonl"
    records = [json.loads(line) for line in trace_path.read_text().splitlines() if line]
    commits = [record for record in records if record.get("event") == "commit"]
    dmem_reqs = [record for record in records if record.get("event") == "dmem_request"]
    imem_reqs = [record for record in records if record.get("event") == "imem_request"]
    tohosts = [record for record in records if record.get("event") == "tohost"]
    final = {record.get("rd"): int(record["rd_value"], 16)
             for record in commits if record.get("rd_valid")}
    signature = all(final.get(rd) == value for rd, value in expected[name].items())
    tohost = any(record.get("committed") and int(record["value"], 16) == 1 for record in tohosts)
    exception_commits = [record for record in commits if record.get("exception")]
    if name == "packet_fault":
        terminal = len(exception_commits) == 1 and int(exception_commits[0]["pc"], 16) == 0x80000002 and \
            int(exception_commits[0]["exception_cause"], 16) == 2 and 9 not in final and not tohost
    else:
        terminal = tohost and not exception_commits
    if name == "packet_redirect":
        terminal = terminal and not any(int(record["pc"], 16) in (0x80000004, 0x80000006)
                                        for record in commits)
    status = "PASS" if signature and terminal else "FAIL"
    rows.append((name, status, len(imem_reqs), len(dmem_reqs), len(commits),
                 "PASS" if signature else "FAIL",
                 "EXCEPTION_CAUSE_2" if name == "packet_fault" else "TOHOST_1",
                 f"traces/{name}.jsonl", f"logs/{name}.log"))

with (out / "ppa5_b3i_full_core_workload_matrix.csv").open("w", newline="") as stream:
    writer = csv.writer(stream)
    writer.writerow(("program", "status", "imem_requests", "dmem_requests", "commits",
                     "architectural_signature", "termination", "trace", "log"))
    writer.writerows(rows)

passed = sum(row[1] == "PASS" for row in rows)
(out / "ppa5_b3i_workload_provenance.txt").write_text(
    f"rtl_top={rtl_top}\n"
    f"rtl_top_sha256={hashlib.sha256(rtl_top.read_bytes()).hexdigest()}\n"
    f"programs={','.join(programs)}\n"
    f"passed={passed}/{len(rows)}\n",
    encoding="utf-8")
print(f"PPA5_B3I_FULL_CORE_RTL_PASS {passed}/{len(rows)}")
if passed != len(rows):
    raise SystemExit(1)
PY
