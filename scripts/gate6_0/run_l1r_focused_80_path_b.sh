#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)"}
FULL_WORK=${G6_L1R80_FULL_CORE_WORK:-/home/lab_726/opencode_tmp/g6_l1_path_b_full_core_retry}
REPORT=${G6_L1R80_REPORT:-$ROOT/reports/gate6_0_full_lsu/l1/focused_rtl_redesign}
VITIS_HLS=${VITIS_HLS:-/home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls}
IMAGE_TIMEOUT=${PATH_B_HLS_TIMEOUT_SEC:-1800}
SOFT_BYTES=${PATH_B_WORKSPACE_SOFT_LIMIT_BYTES:-12884901888}
HARD_BYTES=${PATH_B_WORKSPACE_HARD_LIMIT_BYTES:-17179869184}
MEM_RESERVE_BYTES=${PATH_B_MEM_RESERVE_BYTES:-17179869184}
MEM_WARNING_BYTES=${PATH_B_MEM_WARNING_BYTES:-25769803776}
RSS_GROWTH_DANGER_BYTES=${PATH_B_RSS_GROWTH_DANGER_BYTES:-2147483648}
TRACE=$REPORT/path_b_full_core_resource_trace.csv
RESOURCE=$REPORT/focused_rtl_80_resource_summary.csv
EXHAUSTED=$REPORT/path_b_full_core_retry_exhausted.txt
PROJECT=$FULL_WORK/boom_hls_g6_l1r_default_8_8_boom_core_top
RTL=$PROJECT/solution_module/syn/verilog
LOG=$REPORT/logs/path_b_full_core_retry_csynth.log
TIME_LOG=$REPORT/logs/path_b_full_core_retry_time.log

[[ "${ONE_BOUNDED_PATH_B_FULL_CORE_RETRY:-false}" == true ]] || {
    printf 'ERROR: ONE_BOUNDED_PATH_B_FULL_CORE_RETRY=true is required\n' >&2
    exit 2
}
[[ "$FULL_WORK" == /home/lab_726/opencode_tmp/g6_l1_path_b_full_core_retry ]] || {
    printf 'ERROR: bounded retry workspace must be /home/lab_726/opencode_tmp/g6_l1_path_b_full_core_retry\n' >&2
    exit 2
}
[[ "$IMAGE_TIMEOUT" == 1800 && "$SOFT_BYTES" == 12884901888 && "$HARD_BYTES" == 17179869184 ]] || {
    printf 'ERROR: bounded retry policy must remain 12/16 GiB and 1800 seconds\n' >&2
    exit 2
}
[[ ! -e "$EXHAUSTED" ]] || {
    printf 'ERROR: one-time PATH_B retry already exhausted\n' >&2
    exit 3
}
for tool in "$VITIS_HLS" setsid timeout /usr/bin/time python3; do
    command -v "$tool" >/dev/null 2>&1 || { printf 'ERROR: missing tool %s\n' "$tool" >&2; exit 127; }
done
mkdir -p -- "$FULL_WORK" "$REPORT/logs"

python3 - "$ROOT" <<'PY'
import hashlib, pathlib, subprocess, sys
root=pathlib.Path(sys.argv[1])
expected={
    root/'src/boom_core_merged.cpp':'76d78e9052344a0b1e06c0e723c473d37e0b2dd7679a2ab3b8fbbeed9f1dff9c',
    root/'src/boom_all.cpp':'d6f885632ddd445729adda8148ea256e67683ccc8e7f2b10c9951e915d92c76c',
}
for path, digest in expected.items():
    if hashlib.sha256(path.read_bytes()).hexdigest()!=digest:
        raise SystemExit(f'protected product hash mismatch: {path}')
