#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)"}
SOURCE_ROOT=${G6_L3_R2_SOURCE_ROOT:-/home/lab_726/opencode_tmp/g6_l3r2_r2_clean}
BUILD_ROOT=${G6_L3_R2_BUILD_ROOT:-/home/lab_726/opencode_tmp/g6_l3r2_r2}
REPORT=${G6_L3_R2_REPORT:-$ROOT/reports/gate6_0_full_lsu/l3/rtl_verification}
VITIS_HLS=${VITIS_HLS:-/home/lab_726/Xilinx/Vitis_HLS/2021.2/bin/vitis_hls}
XVLOG=${XVLOG:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xvlog}
XELAB=${XELAB:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xelab}
XSIM=${XSIM:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xsim}
CONFIGS=${G6_L3_R2_CONFIGS:-"4_4 8_8 16_16 4_16 16_4"}
REUSE_RTL=${G6_L3_R2_REUSE_RTL:-0}

[[ "$BUILD_ROOT" == /home/lab_726/opencode_tmp/* ]] || {
    printf 'ERROR: build root must remain under /home/lab_726/opencode_tmp\n' >&2
    exit 2
}
[[ -s "$SOURCE_ROOT/src/boom_core_merged.cpp" ]] || { printf 'ERROR: clean HLS source root missing boom_core_merged.cpp\n' >&2; exit 2; }
[[ -s "$ROOT/rtl_tb/gate6_0/g6_l3_multi_outstanding_stress_tb.sv" ]] || { printf 'ERROR: R2 RTL stress testbench missing\n' >&2; exit 2; }

mkdir -p -- "$BUILD_ROOT/rtl_generation" "$BUILD_ROOT/rtl_stress" "$REPORT/logs/r2"

matrix_tmp="$REPORT/extended_config_matrix.csv.tmp"
stress_tmp="$REPORT/transaction_stress_matrix.csv.tmp"
trace_tmp="$REPORT/transaction_ownership_trace.csv.tmp"
printf 'config,lq_depth,sq_depth,hls_generation,rtl_compile,rtl_sim,classification,cases,failures,requests,responses,max_observed_outstanding,response_backpressure_blocked_cycles,stale_response_reject_count,rtl_sha256,stress_tb_sha256,runner_sha256,simulation_exit_status,csynth_log,build_log,simulation_log\n' >"$matrix_tmp"
printf 'config,case,status,classification,details,evidence\n' >"$stress_tmp"
printf 'config,case,event,index,txid,rob,lq,outstanding,address,data,evidence\n' >"$trace_tmp"

tb_hash=$(python3 - "$ROOT/rtl_tb/gate6_0/g6_l3_multi_outstanding_stress_tb.sv" <<'PY'
import hashlib, sys
from pathlib import Path
print(hashlib.sha256(Path(sys.argv[1]).read_bytes()).hexdigest())
PY
)

runner_hash=$(python3 - "$ROOT/scripts/gate6_0/run_l3_r2_extended_rtl_stress.sh" <<'PY'
import hashlib, sys
from pathlib import Path
print(hashlib.sha256(Path(sys.argv[1]).read_bytes()).hexdigest())
PY
)

for config in $CONFIGS; do
    lq_depth=${config%_*}
    sq_depth=${config#*_}
    gen_work="$BUILD_ROOT/rtl_generation/$config"
    sim_work="$BUILD_ROOT/rtl_stress/$config"
    rtl_dir="$gen_work/hls_project/solution/syn/verilog"
    csynth_log="$REPORT/logs/r2/${config}_csynth.log"
    build_log="$REPORT/logs/r2/${config}_stress_build.log"
    sim_log="$REPORT/logs/r2/${config}_stress_simulation.log"
    sim_stdout="$REPORT/logs/r2/${config}_stress_simulation.stdout.log"
    hls_generation=FAIL
    rtl_compile=NOT_RUN
    rtl_sim=NOT_RUN
    classification=HLS_GENERATION_ERROR
    rtl_hash=NA
    cases=0
    failures=0
    requests=0
    responses=0
    max_observed=0
    response_bp=0
    stale_reject=0
    sim_exit_status=NA

    mkdir -p -- "$gen_work" "$sim_work"
    if [[ "$REUSE_RTL" == 1 && -s "$rtl_dir/boom_core_top.v" ]]; then
        hls_generation=PASS
        classification=RTL_SIM_NOT_RUN
        if [[ ! -s "$csynth_log" ]]; then
            printf 'Reused existing RTL at %s\n' "$rtl_dir" >"$csynth_log"
        fi
        rtl_hash=$(python3 - "$rtl_dir" <<'PY'
import hashlib, sys
from pathlib import Path
root = Path(sys.argv[1])
digest = hashlib.sha256()
for path in sorted(root.glob("*.v")) + sorted(root.glob("*.dat")):
    digest.update(path.name.encode() + b"\0")
    digest.update(hashlib.sha256(path.read_bytes()).digest())
print(digest.hexdigest())
PY
)
    elif HLS_BOOM_ROOT="$SOURCE_ROOT" G6_L2_SYNTH_WORK="$gen_work" \
        LQ_DEPTH="$lq_depth" SQ_DEPTH="$sq_depth" \
        timeout 7200 "$VITIS_HLS" -f "$SOURCE_ROOT/scripts/gate6_0/l2_full_core_csynth.tcl" \
        >"$csynth_log" 2>&1; then
        if [[ -s "$rtl_dir/boom_core_top.v" ]]; then
            hls_generation=PASS
            classification=RTL_SIM_NOT_RUN
            rtl_hash=$(python3 - "$rtl_dir" <<'PY'
import hashlib, sys
from pathlib import Path
root = Path(sys.argv[1])
digest = hashlib.sha256()
for path in sorted(root.glob("*.v")) + sorted(root.glob("*.dat")):
    digest.update(path.name.encode() + b"\0")
    digest.update(hashlib.sha256(path.read_bytes()).digest())
print(digest.hexdigest())
PY
)
        else
            classification=HLS_GENERATION_ERROR_NO_RTL
        fi
    fi

    if [[ "$hls_generation" == PASS ]]; then
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
            timeout 180 "$XVLOG" --sv "${defines[@]}" "$ROOT/rtl_tb/gate6_0/g6_l3_multi_outstanding_stress_tb.sv"
            timeout 300 "$XELAB" g6_l3_multi_outstanding_stress_tb -s g6_l3_multi_outstanding_stress_snapshot -timescale 1ns/1ps
        ) >"$build_log" 2>&1; then
            rtl_compile=PASS
            if (
                cd -- "$sim_work"
                timeout 1800 "$XSIM" g6_l3_multi_outstanding_stress_snapshot --runall --onerror quit --log "$sim_log"
            ) >"$sim_stdout" 2>&1; then
                sim_exit_status=0
                if grep -q "G6_L3_R2_RTL_CONFIG_${lq_depth}_${sq_depth}_PASS" "$sim_log"; then
                    rtl_sim=PASS
                    classification=PASS
                elif grep -q "G6_L3_R2_RTL_CONFIG_${lq_depth}_${sq_depth}_FAIL" "$sim_log"; then
                    rtl_sim=FAIL
                    classification=RTL_STRESS_FAILURE
                else
                    rtl_sim=FAIL
                    classification=RTL_SIM_NO_FINAL_STATUS
                fi
            else
                sim_exit_status=$?
                rtl_sim=FAIL
                classification=RTL_SIM_ERROR
            fi
        else
            rtl_compile=FAIL
            classification=RTL_BUILD_ERROR
        fi
    fi

    if [[ -s "$sim_log" ]]; then
        python3 - "$config" "$sim_log" "$stress_tmp" "$trace_tmp" <<'PY'
import re, sys
config, log_path, stress_path, trace_path = sys.argv[1:]
kv_re = re.compile(r'(\w+)=([^\s]+)')
with open(log_path, 'r', errors='replace') as fh, \
     open(stress_path, 'a') as stress, \
     open(trace_path, 'a') as trace:
    for line in fh:
        line = line.strip()
        if 'G6_L3_R2_CASE_PASS' in line or 'G6_L3_R2_CASE_FAIL' in line:
            status = 'PASS' if 'CASE_PASS' in line else 'FAIL'
            fields = dict(kv_re.findall(line))
            case = fields.get('case', 'UNKNOWN')
            classification = fields.get('classification', 'PASS' if status == 'PASS' else 'UNCLASSIFIED')
            detail = ';'.join(f'{k}={v}' for k, v in fields.items() if k not in {'case', 'classification'})
            stress.write(f'{config},{case},{status},{classification},{detail},{log_path}\n')
        elif 'G6_L3_R2_TRACE' in line:
            fields = dict(kv_re.findall(line))
            trace.write(','.join([
                config,
                fields.get('case', ''),
                fields.get('event', ''),
                fields.get('index', ''),
                fields.get('txid', ''),
                fields.get('rob', ''),
                fields.get('lq', ''),
                fields.get('outstanding', ''),
                fields.get('address', ''),
                fields.get('data', ''),
                log_path,
            ]) + '\n')
PY
        final_line=$(grep "G6_L3_R2_RTL_CONFIG_${lq_depth}_${sq_depth}_" "$sim_log" | tail -n 1 || true)
        if [[ -n "$final_line" ]]; then
            cases=$(printf '%s\n' "$final_line" | sed -n 's/.*cases=\([0-9][0-9]*\).*/\1/p')
            failures=$(printf '%s\n' "$final_line" | sed -n 's/.*failures=\([0-9][0-9]*\).*/\1/p')
            requests=$(printf '%s\n' "$final_line" | sed -n 's/.*requests=\([0-9][0-9]*\).*/\1/p')
            responses=$(printf '%s\n' "$final_line" | sed -n 's/.*responses=\([0-9][0-9]*\).*/\1/p')
            max_observed=$(printf '%s\n' "$final_line" | sed -n 's/.*max_outstanding=\([0-9][0-9]*\).*/\1/p')
            response_bp=$(printf '%s\n' "$final_line" | sed -n 's/.*response_backpressure_blocked_cycles=\([0-9][0-9]*\).*/\1/p')
            stale_reject=$(printf '%s\n' "$final_line" | sed -n 's/.*stale_response_reject_count=\([0-9][0-9]*\).*/\1/p')
            cases=${cases:-0}
            failures=${failures:-0}
            requests=${requests:-0}
            responses=${responses:-0}
            max_observed=${max_observed:-0}
            response_bp=${response_bp:-0}
            stale_reject=${stale_reject:-0}
        fi
    fi

    printf '%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n' \
        "CONFIG_$config" "$lq_depth" "$sq_depth" "$hls_generation" "$rtl_compile" "$rtl_sim" "$classification" \
        "$cases" "$failures" "$requests" "$responses" "$max_observed" "$response_bp" \
        "$stale_reject" "$rtl_hash" "$tb_hash" "$runner_hash" "$sim_exit_status" \
        "reports/gate6_0_full_lsu/l3/rtl_verification/logs/r2/${config}_csynth.log" \
        "reports/gate6_0_full_lsu/l3/rtl_verification/logs/r2/${config}_stress_build.log" \
        "reports/gate6_0_full_lsu/l3/rtl_verification/logs/r2/${config}_stress_simulation.log" >>"$matrix_tmp"
    printf 'G6_L3_R2_CONFIG_%s=%s classification=%s max_observed=%s stale_reject=%s rtl_hash=%s sim_exit_status=%s\n' \
        "$config" "$rtl_sim" "$classification" "$max_observed" "$stale_reject" "$rtl_hash" "$sim_exit_status"
done

mv -- "$matrix_tmp" "$REPORT/extended_config_matrix.csv"
mv -- "$stress_tmp" "$REPORT/transaction_stress_matrix.csv"
mv -- "$trace_tmp" "$REPORT/transaction_ownership_trace.csv"
