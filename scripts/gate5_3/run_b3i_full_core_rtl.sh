#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
REPORT="$ROOT/reports/gate5_3_fetch_buffer/b3i/rtl"
BUILD_ROOT="$ROOT/build/gate5_3_fetch_buffer/b3i/rtl"
VITIS_HLS_BIN=${VITIS_HLS:-/home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls}
XVLOG_BIN=${XVLOG:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xvlog}
XELAB_BIN=${XELAB:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xelab}
XSIM_BIN=${XSIM:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xsim}
PROGRAMS=(packet_two_rvc packet_rvc_rvc_long packet_carry_plus_rvc packet_atomic_backpressure packet_redirect packet_fault)

mapfile -t CPP_INPUTS < <(printf '%s\n' "$ROOT"/src/*.cpp | sort)
mapfile -t HEADER_INPUTS < <(printf '%s\n' "$ROOT"/include/*.hpp | sort)
INPUTS=()
for source in "${CPP_INPUTS[@]}"; do
    case "$source" in
        */src/boom_all.cpp|*/src/boom_core_merged.cpp) continue ;;
    esac
    INPUTS+=("$source")
done
INPUTS+=("${HEADER_INPUTS[@]}")

hash_inputs() {
    python3 - "$ROOT" "${INPUTS[@]}" <<'PY'
import hashlib
import sys
from pathlib import Path
root = Path(sys.argv[1])
digest = hashlib.sha256()
for item in sys.argv[2:]:
    path = Path(item)
    digest.update(str(path.relative_to(root)).encode("utf-8"))
    digest.update(b"\0")
    digest.update(hashlib.sha256(path.read_bytes()).digest())
print(digest.hexdigest())
PY
}

SOURCE_HASH=$(hash_inputs)
HASH_TAG=${SOURCE_HASH:0:16}
BUILD="$BUILD_ROOT/$HASH_TAG"
HLS_PROJECT="$BUILD/boom_core_top_hls"
RTL=${GATE5_3_B3I_PREBUILT_RTL:-"$HLS_PROJECT/solution_b3i/syn/verilog"}
XSIM_BUILD="$BUILD/xsim"
mkdir -p "$REPORT/logs" "$REPORT/traces" "$BUILD"

python3 - "$ROOT" "$REPORT/source_freshness_manifest.csv" "$SOURCE_HASH" "${INPUTS[@]}" <<'PY'
import csv
import hashlib
import sys
from pathlib import Path
root, output, aggregate = Path(sys.argv[1]), Path(sys.argv[2]), sys.argv[3]
with output.open("w", newline="") as stream:
    writer = csv.writer(stream)
    writer.writerow(("path", "scope", "sha256"))
    for item in sys.argv[4:]:
        path = Path(item)
        writer.writerow((path.relative_to(root), "INCLUDED", hashlib.sha256(path.read_bytes()).hexdigest()))
    writer.writerow(("src/boom_all.cpp", "EXCLUDED_NON_MODULAR", ""))
    writer.writerow(("src/boom_core_merged.cpp", "EXCLUDED_GENERATED", ""))
    writer.writerow(("AGGREGATE", "CURRENT_SOURCE_HEADER_HASH", aggregate))
PY

"$ROOT/scripts/gate5_3/build_b3i_programs.sh" >"$REPORT/logs/program_build.log" 2>&1
"$ROOT/scripts/generate_merged.sh" >"$REPORT/logs/generate_merged.log" 2>&1
if [[ -z "${GATE5_3_B3I_PREBUILT_RTL:-}" ]]; then
    GATE5_3_B3I_HLS_PROJECT="$HLS_PROJECT" "$VITIS_HLS_BIN" \
        -f "$ROOT/scripts/gate5_3/b3i_core_csynth.tcl" >"$REPORT/logs/csynth.log" 2>&1
    printf '%s\n' "$SOURCE_HASH" >"$RTL/.b3i_source_hash"
else
    printf 'Using explicitly supplied B3I RTL: %s\n' "$RTL" >"$REPORT/logs/csynth.log"
fi

RTL_TOP="$RTL/boom_core_top.v"
[[ -s "$RTL_TOP" ]] || { printf 'B3I boom_core_top RTL unavailable at %s\n' "$RTL_TOP" >&2; exit 3; }
[[ -s "$RTL/.b3i_source_hash" ]] || { printf '%s\n' 'B3I RTL has no source-hash freshness stamp' >&2; exit 3; }
[[ "$(<"$RTL/.b3i_source_hash")" == "$SOURCE_HASH" ]] || {
    printf '%s\n' 'B3I RTL source-hash stamp does not match current source/header inputs' >&2
    exit 3
}
[[ "$(hash_inputs)" == "$SOURCE_HASH" ]] || {
    printf '%s\n' 'source/header inputs changed during B3I RTL generation' >&2
    exit 3
}
for source in "${INPUTS[@]}"; do
    [[ ! "$source" -nt "$RTL_TOP" ]] || {
        printf 'B3I RTL is older than current input: %s\n' "$source" >&2
        exit 3
    }
