#!/usr/bin/env bash
set -euo pipefail

ROOT=${HLS_BOOM_ROOT:-"$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"}
SRC="$ROOT/tb/programs/b3i_packet"
OUT="$SRC/build"
CLANG=${CLANG:-clang}
OBJCOPY=${LLVM_OBJCOPY:-/usr/lib/llvm-14/bin/llvm-objcopy}
OBJDUMP=${LLVM_OBJDUMP:-/usr/lib/llvm-14/bin/llvm-objdump}
PROGRAMS=(packet_two_rvc packet_rvc_rvc_long packet_carry_plus_rvc packet_atomic_backpressure packet_redirect packet_fault)

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
if not data:
    raise SystemExit(f"{binary}: empty text image")
padded = data + b"\x01\x00" * ((8 - len(data) % 8) % 8 // 2)
words_path.write_text("\n".join(f"{int.from_bytes(padded[i:i+4], 'little'):08x}"
                                 for i in range(0, len(padded), 4)) + "\n", encoding="ascii")
beats_path.write_text("\n".join(f"{int.from_bytes(padded[i:i+8], 'little'):016x}"
                                for i in range(0, len(padded), 8)) + "\n", encoding="ascii")
PY
done

python3 - "$OUT" "${PROGRAMS[@]}" <<'PY'
import csv
import sys
from pathlib import Path

out = Path(sys.argv[1])
programs = sys.argv[2:]

def parcel(data, offset):
    return int.from_bytes(data[offset:offset + 2], "little")

def is_c(data, offset):
    return offset + 2 <= len(data) and parcel(data, offset) & 3 != 3

def is_i32(data, offset):
    return offset + 4 <= len(data) and parcel(data, offset) & 3 == 3 and parcel(data, offset) & 0x1f != 0x1f

checks = {
    "packet_two_rvc": lambda d: is_c(d, 0) and is_c(d, 2),
    "packet_rvc_rvc_long": lambda d: is_c(d, 0) and is_c(d, 2) and is_i32(d, 4),
    "packet_carry_plus_rvc": lambda d: is_c(d, 0) and is_i32(d, 2) and is_c(d, 6),
    "packet_atomic_backpressure": lambda d: is_i32(d, 12) and is_c(d, 16) and is_c(d, 18),
    "packet_redirect": lambda d: is_c(d, 0) and is_c(d, 2) and ((parcel(d, 2) >> 13) & 7) == 5,
    "packet_fault": lambda d: is_c(d, 0) and parcel(d, 2) == 0 and is_c(d, 4),
}
descriptions = {
    "packet_two_rvc": "aligned response contains two legal RVC parcels",
    "packet_rvc_rvc_long": "two RVC parcels followed by an aligned 32-bit instruction",
    "packet_carry_plus_rvc": "32-bit instruction starts at +2 and response B upper parcel is RVC",
    "packet_atomic_backpressure": "load-delayed ROB pressure followed by aligned RVC pairs",
    "packet_redirect": "compressed jump redirects over younger compressed instructions",
    "packet_fault": "older RVC, illegal RVC parcel, then structurally younger RVC",
}
rows = []
for name in programs:
    data = (out / f"{name}.bin").read_bytes()
    compressed = 0
    offset = 0
    while offset + 1 < len(data):
        if is_c(data, offset):
            compressed += 1
            offset += 2
        else:
            offset += 4
    ok = compressed > 0 and checks[name](data)
    rows.append((name, len(data), compressed, descriptions[name], "PASS" if ok else "FAIL"))
if any(row[-1] != "PASS" for row in rows):
    raise SystemExit("B3I packet image structural verification failed")
with (out / "structural_manifest.csv").open("w", newline="") as stream:
    writer = csv.writer(stream)
    writer.writerow(("program", "text_bytes", "compressed_parcels", "required_structure", "status"))
    writer.writerows(rows)
print(f"Gate 5.3 B3I assembled packet images: {len(rows)}/{len(rows)} PASS")
PY
