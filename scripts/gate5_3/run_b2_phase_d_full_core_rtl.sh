#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
REPORT="$ROOT/reports/gate5_3_fetch_buffer/b2/phase_d"
BUILD_ROOT="$ROOT/build/gate5_3_fetch_buffer/b2/phase_d"
VITIS_HLS_BIN=${VITIS_HLS:-/home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls}
XVLOG_BIN=${XVLOG:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xvlog}
XELAB_BIN=${XELAB:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xelab}
XSIM_BIN=${XSIM:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xsim}
PROGRAMS=(rvc_addi rvc_load_store rvc_branch rvc_jump rvc_word_ops rvc_mixed_16_32 rvc_cross_boundary rvc_rv64m_mix rvc_redirect_halfword rvc_tohost rvc_decode_gaps)

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
import hashlib, sys
from pathlib import Path
root = Path(sys.argv[1])
digest = hashlib.sha256()
for name in sys.argv[2:]:
    path = Path(name)
    digest.update(str(path.relative_to(root)).encode())
    digest.update(b'\0')
    digest.update(hashlib.sha256(path.read_bytes()).digest())
print(digest.hexdigest())
PY
}

SOURCE_HASH=$(hash_inputs)
HASH_TAG=${SOURCE_HASH:0:16}
BUILD="$BUILD_ROOT/$HASH_TAG"
HLS_PROJECT="$BUILD/boom_core_top_hls"
RTL=${GATE5_3_B2_D_PREBUILT_RTL:-"$HLS_PROJECT/solution_phase_d/syn/verilog"}
XSIM_BUILD="$BUILD/xsim"
mkdir -p "$REPORT/logs" "$REPORT/traces" "$BUILD" "$XSIM_BUILD"

python3 - "$ROOT" "$REPORT/source_freshness_manifest.csv" "$SOURCE_HASH" "${INPUTS[@]}" <<'PY'
import csv, hashlib, sys
from pathlib import Path
root, output, aggregate = Path(sys.argv[1]), Path(sys.argv[2]), sys.argv[3]
included = [Path(x) for x in sys.argv[4:]]
with output.open('w', newline='') as f:
    w = csv.writer(f)
    w.writerow(('path', 'kind', 'freshness_scope', 'sha256'))
    for path in included:
        kind = 'modular_source' if path.suffix == '.cpp' else 'header'
        w.writerow((path.relative_to(root), kind, 'INCLUDED', hashlib.sha256(path.read_bytes()).hexdigest()))
    w.writerow(('src/boom_all.cpp', 'aggregate_source', 'EXCLUDED_NON_MODULAR', ''))
    w.writerow(('src/boom_core_merged.cpp', 'generated_source', 'EXCLUDED_GENERATED', ''))
    w.writerow(('AGGREGATE', 'source_header_hash', 'INCLUDED', aggregate))
PY

bash "$ROOT/scripts/gate5_2/build_rvc_programs.sh" >"$REPORT/logs/program_build.log" 2>&1
bash "$ROOT/scripts/generate_merged.sh" >"$REPORT/logs/generate_merged.log" 2>&1
MERGED_HASH=$(sha256sum "$ROOT/src/boom_core_merged.cpp" | cut -d' ' -f1)
if [[ -z "${GATE5_3_B2_D_PREBUILT_RTL:-}" ]]; then
    GATE5_3_B2_D_HLS_PROJECT="$HLS_PROJECT" "$VITIS_HLS_BIN" \
        -f "$ROOT/scripts/gate5_3/b2_phase_d_core_csynth.tcl" \
        >"$REPORT/logs/boom_core_top_csynth.log" 2>&1
    if grep -q 'ERROR:' "$REPORT/logs/boom_core_top_csynth.log"; then
        printf '%s\n' 'Vitis HLS reported an error during phase D boom_core_top generation' >&2
        exit 3
    fi
else
    printf 'Using reviewed current-source boom_core_top RTL: %s\n' "$RTL" \
        >"$REPORT/logs/boom_core_top_csynth.log"
fi

RTL_TOP="$RTL/boom_core_top.v"
[[ -s "$RTL_TOP" ]] || { printf 'phase D boom_core_top RTL unavailable at %s\n' "$RTL_TOP" >&2; exit 3; }
[[ "$(hash_inputs)" == "$SOURCE_HASH" ]] || {
    printf '%s\n' 'modular source/header inputs changed during phase D generation' >&2
    exit 3
}
for source in "${INPUTS[@]}"; do
    [[ ! "$source" -nt "$RTL_TOP" ]] || {
        printf 'phase D boom_core_top RTL is older than source/header input: %s\n' "$source" >&2
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
        "$ROOT/rtl_tb/commit_trace_monitor.sv" "$ROOT/tb/differential/gate5_2_r2_rtl_harness.sv" \
        "$ROOT/rtl_tb/boom_core_rtl_tb.sv"
    "$XELAB_BIN" boom_core_rtl_tb -s gate5_3_b2_phase_d_snapshot -timescale 1ns/1ps
) >"$REPORT/logs/xsim_build.log" 2>&1

