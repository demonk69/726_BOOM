#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd -P)"}
SRC="$ROOT/tb/programs/pf5_product"
OUT=${PF5_PROGRAM_BUILD:-/tmp/boom_hls/pf5/programs}
CLANG=${CLANG:-clang}
OBJCOPY=${LLVM_OBJCOPY:-/usr/lib/llvm-14/bin/llvm-objcopy}
OBJDUMP=${LLVM_OBJDUMP:-/usr/lib/llvm-14/bin/llvm-objdump}
NM=${LLVM_NM:-nm}
PROGRAMS=(pf5_pred_nt_actual_nt pf5_pred_nt_actual_t pf5_pred_t_actual_t
          pf5_pred_t_actual_nt pf5_rvc_commit pf5_same_packet_kill
          pf5_fault_no_training pf5_ftq_wrap_training pf5_jal_no_training
          pf5_jalr_no_training pf5_exception_no_training pf5_mixed_long_training)

mkdir -p -- "$OUT"
rm -f -- "$OUT"/*
for name in "${PROGRAMS[@]}"; do
  "$CLANG" --target=riscv64 -march=rv64imac -mabi=lp64 -mno-relax -nostdlib \
    -Wl,--no-relax -Wl,-T,"$ROOT/tb/programs/rvc_fetch/linker.ld" \
    -I"$ROOT/tb/programs/rvc_fetch" "$SRC/$name.S" -o "$OUT/$name.elf"
  "$OBJCOPY" -O binary --only-section=.text "$OUT/$name.elf" "$OUT/$name.bin"
  "$OBJDUMP" -d --mattr=+c "$OUT/$name.elf" >"$OUT/$name.dump"
  "$NM" -n "$OUT/$name.elf" >"$OUT/$name.symbols"
  python3 - "$OUT/$name.bin" "$OUT/$name.words.hex" "$OUT/$name.hex" \
    "$OUT/$name.symbols" "$OUT/$name.init" <<'PY'
import sys
from pathlib import Path

binary, words_path, beats_path, symbols_path, init_path = map(Path, sys.argv[1:])
data = binary.read_bytes()
padded = data + b"\x01\x00" * ((8 - len(data) % 8) % 8 // 2)
words_path.write_text("\n".join(f"{int.from_bytes(padded[i:i+4], 'little'):08x}" for i in range(0, len(padded), 4)) + "\n", encoding="ascii")
beats_path.write_text("\n".join(f"{int.from_bytes(padded[i:i+8], 'little'):016x}" for i in range(0, len(padded), 8)) + "\n", encoding="ascii")
entries = []
for line in symbols_path.read_text(encoding="ascii").splitlines():
    fields = line.split()
    if len(fields) != 3:
        continue
    address, _, symbol = fields
    if symbol.startswith("pf5_bim_wt_"):
        entries.append((address, 2))
    elif symbol.startswith("pf5_bim_st_"):
        entries.append((address, 3))
    elif symbol.startswith("pf5_bim_probe_"):
        entries.append((address, 1))
init_path.write_text("".join(f"{address} {counter}\n" for address, counter in entries), encoding="ascii")
PY
done
printf 'PF5_PRODUCT_PROGRAM_BUILD %u/%u PASS\n' "${#PROGRAMS[@]}" "${#PROGRAMS[@]}"
