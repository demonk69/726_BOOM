#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-/home/lab_726/boom/hls_boom}
REPORT=${G6_L3_PPA6_REPORT_DIR:-$ROOT/reports/gate6_0_full_lsu/l3/full_core_end_to_end}
RTL=${G6_L3_PPA6_RTL_DIR:-/home/lab_726/opencode_tmp/g6_l3r2_r5_preflight_4_4_20261009_155117/hls_project/solution/syn/verilog}
OUT=${G6_L3_PPA6_OUT:-/home/lab_726/opencode_tmp/g6_l3r5_ppa6_full_core_e2e_20261010}
CLANG=${CLANG:-clang}
OBJCOPY=${LLVM_OBJCOPY:-/usr/lib/llvm-14/bin/llvm-objcopy}
OBJDUMP=${LLVM_OBJDUMP:-/usr/lib/llvm-14/bin/llvm-objdump}
XVLOG=${XVLOG:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xvlog}
XELAB=${XELAB:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xelab}
XSIM=${XSIM:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xsim}

[[ "$OUT" == /home/lab_726/opencode_tmp/* ]] || {
  printf 'ERROR: PPA6 output must remain under /home/lab_726/opencode_tmp\n' >&2
  exit 2
}
[[ -s "$RTL/boom_core_top.v" ]] || { printf 'ERROR: boom_core_top.v missing in %s\n' "$RTL" >&2; exit 2; }

mkdir -p "$OUT/program" "$OUT/xsim" "$OUT/logs" "$REPORT"

"$CLANG" --target=riscv64 -march=rv64imac -mabi=lp64 -nostdlib \
  -Wl,--no-relax -Wl,-T,"$ROOT/tb/programs/rvc_fetch/linker.ld" \
  "$REPORT/ppa6_multi_outstanding_program.S" -o "$OUT/program/ppa6_multi_outstanding.elf"
"$OBJCOPY" -O binary --only-section=.text "$OUT/program/ppa6_multi_outstanding.elf" "$OUT/program/ppa6_multi_outstanding.bin"
"$OBJDUMP" -d --mattr=+c "$OUT/program/ppa6_multi_outstanding.elf" > "$OUT/program/ppa6_multi_outstanding.dump"
python3 - "$OUT/program/ppa6_multi_outstanding.bin" "$OUT/program/ppa6_multi_outstanding.words.hex" "$OUT/program/ppa6_multi_outstanding.hex" <<'PY'
import sys
from pathlib import Path
binary, words_path, beats_path = map(Path, sys.argv[1:])
data = binary.read_bytes()
if not data:
    raise SystemExit("empty PPA6 program image")
padded = data + b"\x01\x00" * ((8 - len(data) % 8) % 8 // 2)
words_path.write_text("\n".join(f"{int.from_bytes(padded[i:i+4], 'little'):08x}" for i in range(0, len(padded), 4)) + "\n", encoding="ascii")
beats_path.write_text("\n".join(f"{int.from_bytes(padded[i:i+8], 'little'):016x}" for i in range(0, len(padded), 8)) + "\n", encoding="ascii")
PY

rm -rf "$OUT/xsim/xsim.dir"
cp "$RTL"/*.dat "$OUT/xsim/" 2>/dev/null || true
mapfile -t RTL_FILES < <(printf '%s\n' "$RTL"/*.v | sort)
(
  cd "$OUT/xsim"
  "$XVLOG" "${RTL_FILES[@]}"
  "$XVLOG" --sv "$REPORT/ppa6_multi_outstanding_tb.sv"
  "$XELAB" ppa6_multi_outstanding_tb -s ppa6_multi_outstanding_snapshot -timescale 1ns/1ps
) >"$OUT/logs/xsim_build.log" 2>&1

(
  cd "$OUT/xsim"
  "$XSIM" ppa6_multi_outstanding_snapshot --runall --onerror quit \
    --testplusarg "PROGRAM=$OUT/program/ppa6_multi_outstanding.hex" \
    --testplusarg "OUT_DIR=$OUT" \
    --wdb "$OUT/ppa6_multi_outstanding.wdb" \
    --log "$OUT/logs/ppa6_multi_outstanding.log"
) >"$OUT/logs/ppa6_multi_outstanding.stdout.log" 2>&1

python3 "$REPORT/ppa6_analyze_full_core_e2e.py" \
  "$ROOT" "$OUT" "$REPORT" "$RTL" \
  "$OUT/program/ppa6_multi_outstanding.elf" \
  "$OUT/program/ppa6_multi_outstanding.bin" \
  >"$OUT/logs/ppa6_analysis.json"