done

rm -rf "$XSIM_BUILD"
mkdir -p "$XSIM_BUILD"
cp "$RTL"/*.dat "$XSIM_BUILD"/ 2>/dev/null || true
mapfile -t RTL_FILES < <(printf '%s\n' "$RTL"/*.v | sort)
(
    cd "$XSIM_BUILD"
    "$XVLOG_BIN" "${RTL_FILES[@]}"
    "$XVLOG_BIN" --sv "$ROOT/rtl_tb/axis_imem_model.sv" "$ROOT/rtl_tb/axis_dmem_model.sv" \
        "$ROOT/rtl_tb/commit_trace_monitor.sv" \
        "$ROOT/tb/differential/gate5_3_b3i_rtl_harness.sv" \
        "$ROOT/tb/differential/gate5_3_b3i_rtl_tb.sv"
    "$XELAB_BIN" gate5_3_b3i_rtl_tb -s gate5_3_b3i_snapshot -timescale 1ns/1ps
) >"$REPORT/logs/xsim_build.log" 2>&1

for name in "${PROGRAMS[@]}"; do
    trap=0
    scenario=R0_POWER_ON_RESET
    [[ "$name" == packet_fault ]] && trap=1
    [[ "$name" == packet_atomic_backpressure ]] && scenario=I3_IMEM_RESPONSE_DELAY_4
    (
        cd "$XSIM_BUILD"
        "$XSIM_BIN" gate5_3_b3i_snapshot --runall --onerror quit \
            --testplusarg "PROGRAM=$ROOT/tb/programs/b3i_packet/build/$name.hex" \
            --testplusarg "PROGRAM_NAME=$name" --testplusarg "SCENARIO=$scenario" \
            --testplusarg "EXPECT_TRAP=$trap" --testplusarg "MAX_CYCLES=1000000" \
            --testplusarg "TRACE=$REPORT/traces/$name.jsonl" \
            --log "$REPORT/logs/$name.log"
    ) >"$REPORT/logs/$name.stdout.log" 2>&1
    grep -q "GATE5_3_B3I_RTL_PASS program=$name" "$REPORT/logs/$name.log"
done

python3 - "$REPORT" "$SOURCE_HASH" "$RTL_TOP" "${PROGRAMS[@]}" <<'PY'
import csv
import json
import sys
from pathlib import Path
report = Path(sys.argv[1])
source_hash, rtl_top = sys.argv[2:4]
programs = sys.argv[4:]
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
    records = [json.loads(line) for line in (report / "traces" / f"{name}.jsonl").read_text().splitlines() if line]
    commits = [record for record in records if record.get("event") == "commit"]
    final = {record.get("rd"): int(record["rd_value"], 16)
             for record in commits if record.get("rd_valid")}
    signature = all(final.get(rd) == value for rd, value in expected[name].items())
    tohost = any(record.get("event") == "tohost" and record.get("committed") and
                 int(record["value"], 16) == 1 for record in records)
    if name == "packet_fault":
        faults = [record for record in commits if record.get("exception")]
        terminal = len(faults) == 1 and int(faults[0]["pc"], 16) == 0x80000002 and \
            int(faults[0]["exception_cause"], 16) == 2 and 9 not in final and not tohost
    else:
        terminal = tohost and not any(record.get("exception") for record in commits)
    if name == "packet_redirect":
        terminal = terminal and not any(int(record["pc"], 16) in (0x80000004, 0x80000006)
                                        for record in commits)
    status = "PASS" if signature and terminal else "FAIL"
    rows.append((name, status, len(commits), "PASS" if signature else "FAIL",
                 "TRAP_CAUSE_2_PC_80000002" if name == "packet_fault" else "TOHOST_1",
                 f"traces/{name}.jsonl"))
with (report / "full_core_rtl_matrix.csv").open("w", newline="") as stream:
    writer = csv.writer(stream)
    writer.writerow(("program", "status", "commits", "signature", "termination", "trace"))
    writer.writerows(rows)
passed = sum(row[1] == "PASS" for row in rows)
if passed != 6:
    raise SystemExit(f"Gate 5.3 B3I generated full-core RTL failed: {passed}/6")
(report / "generation_provenance.md").write_text(
    "# Gate 5.3 B3I Full-Core RTL Provenance\n\n"
    f"- Current modular source/header SHA-256: `{source_hash}`.\n"
    f"- Hash-keyed generated RTL top: `{rtl_top}`.\n"
    "- Freshness: source-hash stamp matched and no included input was newer than RTL.\n"
    "- Programs: six B3I-only assembled images; Gate 5.2 lists were not reused or changed.\n"
    "- `packet_fault`: architectural illegal-compressed trap, cause 2 at PC `0x80000002`; no fetch-fault bus injection.\n"
    f"- Generated full-core RTL result: `{passed}/6 PASS`.\n", encoding="utf-8")
print(f"GATE5_3_B3I_GENERATED_FULL_CORE_RTL_PASS {passed}/6 source_hash={source_hash}")
PY
