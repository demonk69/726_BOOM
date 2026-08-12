#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
SRC="$ROOT/tb/programs/rvc_fetch"
OUT="$SRC/build"
CLANG=${CLANG:-clang}
OBJCOPY=${LLVM_OBJCOPY:-/usr/lib/llvm-14/bin/llvm-objcopy}
OBJDUMP=${LLVM_OBJDUMP:-/usr/lib/llvm-14/bin/llvm-objdump}
PROGRAMS=(rvc_addi rvc_load_store rvc_branch rvc_jump rvc_word_ops rvc_mixed_16_32 rvc_cross_boundary rvc_rv64m_mix rvc_redirect_halfword rvc_tohost rvc_decode_gaps)

mkdir -p "$OUT"
rm -f "$OUT"/*
for name in "${PROGRAMS[@]}"; do
  "$CLANG" --target=riscv64 -march=rv64imac -mabi=lp64 -nostdlib \
    -Wl,--no-relax -Wl,-T,"$SRC/linker.ld" -I"$SRC" "$SRC/$name.S" -o "$OUT/$name.elf"
  "$OBJCOPY" -O binary --only-section=.text "$OUT/$name.elf" "$OUT/$name.bin"
  "$OBJDUMP" -d --mattr=+c "$OUT/$name.elf" > "$OUT/$name.dump"
  python3 - "$OUT/$name.bin" "$OUT/$name.words.hex" "$OUT/$name.hex" <<'PY'
import sys
from pathlib import Path

binary, words_path, beats_path = map(Path, sys.argv[1:])
data = binary.read_bytes()
if not data:
    raise SystemExit(f"{binary}: empty text image")
data += b"\x01\x00" * ((8 - len(data) % 8) % 8 // 2)
words = [data[i:i + 4] for i in range(0, len(data), 4)]
beats = [data[i:i + 8] for i in range(0, len(data), 8)]
# Each token is the numeric value returned by a little-endian aligned memory
# read. Thus bytes 11 22 33 44 become the readmemh token 44332211.
words_path.write_text("\n".join(f"{int.from_bytes(x, 'little'):08x}" for x in words) + "\n", encoding="ascii")
beats_path.write_text("\n".join(f"{int.from_bytes(x, 'little'):016x}" for x in beats) + "\n", encoding="ascii")
PY
done
printf 'Gate 5.2 R2 RVC byte streams built: %d/%d\n' "${#PROGRAMS[@]}" "${#PROGRAMS[@]}"
