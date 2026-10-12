#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-/home/lab_726/boom/hls_boom}
REPORT=${G6_L3R6_REPORT_DIR:-$ROOT/reports/gate6_0_full_lsu/l3/functional_signoff}
RTL=${G6_L3R6_RTL_DIR:-/home/lab_726/opencode_tmp/g6_l3r2_r5_preflight_4_4_20261009_155117/hls_project/solution/syn/verilog}
OUT=${G6_L3R6_CAPACITY_OUT:-/home/lab_726/opencode_tmp/g6_l3r6_capacity_4_4_20261012}
CLANG=${CLANG:-clang}
OBJCOPY=${LLVM_OBJCOPY:-/usr/lib/llvm-14/bin/llvm-objcopy}
OBJDUMP=${LLVM_OBJDUMP:-/usr/lib/llvm-14/bin/llvm-objdump}
XVLOG=${XVLOG:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xvlog}
XELAB=${XELAB:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xelab}
XSIM=${XSIM:-/home/lab_726/Xilinx/Vivado/2021.2/bin/xsim}

[[ "$OUT" == /home/lab_726/opencode_tmp/* ]] || {
  printf 'ERROR: capacity output must remain under /home/lab_726/opencode_tmp\n' >&2
  exit 2
}
[[ -s "$RTL/boom_core_top.v" ]] || { printf 'ERROR: boom_core_top.v missing in %s\n' "$RTL" >&2; exit 2; }

mkdir -p "$OUT/program" "$OUT/xsim" "$OUT/logs" "$REPORT"

"$CLANG" --target=riscv64 -march=rv64imac -mabi=lp64 -nostdlib \
  -Wl,--no-relax -Wl,-T,"$ROOT/tb/programs/rvc_fetch/linker.ld" \
  "$REPORT/l3r6_capacity_program.S" -o "$OUT/program/l3r6_capacity.elf"
"$OBJCOPY" -O binary --only-section=.text "$OUT/program/l3r6_capacity.elf" "$OUT/program/l3r6_capacity.bin"
"$OBJDUMP" -d --mattr=+c "$OUT/program/l3r6_capacity.elf" > "$OUT/program/l3r6_capacity.dump"
python3 - "$OUT/program/l3r6_capacity.bin" "$OUT/program/l3r6_capacity.words.hex" "$OUT/program/l3r6_capacity.hex" <<'PY'
import sys
from pathlib import Path
binary, words_path, beats_path = map(Path, sys.argv[1:])
data = binary.read_bytes()
if not data:
    raise SystemExit("empty capacity program image")
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
  "$XVLOG" --sv "$REPORT/l3r6_capacity_tb.sv"
  "$XELAB" l3r6_capacity_tb -s l3r6_capacity_snapshot -timescale 1ns/1ps
) >"$OUT/logs/xsim_build.log" 2>&1

(
  cd "$OUT/xsim"
  "$XSIM" l3r6_capacity_snapshot --runall --onerror quit \
    --testplusarg "PROGRAM=$OUT/program/l3r6_capacity.hex" \
    --testplusarg "OUT_DIR=$OUT" \
    --wdb "$OUT/l3r6_capacity.wdb" \
    --log "$OUT/logs/l3r6_capacity.log"
) >"$OUT/logs/l3r6_capacity.stdout.log" 2>&1

python3 "$REPORT/l3r6_analyze_capacity.py" "$OUT" "$REPORT" "$RTL" "$OUT/program/l3r6_capacity.bin" >"$OUT/logs/l3r6_capacity_analysis.json"
