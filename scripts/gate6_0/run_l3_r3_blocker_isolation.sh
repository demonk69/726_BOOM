#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)"}
R2_BUILD_ROOT=${G6_L3_R2_BUILD_ROOT:-/home/lab_726/opencode_tmp/g6_l3r2_r2}
BUILD_ROOT=${G6_L3_R3_BUILD_ROOT:-/home/lab_726/opencode_tmp/g6_l3r2_r3}
REPORT=${G6_L3_R3_REPORT:-$ROOT/reports/gate6_0_full_lsu/l3/rtl_verification/r3_blocker_isolation}
XVLOG=${XVLOG:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xvlog}
XELAB=${XELAB:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xelab}
XSIM=${XSIM:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xsim}
CONFIGS=${G6_L3_R3_CONFIGS:-"4_4"}
TB="$ROOT/rtl_tb/gate6_0/g6_l3_r3_blocker_isolation_tb.sv"

[[ "$BUILD_ROOT" == /home/lab_726/opencode_tmp/* ]] || {
    printf 'ERROR: build root must remain under /home/lab_726/opencode_tmp\n' >&2
    exit 2
}
[[ -s "$TB" ]] || { printf 'ERROR: R3 testbench missing: %s\n' "$TB" >&2; exit 2; }

mkdir -p -- "$BUILD_ROOT" "$REPORT/logs"

req_trace_tmp="$REPORT/request_backpressure_trace.csv.tmp"
fwd_trace_tmp="$REPORT/store_forwarding_trace.csv.tmp"
flush_trace_tmp="$REPORT/flush_recovery_trace.csv.tmp"
results_tmp="$REPORT/r3_results.raw.tmp"
: >"$results_tmp"
first_req=1
first_fwd=1
first_flush=1

for config in $CONFIGS; do
    lq_depth=${config%_*}
    sq_depth=${config#*_}
    rtl_dir="$R2_BUILD_ROOT/rtl_generation/$config/hls_project/solution/syn/verilog"
    sim_work="$BUILD_ROOT/rtl_isolation/$config"
    build_log="$REPORT/logs/${config}_build.log"
    sim_log="$REPORT/logs/${config}_simulation.log"
    sim_stdout="$REPORT/logs/${config}_simulation.stdout.log"

    [[ -s "$rtl_dir/boom_core_top.v" ]] || { printf 'ERROR: missing R2 RTL for %s at %s\n' "$config" "$rtl_dir" >&2; exit 2; }
    mkdir -p -- "$sim_work"
    rm -rf -- "$sim_work/xsim.dir"
    cp -- "$rtl_dir"/*.dat "$sim_work/" 2>/dev/null || true
    mapfile -t rtl_files < <(printf '%s\n' "$rtl_dir"/*.v | sort)
        defines=(-d "G6_L3_LQ_DEPTH=$lq_depth" -d "G6_L3_SQ_DEPTH=$sq_depth")
        if (( lq_depth >= 8 )); then defines+=(-d G6_L3_HAS_LQ8); fi
        if (( lq_depth >= 16 )); then defines+=(-d G6_L3_HAS_LQ16); fi
        if (( sq_depth >= 8 )); then defines+=(-d G6_L3_HAS_SQ8); fi
        if (( sq_depth >= 16 )); then defines+=(-d G6_L3_HAS_SQ16); fi
        case "$config" in
            8_8) defines+=(-d G6_L3_CYCLE_FU_6331) ;;
            16_16) defines+=(-d G6_L3_CYCLE_FU_6373) ;;
            4_16) defines+=(-d G6_L3_CYCLE_FU_6317) ;;
            16_4) defines+=(-d G6_L3_CYCLE_FU_6257) ;;
        esac

    if (
        cd -- "$sim_work"
        timeout 900 "$XVLOG" "${rtl_files[@]}"
        timeout 180 "$XVLOG" --sv "${defines[@]}" "$TB"
        timeout 300 "$XELAB" g6_l3_r3_blocker_isolation_tb -s g6_l3_r3_blocker_isolation_snapshot -timescale 1ns/1ps
    ) >"$build_log" 2>&1; then
        if (
            cd -- "$sim_work"
            timeout 1800 "$XSIM" g6_l3_r3_blocker_isolation_snapshot --runall --onerror quit --log "$sim_log"
        ) >"$sim_stdout" 2>&1; then
            sim_status=PASS
        else
            sim_status=FAIL
        fi
    else
        sim_status=BUILD_FAIL
    fi

    if [[ -s "$sim_work/request_backpressure_trace.csv" ]]; then
        if (( first_req )); then
            cp -- "$sim_work/request_backpressure_trace.csv" "$req_trace_tmp"
            first_req=0
        else
            python3 - "$sim_work/request_backpressure_trace.csv" "$req_trace_tmp" <<'PY'
import sys
src, dst = sys.argv[1:]
with open(src, 'r', errors='replace') as fh, open(dst, 'a') as out:
    next(fh, None)
    for line in fh:
        out.write(line)
PY
        fi
    fi
    if [[ -s "$sim_work/store_forwarding_trace.csv" ]]; then
        if (( first_fwd )); then
            cp -- "$sim_work/store_forwarding_trace.csv" "$fwd_trace_tmp"
            first_fwd=0
        else
            python3 - "$sim_work/store_forwarding_trace.csv" "$fwd_trace_tmp" <<'PY'
import sys
src, dst = sys.argv[1:]
with open(src, 'r', errors='replace') as fh, open(dst, 'a') as out:
    next(fh, None)
    for line in fh:
        out.write(line)
PY
        fi
    fi
    if [[ -s "$sim_work/flush_recovery_trace.csv" ]]; then
        if (( first_flush )); then
            cp -- "$sim_work/flush_recovery_trace.csv" "$flush_trace_tmp"
            first_flush=0
        else
            python3 - "$sim_work/flush_recovery_trace.csv" "$flush_trace_tmp" <<'PY'
import sys
src, dst = sys.argv[1:]
with open(src, 'r', errors='replace') as fh, open(dst, 'a') as out:
    next(fh, None)
    for line in fh:
        out.write(line)
PY
        fi
    fi

    if [[ -s "$sim_log" ]]; then
        python3 - "$config" "$sim_status" "$sim_log" "$results_tmp" <<'PY'
import re, sys
config, sim_status, log_path, out_path = sys.argv[1:]
kv_re = re.compile(r'(\w+)=([^\s]+)')
with open(log_path, 'r', errors='replace') as fh, open(out_path, 'a') as out:
    saw_final = False
    for line in fh:
        if 'G6_L3_R3_RESULT' in line:
            fields = dict(kv_re.findall(line))
            out.write(f"{config},{fields.get('key','UNKNOWN')},{fields.get('value','')},{log_path}\n")
        if f'G6_L3_R3_RTL_CONFIG_{config}_' in line:
            saw_final = True
            status = 'PASS' if f'CONFIG_{config}_PASS' in line else 'FAIL'
            out.write(f"{config},CONFIG_STATUS,{status},{log_path}\n")
    if not saw_final:
        out.write(f"{config},CONFIG_STATUS,{sim_status},{log_path}\n")
PY
    else
        printf '%s,CONFIG_STATUS,%s,%s\n' "$config" "$sim_status" "$build_log" >>"$results_tmp"
    fi
    printf 'G6_L3_R3_CONFIG_%s=%s log=%s\n' "$config" "$sim_status" "$sim_log"
done

[[ -s "$req_trace_tmp" ]] && mv -- "$req_trace_tmp" "$REPORT/request_backpressure_trace.csv" || printf 'config,cycle,phase,top_valid,top_ready,top_handshake,top_txid,top_rob,top_cmd,top_addr,internal_valid,internal_ready,internal_handshake,internal_txid,internal_rob,internal_cmd,internal_addr,regslice_state,owner_sent,outstanding,lq0_valid,lq0_pending,lq0_txid,event\n' >"$REPORT/request_backpressure_trace.csv"
[[ -s "$fwd_trace_tmp" ]] && mv -- "$fwd_trace_tmp" "$REPORT/store_forwarding_trace.csv" || printf 'config,cycle,phase,top_valid,top_ready,handshake,txid,rob,cmd,address,store_valid,store_addr_valid,store_data_valid,store_busy,load_completed,load_data,outstanding,event\n' >"$REPORT/store_forwarding_trace.csv"
[[ -s "$flush_trace_tmp" ]] && mv -- "$flush_trace_tmp" "$REPORT/flush_recovery_trace.csv" || printf 'config,cycle,phase,outstanding,active_lq,lq0_valid,lq0_pending,lq0_txid,lq1_valid,lq1_pending,lq1_txid,rob0_valid,rob1_valid,global_flush,exception_commit_valid,frontend_redirect_valid,dmem_resp_valid,dmem_resp_ready,event\n' >"$REPORT/flush_recovery_trace.csv"

python3 - "$REPORT" "$CONFIGS" "$results_tmp" <<'PY'
from pathlib import Path
import csv
import sys

report = Path(sys.argv[1])
configs = sys.argv[2].split()
results_path = Path(sys.argv[3])
all_configs = ['4_4', '8_8', '16_16', '4_16', '16_4']
results = {}
evidence = {}
if results_path.exists():
    with results_path.open(newline='', errors='replace') as fh:
        for row in csv.reader(fh):
            if len(row) < 4:
                continue
            cfg, key, value, ev = row[:4]
            results.setdefault(cfg, {})[key] = value
            evidence[(cfg, key)] = ev

def first_value(key, default='NOT_RUN'):
    for cfg in configs:
        if key in results.get(cfg, {}):
            return results[cfg][key]
    return default

def first_evidence(key):
    for cfg in configs:
        if (cfg, key) in evidence:
            return evidence[(cfg, key)]
    return ''

validation_rows = [
    ('L3R2_R3_BACKPRESSURE_ROOT_CAUSE', first_value('BACKPRESSURE_ROOT_CAUSE'), first_evidence('BACKPRESSURE_ROOT_CAUSE')),
    ('L3R2_R3_TVALID_HOLD_VERIFIED', first_value('TVALID_HOLD_VERIFIED'), first_evidence('TVALID_HOLD_VERIFIED')),
    ('L3R2_R3_REQUEST_LOSS_COUNT', first_value('REQUEST_LOSS_COUNT'), first_evidence('REQUEST_LOSS_COUNT')),
    ('L3R2_R3_REQUEST_DUPLICATION_COUNT', first_value('REQUEST_DUPLICATION_COUNT'), first_evidence('REQUEST_DUPLICATION_COUNT')),
    ('L3R2_R3_FORWARDING_SEED_VALID', first_value('FORWARDING_SEED_VALID'), first_evidence('FORWARDING_SEED_VALID')),
    ('L3R2_R3_FORWARDING_RTL_STATUS', first_value('FORWARDING_RTL_STATUS'), first_evidence('FORWARDING_RTL_STATUS')),
    ('L3R2_R3_FORWARDING_EXTERNAL_REQUESTS', first_value('FORWARDING_EXTERNAL_REQUESTS'), first_evidence('FORWARDING_EXTERNAL_REQUESTS')),
    ('L3R2_R3_FLUSH_RECOVERY_PATH_VALID', first_value('FLUSH_RECOVERY_PATH_VALID'), first_evidence('FLUSH_RECOVERY_PATH_VALID')),
    ('L3R2_R3_FLUSH_RTL_STATUS', first_value('FLUSH_RTL_STATUS'), first_evidence('FLUSH_RTL_STATUS')),
    ('L3R2_R3_STALE_RESPONSE_REJECTED', first_value('STALE_RESPONSE_REJECTED'), first_evidence('STALE_RESPONSE_REJECTED')),
]
for cfg in all_configs:
    status = results.get(cfg, {}).get('CONFIG_STATUS', 'NOT_RUN')
    validation_rows.append((f'L3R2_R3_CONFIG_{cfg}', status, evidence.get((cfg, 'CONFIG_STATUS'), '')))
validation_rows.extend([
    ('L3R2_R3_PRODUCT_CODE_MODIFIED', 'false', 'R3 added only isolated testbench/runner/reports; no product source edits'),
    ('L3R2_R3_VERIFICATION_COMPLETE', 'true' if all(results.get(cfg, {}).get('CONFIG_STATUS') == 'PASS' for cfg in all_configs) else 'false', str(report)),
    ('G6_0_L3_MULTI_OUTSTANDING_LOADS_VERIFIED', 'false', 'R3 isolates blockers only; full L3 closure remains gated until all configs PASS'),
])

with (report / 'validation_matrix.csv').open('w', newline='') as fh:
    wr = csv.writer(fh)
    wr.writerow(['field', 'value', 'evidence'])
    wr.writerows(validation_rows)

with (report / 'failure_root_cause_matrix.csv').open('w', newline='') as fh:
    wr = csv.writer(fh)
    wr.writerow(['blocker', 'r2_classification', 'r3_root_cause', 'rtl_status', 'evidence'])
    wr.writerow(['request_backpressure', 'INTERFACE_PROTOCOL_ERROR:no_tvalid_hold_after_owner_reserved', first_value('BACKPRESSURE_ROOT_CAUSE'), 'PASS' if first_value('TVALID_HOLD_VERIFIED') == '1' and first_value('REQUEST_LOSS_COUNT') == '0' and first_value('REQUEST_DUPLICATION_COUNT') == '0' else 'FAIL', 'request_backpressure_trace.csv'])
    wr.writerow(['store_forwarding', 'TESTBENCH_ERROR:focused_seed_did_not_force_forwarding', 'R2 assertion counted all external requests; R3 separates load/store command traffic', first_value('FORWARDING_RTL_STATUS'), 'store_forwarding_trace.csv'])
    wr.writerow(['flush_with_pending', 'TESTBENCH_ERROR:focused_global_flush_seed_not_observed', 'R3 uses exception commit recovery path instead of direct global_flush poke', first_value('FLUSH_RTL_STATUS'), 'flush_recovery_trace.csv'])

def write_md(path, title, lines):
    with (report / path).open('w') as fh:
        fh.write(f'# {title}\n\n')
        for line in lines:
            fh.write(f'{line}\n')

write_md('request_backpressure_analysis.md', 'Request Backpressure Analysis', [
    f"- Root cause: `{first_value('BACKPRESSURE_ROOT_CAUSE')}`.",
    f"- TVALID hold verified: `{first_value('TVALID_HOLD_VERIFIED')}`.",
    f"- Request loss count: `{first_value('REQUEST_LOSS_COUNT')}`.",
    f"- Request duplication count: `{first_value('REQUEST_DUPLICATION_COUNT')}`.",
    '- Evidence: `request_backpressure_trace.csv` traces top AXIS, internal producer, and regslice state.',
])
write_md('store_forwarding_analysis.md', 'Store Forwarding Analysis', [
    f"- Seed valid: `{first_value('FORWARDING_SEED_VALID')}`.",
    f"- RTL status: `{first_value('FORWARDING_RTL_STATUS')}`.",
    f"- External requests: `{first_value('FORWARDING_EXTERNAL_REQUESTS')}`.",
    '- Evidence: `store_forwarding_trace.csv` separates DMEM load (`cmd=0`) from store (`cmd=1`) requests.',
])
write_md('flush_recovery_analysis.md', 'Flush Recovery Analysis', [
    f"- Exception recovery path valid: `{first_value('FLUSH_RECOVERY_PATH_VALID')}`.",
    f"- RTL status: `{first_value('FLUSH_RTL_STATUS')}`.",
    f"- Stale response rejected: `{first_value('STALE_RESPONSE_REJECTED')}`.",
    '- Evidence: `flush_recovery_trace.csv` traces exception commit, redirect, global flush, outstanding count, and stale response probe.',
])
PY

rm -f -- "$results_tmp"
printf 'R3 reports written to %s\n' "$REPORT"