text=subprocess.check_output(['ps','-eo','comm='], text=True)
active=[name for name in text.split() if name in {'vitis_hls','vivado','xsim','xelab','xvlog'}]
if active: raise SystemExit(f'active EDA processes before retry: {active}')
available=int(subprocess.check_output(['df','-B1','--output=avail',str(root)], text=True).splitlines()[1])
if available < 30*1024**3: raise SystemExit(f'insufficient /home capacity: {available}')
config=(root/'include/boom_config.hpp').read_text()
normalized=' '.join(config.split())
if ('#ifndef LQ_DEPTH #define LQ_DEPTH 8 #endif' not in normalized or
    '#ifndef SQ_DEPTH #define SQ_DEPTH 8 #endif' not in normalized or
    '#define LDQ_DEPTH LQ_DEPTH' not in normalized or
    '#define STQ_DEPTH SQ_DEPTH' not in normalized):
    raise SystemExit('queue-depth configuration changed')
print(f'ACTIVE_EDA_PROCESSES_BEFORE_RETRY=0')
print(f'HOME_AVAILABLE_GB={available//1024**3}')
PY

printf 'timestamp,elapsed_sec,workspace_bytes,rss_bytes,mem_available_bytes,stage\n' >"$TRACE"
start=$(date +%s)
peak_workspace=0; peak_rss=0; min_available=0; review=0
termination=NONE; previous_rss=0
setsid timeout --signal=TERM --kill-after=30s "$IMAGE_TIMEOUT" /usr/bin/time -v \
    -o "$TIME_LOG" env HLS_BOOM_ROOT="$ROOT" G6_L1R80_FULL_CORE_WORK="$FULL_WORK" \
    FPGA_PART=xczu7ev-ffvc1156-2-e CLOCK_PERIOD=10 \
    "$VITIS_HLS" -f "$ROOT/scripts/gate6_0/l1r_focused_80_full_core_csynth.tcl" \
    >"$LOG" 2>&1 &
pid=$!
while kill -0 "$pid" 2>/dev/null; do
    now=$(date +%s); elapsed=$((now-start))
    workspace_kb=$(du -sk -- "$FULL_WORK" 2>/dev/null | awk '{print $1}')
    workspace_bytes=$((workspace_kb * 1024))
    rss_kb=$(ps -eo pgid=,rss= | awk -v group="$pid" '$1 == group {sum += $2} END {print sum + 0}')
    rss_bytes=$((rss_kb * 1024))
    available_kb=$(awk '/^MemAvailable:/ {print $2}' /proc/meminfo)
    available_bytes=$((available_kb * 1024))
    stage=$(python3 - "$LOG" <<'PY'
import pathlib, sys
p=pathlib.Path(sys.argv[1])
stage='STARTUP'
if p.exists():
    text=p.read_text(errors='replace')[-200000:]
    for token,name in [('Generating Verilog RTL','RTL_GENERATION'),('Finished Creating RTL model','RTL_MODEL'),('Starting scheduling','SCHEDULING'),('Starting hardware synthesis','HARDWARE_SYNTHESIS'),('Starting code transformations','TRANSFORMS'),('Analyzing design file','SOURCE_ANALYSIS')]:
        if token in text: stage=name; break
print(stage)
PY
)
    ((workspace_bytes > peak_workspace)) && peak_workspace=$workspace_bytes
    ((rss_bytes > peak_rss)) && peak_rss=$rss_bytes
    if ((min_available == 0 || available_bytes < min_available)); then min_available=$available_bytes; fi
    ((workspace_bytes >= SOFT_BYTES)) && review=1
    printf '%s,%s,%s,%s,%s,%s\n' "$(date --iso-8601=seconds)" "$elapsed" \
        "$workspace_bytes" "$rss_bytes" "$available_bytes" "$stage" >>"$TRACE"
    if ((workspace_bytes >= HARD_BYTES)); then termination=WORKSPACE_HARD_LIMIT; break; fi
    if ((available_bytes < MEM_RESERVE_BYTES)); then termination=MEMORY_SAFETY_RESERVE; break; fi
    if ((available_bytes < MEM_WARNING_BYTES && rss_bytes - previous_rss >= RSS_GROWTH_DANGER_BYTES)); then
        termination=MEMORY_RSS_GROWTH_DANGER; break
    fi
    previous_rss=$rss_bytes
    sleep 30