for name in "${PROGRAMS[@]}"; do
    (
        cd "$XSIM_BUILD"
        "$XSIM_BIN" gate5_3_b2_phase_d_snapshot --runall --onerror quit \
            --testplusarg "PROGRAM=$ROOT/tb/programs/rvc_fetch/build/$name.hex" \
            --testplusarg "PROGRAM_NAME=$name" --testplusarg "SCENARIO=R0_POWER_ON_RESET" \
            --testplusarg "MAX_CYCLES=1000000" \
            --testplusarg "TRACE=$REPORT/traces/$name.jsonl" \
            --log "$REPORT/logs/$name.log"
    ) >"$REPORT/logs/$name.stdout.log" 2>&1
    grep -q 'GATE3_8_PASS scenario=R0_POWER_ON_RESET' "$REPORT/logs/$name.log"
done

python3 - "$REPORT" "$SOURCE_HASH" "$HASH_TAG" "$RTL_TOP" "$MERGED_HASH" "${PROGRAMS[@]}" <<'PY'
import csv, json, sys
from pathlib import Path
report = Path(sys.argv[1])
source_hash, hash_tag, rtl_top, merged_hash = sys.argv[2:6]
programs = sys.argv[6:]
expected = {
 "rvc_addi": {8:12,9:15}, "rvc_load_store": {9:46,10:46,11:47},
 "rvc_branch": {9:9,10:13}, "rvc_jump": {8:7,9:18},
 "rvc_word_ops": {8:12,10:0xfffffffffffffff8},
 "rvc_mixed_16_32": {8:19,9:13,10:30}, "rvc_cross_boundary": {8:10,9:21,10:31},
 "rvc_rv64m_mix": {10:42,11:8}, "rvc_redirect_halfword": {8:23,9:27},
 "rvc_tohost": {8:15,9:31}, "rvc_decode_gaps": {8:0xffffffff,9:2}}
rows = []
for name in programs:
    records = [json.loads(x) for x in (report/'traces'/f'{name}.jsonl').read_text().splitlines() if x]
    commits = [x for x in records if x.get('event') == 'commit']
    final = {x.get('rd'): int(x['rd_value'], 16) for x in commits if x.get('rd_valid')}
    checks = [f'x{rd}=0x{value:016x}' for rd, value in expected[name].items()]
    signature = all(final.get(rd) == value for rd, value in expected[name].items())
    tohost = any(x.get('event') == 'tohost' and x.get('committed') and
                 int(x['address'], 16) == 0x80000080 and int(x['value'], 16) == 1 for x in records)
    status = 'PASS' if signature and tohost else 'FAIL'
    rows.append((name, status, len(commits), 'PASS' if signature else 'FAIL', ';'.join(checks),
                 'PASS' if tohost else 'FAIL', f'traces/{name}.jsonl'))
with (report/'full_core_rtl_matrix.csv').open('w', newline='') as f:
    w = csv.writer(f)
    w.writerow(('program','status','commits','signature_status','expected_signature','tohost','trace'))
    w.writerows(rows)
passed = sum(row[1] == 'PASS' for row in rows)
if passed != 11:
    raise SystemExit(f'Gate 5.3 B2 phase D full-core RTL signature/tohost check failed: {passed}/11')
(report/'generation_provenance.md').write_text(
    '# Gate 5.3 B2 Phase D Generation Provenance\n\n'
    f'- Phase: `D`, generated full-core RTL verification prerequisite only.\n'
    f'- Acceptance claim: not phase F canonical acceptance.\n'
    f'- Synthesized top: `boom_core_top` only; raw `boom_core_step` was not synthesized.\n'
    f'- Modular source/header SHA-256: `{source_hash}`.\n'
    f'- Current-hash build key: `{hash_tag}`.\n'
    f'- Generated merged-source SHA-256: `{merged_hash}`.\n'
    f'- Generated RTL top: `{rtl_top}`.\n'
    f'- Freshness: PASS; all included modular source/header hashes remained stable and no input was newer than RTL.\n'
    f'- Explicit exclusions: `src/boom_all.cpp` (non-modular aggregate), `src/boom_core_merged.cpp` (generated input).\n'
    f'- XSim mixed-RVC result: `{passed}/11 PASS`; every row passed signature and committed tohost checks.\n')
(report/'b2_scenario_mapping.md').write_text(
    '# Gate 5.3 B2 Scenario Evidence Mapping\n\n'
    '| Named scenario | Honest existing/new evidence | Scope |\n'
    '|---|---|---|\n'
    '| `buffer_decode_stall` | `../rtl_test_matrix.csv`: `decode_stall_retains_head`, `stall_release`; `../decoupling_metrics.csv` | Focused generated RTL plus native decoupling |\n'
    '| `buffer_redirect` | `../rtl_test_matrix.csv`: `redirect_flush`, `redirect_no_old_pop` | Focused generated RTL |\n'
    '| `buffer_rvc_mix` | `full_core_rtl_matrix.csv`: mixed RVC 11-program XSim matrix; `../rtl_test_matrix.csv`: RVC/cross-word cases | Generated full-core RTL plus focused generated RTL |\n'
    '| `buffer_fault_flush` | `../rtl_test_matrix.csv`: `fault_entry`, `fault_cause`, `fault_flush_no_late_exception` | Focused generated RTL |\n\n'
    'No new full-core programs are claimed for these names. The 11 reused programs retain their original Gate 5.2 identities.\n')
print(f'GATE5_3_B2_PHASE_D_GENERATED_FULL_CORE_RTL_PASS {passed}/11 source_hash={source_hash} rtl={rtl_top}')
PY
