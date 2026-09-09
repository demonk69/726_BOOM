#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
SRC="$ROOT/tb/programs/pf3_product"
OUT=${PF3_PROGRAM_BUILD:-/tmp/boom_hls/pf3a/programs}
CLANG=${CLANG:-clang}
OBJCOPY=${LLVM_OBJCOPY:-/usr/lib/llvm-14/bin/llvm-objcopy}
OBJDUMP=${LLVM_OBJDUMP:-/usr/lib/llvm-14/bin/llvm-objdump}
PROGRAMS=(pf3_straight_commit pf3_rvc_packets pf3_jal_mask pf3_conditional_shadow
          pf3_branch_squash pf3_exception_flush pf3_ftq_wrap pf3_generation_reuse
          pf3_rv64m pf3_mixed_control pf3_long_stream pf3_reset_midstream)

mkdir -p "$OUT"
rm -f "$OUT"/*
for name in "${PROGRAMS[@]}"; do
  "$CLANG" --target=riscv64 -march=rv64imac -mabi=lp64 -nostdlib \
    -Wl,--no-relax -Wl,-T,"$ROOT/tb/programs/rvc_fetch/linker.ld" \
    -I"$ROOT/tb/programs/rvc_fetch" "$SRC/$name.S" -o "$OUT/$name.elf"
  "$OBJCOPY" -O binary --only-section=.text "$OUT/$name.elf" "$OUT/$name.bin"
  "$OBJDUMP" -d --mattr=+c "$OUT/$name.elf" > "$OUT/$name.dump"
  python3 - "$OUT/$name.bin" "$OUT/$name.words.hex" "$OUT/$name.hex" <<'PY'
import sys
from pathlib import Path
binary, words_path, beats_path = map(Path, sys.argv[1:])
data = binary.read_bytes()
padded = data + b"\x01\x00" * ((8 - len(data) % 8) % 8 // 2)
words_path.write_text("\n".join(f"{int.from_bytes(padded[i:i+4], 'little'):08x}" for i in range(0, len(padded), 4)) + "\n", encoding="ascii")
beats_path.write_text("\n".join(f"{int.from_bytes(padded[i:i+8], 'little'):016x}" for i in range(0, len(padded), 8)) + "\n", encoding="ascii")
PY
done
printf 'PF3_PRODUCT_PROGRAM_BUILD %u/%u PASS\n' "${#PROGRAMS[@]}" "${#PROGRAMS[@]}"