done
if [[ "$termination" != NONE ]]; then
    kill -TERM -- "-$pid" 2>/dev/null || true
    sleep 5
    kill -KILL -- "-$pid" 2>/dev/null || true
fi
set +e; wait "$pid"; rc=$?; set -e
end=$(date +%s); runtime=$((end-start))
if [[ $rc -eq 124 || $rc -eq 137 ]] && ((runtime >= IMAGE_TIMEOUT)); then termination=TIMEOUT; fi
if [[ -s "$TIME_LOG" ]]; then
    final_rss_kb=$(awk -F: '/Maximum resident set size/ {gsub(/^[[:space:]]+/, "", $2); print $2}' "$TIME_LOG")
    final_rss_kb=${final_rss_kb:-0}; final_rss=$((final_rss_kb * 1024))
    ((final_rss > peak_rss)) && peak_rss=$final_rss
fi
workspace_kb=$(du -sk -- "$FULL_WORK" 2>/dev/null | awk '{print $1}')
workspace_bytes=$((workspace_kb * 1024)); ((workspace_bytes > peak_workspace)) && peak_workspace=$workspace_bytes

rtl_generated=false
if [[ $rc -eq 0 && -s "$RTL/boom_core_top.v" ]] && grep -q 'Finished Generating all RTL models' "$LOG"; then
    rtl_generated=true
    termination=NONE
fi
rtl_hash=NONE
if [[ "$rtl_generated" == true ]]; then
    rtl_hash=$(python3 - "$RTL" <<'PY'
import hashlib, sys
from pathlib import Path
root=Path(sys.argv[1]); digest=hashlib.sha256()
for path in sorted(root.glob('*.v'))+sorted(root.glob('*.dat')):
    digest.update(path.name.encode()+b'\0')
    digest.update(bytes.fromhex(hashlib.sha256(path.read_bytes()).hexdigest()))
print(digest.hexdigest())
PY
)
fi
printf 'PATH_B_HLS_RUNTIME_SEC=%s\nPATH_B_PEAK_WORKSPACE_BYTES=%s\nPATH_B_PEAK_RSS_BYTES=%s\nPATH_B_MIN_MEM_AVAILABLE_BYTES=%s\nPATH_B_RESOURCE_REVIEW=%s\nPATH_B_TERMINATION_REASON=%s\nPATH_B_FULL_CORE_RTL_GENERATED=%s\nPATH_B_FULL_CORE_RTL_HASH=%s\n' \
    "$runtime" "$peak_workspace" "$peak_rss" "$min_available" "$review" \
    "$termination" "$rtl_generated" "$rtl_hash" >"$REPORT/path_b_full_core_retry_outcome.txt"
if ! grep -q '^7,full_core_bounded_retry,boom_core_top,' "$RESOURCE"; then
    printf '7,full_core_bounded_retry,boom_core_top,%s,%s,%s,%s,%s,%s,%s,%s\n' \
        "$IMAGE_TIMEOUT" "$runtime" "$((peak_workspace/1024))" "$((peak_rss/1024))" \
        "$review" "$([[ "$termination" == WORKSPACE_HARD_LIMIT ]] && printf 1 || printf 0)" \
        "$rc" "$rtl_hash" >>"$RESOURCE"
fi
if [[ "$rtl_generated" != true ]]; then
    printf 'PATH_B_FULL_CORE_RETRY_EXHAUSTED=true\nPATH_B_FAILURE_CLASS=%s\n' "$termination" >"$EXHAUSTED"
    printf 'ERROR: bounded PATH_B retry failed: rc=%s reason=%s\n' "$rc" "$termination" >&2
    exit 1
fi

printf 'PATH_B_BOUNDED_RETRY=PASS\nPATH_B_FULL_CORE_RTL_GENERATED=true\nPATH_B_FULL_CORE_RTL_HASH=%s\n' "$rtl_hash"
