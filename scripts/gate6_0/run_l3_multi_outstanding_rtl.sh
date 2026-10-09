#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)"}
RTL=${G6_L3_RTL:-/home/lab_726/opencode_tmp/g6_l3r2/rtl_generation/4_4/hls_project/solution/syn/verilog}
WORK=${G6_L3_RTL_WORK:-/home/lab_726/opencode_tmp/g6_l3r2/rtl_verification/xsim_4_4}
REPORT=${G6_L3_RTL_REPORT:-$ROOT/reports/gate6_0_full_lsu/l3/rtl_verification}
XVLOG=${XVLOG:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xvlog}
XELAB=${XELAB:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xelab}
XSIM=${XSIM:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xsim}

[[ "$WORK" == /home/lab_726/opencode_tmp/* ]] || {
    printf 'ERROR: RTL simulation workspace must remain under /home/lab_726/opencode_tmp\n' >&2
    exit 2
}
[[ -s "$RTL/boom_core_top.v" ]] || { printf 'ERROR: CONFIG_4_4 boom_core_top RTL is missing\n' >&2; exit 2; }
[[ -s "$ROOT/rtl_tb/gate6_0/g6_l3_multi_outstanding_tb.sv" ]] || { printf 'ERROR: L3 RTL testbench is missing\n' >&2; exit 2; }

mkdir -p -- "$WORK" "$REPORT/logs"
rm -rf -- "$WORK/xsim.dir"
cp -- "$RTL"/*.dat "$WORK/" 2>/dev/null || true
mapfile -t rtl_files < <(printf '%s\n' "$RTL"/*.v | sort)
rtl_hash=$(python3 - "$RTL" <<'PY'
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
tb_hash=$(python3 - "$ROOT/rtl_tb/gate6_0/g6_l3_multi_outstanding_tb.sv" <<'PY'
import hashlib, sys
from pathlib import Path
print(hashlib.sha256(Path(sys.argv[1]).read_bytes()).hexdigest())
PY
)

(
    cd -- "$WORK"
    timeout 600 "$XVLOG" "${rtl_files[@]}"
    timeout 180 "$XVLOG" --sv "$ROOT/rtl_tb/gate6_0/g6_l3_multi_outstanding_tb.sv"
    timeout 300 "$XELAB" g6_l3_multi_outstanding_tb -s g6_l3_multi_outstanding_snapshot -timescale 1ns/1ps
) >"$REPORT/logs/config_4_4_build.log" 2>&1

(
    cd -- "$WORK"
    timeout 1200 "$XSIM" g6_l3_multi_outstanding_snapshot --runall --onerror quit \
        --log "$REPORT/logs/config_4_4_simulation.log"
) >"$REPORT/logs/config_4_4_simulation.stdout.log" 2>&1

grep -q 'G6_L3_RTL_CONFIG_4_4_PASS cases=5' "$REPORT/logs/config_4_4_simulation.log"
printf 'config,lq_depth,sq_depth,top,status,cases,requests,responses,rtl_hash,tb_hash,evidence\n' >"$REPORT/rtl_test_matrix.csv"
printf 'CONFIG_4_4,4,4,boom_core_top,PASS,5,5,6,%s,%s,%s\n' \
    "$rtl_hash" "$tb_hash" "reports/gate6_0_full_lsu/l3/rtl_verification/logs/config_4_4_simulation.log" \
    >>"$REPORT/rtl_test_matrix.csv"
printf 'G6_L3_RTL_CONFIG_4_4=PASS cases=5 requests=5 responses=6 rtl_hash=%s\n' "$rtl_hash"
