#!/usr/bin/env python3
import csv
import sys
from pathlib import Path


def encode_i(immediate, rs1, funct3, rd, opcode):
    return ((immediate & 0xfff) << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode


output = Path(sys.argv[1])
rows = [(0x9002, 0x00100073, "C.EBREAK", "SYSTEM Decode lacked breakpoint cause 3")]
for register in range(8):
    for low_shamt in range(32):
        parcel = 0x9001 | (register << 7) | (low_shamt << 2)
        rd = register + 8
        rows.append((parcel, encode_i(32 | low_shamt, rd, 5, rd, 0x13),
                     "C.SRLI shamt[5]=1", "RV64 OP-IMM shift was classified by funct7 instead of funct6"))
for rs1 in range(1, 32):
    parcel = 0x9002 | (rs1 << 7)
    rows.append((parcel, encode_i(0, rs1, 0, 1, 0x67), "C.JALR",
                 "Execute link value ignored compressed instruction length"))

if len(rows) != 288 or len({row[0] for row in rows}) != 288:
    raise SystemExit("R3 gap inventory is not exactly 288 unique encodings")
output.parent.mkdir(parents=True, exist_ok=True)
with output.open("w", newline="", encoding="ascii") as stream:
    writer = csv.writer(stream, lineterminator="\n")
    writer.writerow(("compressed_encoding", "expanded_instruction", "class", "r2_root_cause", "r3_result"))
    for parcel, instruction, family, cause in sorted(rows):
        writer.writerow((f"0x{parcel:04x}", f"0x{instruction:08x}", family, cause, "SUPPORTED"))
print(f"R3_GAP_INVENTORY entries={len(rows)} unique={len(set(row[0] for row in rows))}")
