#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "tb/programs/boom_reference/m3b"
TOHOST = 0x80000080


def r(opcode, rd, funct3, rs1, rs2, funct7=1):
    return (funct7 << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode


def m(rd, rs1, rs2, funct3, word=False):
    return r(0x3B if word else 0x33, rd, funct3, rs1, rs2)


def addi(rd, rs1, imm):
    return ((imm & 0xFFF) << 20) | (rs1 << 15) | (rd << 7) | 0x13


def slli(rd, rs1, shamt):
    return ((shamt & 0x3F) << 20) | (rs1 << 15) | (1 << 12) | (rd << 7) | 0x13


def auipc(rd, imm20=0):
    return (imm20 << 12) | (rd << 7) | 0x17


def sd(rs2, rs1, imm=0):
    value = imm & 0xFFF
    return ((value >> 5) << 25) | (rs2 << 20) | (rs1 << 15) | (3 << 12) | ((value & 31) << 7) | 0x23


def ld(rd, rs1, imm=0):
    return ((imm & 0xFFF) << 20) | (rs1 << 15) | (3 << 12) | (rd << 7) | 0x03


def beq(rs1, rs2, offset):
    imm = offset & 0x1FFF
    return (((imm >> 12) & 1) << 31) | (((imm >> 5) & 0x3F) << 25) | (rs2 << 20) | (rs1 << 15) | (((imm >> 1) & 0xF) << 8) | (((imm >> 11) & 1) << 7) | 0x63


def absolute(reg, offset):
    return [addi(reg, 0, 1), slli(reg, reg, 31), addi(reg, reg, offset)]


def finish(code, stress=False):
    code += absolute(30, 0x80)
    if stress:
        code += [addi(29, 0, 7), sd(29, 30, 8), sd(29, 30, 16)]
    code += [addi(31, 0, 1), sd(31, 30, 0)]
    code += [0x00000013] * 128
    code += [0x0000006F]
    return code


programs = {}
programs["mul_family_all"] = finish([
    addi(1, 0, -7), addi(2, 0, 9), m(3, 1, 2, 0), m(4, 1, 2, 1),
    m(5, 1, 2, 2), m(6, 1, 2, 3), m(7, 1, 2, 0, True)])
programs["div_family_all"] = finish([
    addi(1, 0, 123), addi(2, 0, 0), m(3, 1, 2, 4), m(4, 1, 2, 5),
    m(5, 1, 2, 6), m(6, 1, 2, 7), m(7, 1, 2, 4, True),
    m(8, 1, 2, 5, True), m(9, 1, 2, 6, True), m(10, 1, 2, 7, True)])
programs["rv64m_all_13"] = finish([
    addi(1, 0, 100), addi(2, 0, 7),
    m(3, 1, 2, 0), m(4, 1, 2, 1), m(5, 1, 2, 2), m(6, 1, 2, 3),
    m(7, 1, 2, 0, True), addi(2, 0, 0), m(8, 1, 2, 4), m(9, 1, 2, 5),
    m(10, 1, 2, 6), m(11, 1, 2, 7), m(12, 1, 2, 4, True),
    m(13, 1, 2, 5, True), m(14, 1, 2, 6, True), m(15, 1, 2, 7, True)])
programs["mul_div_dependency"] = finish([
    addi(1, 0, 6), addi(2, 0, 7), m(3, 1, 2, 0), addi(4, 0, 3),
    m(5, 3, 4, 4), addi(6, 0, 5), m(7, 5, 6, 0)])
programs["div_mul_dependency"] = finish([
    addi(1, 0, 100), addi(2, 0, 7), m(3, 1, 2, 4), addi(4, 0, 9),
    m(5, 3, 4, 0), m(6, 5, 2, 6)])
programs["high_multiply_mix"] = finish([
    addi(1, 0, -2), addi(2, 0, 3), m(3, 1, 2, 1), m(4, 1, 2, 2),
    m(5, 1, 2, 3), m(6, 1, 2, 0)])
programs["word_multiply_divide_mix"] = finish([
    addi(1, 0, -100), addi(2, 0, 7), m(3, 1, 2, 0, True),
    m(4, 1, 2, 4, True), m(5, 1, 2, 5, True),
    m(6, 1, 2, 6, True), m(7, 1, 2, 7, True)])
programs["divide_by_zero_mix"] = finish([
    addi(1, 0, 123), addi(2, 0, 0), m(3, 1, 2, 4), m(4, 1, 2, 5),
    m(5, 1, 2, 6), m(6, 1, 2, 7), m(7, 1, 2, 4, True),
    m(8, 1, 2, 7, True)])
programs["signed_overflow_mix"] = finish([
    addi(1, 0, 1), slli(1, 1, 63), addi(2, 0, -1), m(3, 1, 2, 4),
    m(4, 1, 2, 6), m(5, 1, 2, 4, True), m(6, 1, 2, 6, True)])
programs["rv64m_branch_mix"] = finish([
    addi(1, 0, 1), addi(2, 0, 1), beq(1, 2, 8), m(20, 1, 0, 4),
    addi(3, 0, 81), addi(4, 0, 9), m(5, 3, 4, 4), m(6, 5, 4, 0)])

load_code = absolute(20, 0x100) + [addi(1, 0, 84), sd(1, 20, 0), ld(2, 20, 0), addi(3, 0, 7), m(4, 2, 3, 4)]
programs["rv64m_load_mix"] = finish(load_code)
store_code = [addi(1, 0, 9), addi(2, 0, 11), m(3, 1, 2, 0)] + absolute(20, 0x108) + [sd(3, 20, 0), ld(4, 20, 0)]
programs["rv64m_store_mix"] = finish(store_code)
programs["rv64m_reset_replay"] = finish([
    addi(1, 0, 120), addi(2, 0, 5), m(3, 1, 2, 4), m(4, 3, 2, 0)])
rob_code = [addi(1, 0, 0)]
rob_code += [addi(1, 1, 1) for _ in range(40)]
rob_code += [addi(2, 0, 7), m(3, 1, 2, 0), m(4, 3, 2, 4)]
programs["rv64m_rob_wrap"] = finish(rob_code)
programs["rv64m_tohost_stress"] = finish([
    addi(1, 0, 144), addi(2, 0, 12), m(3, 1, 2, 4), m(4, 3, 2, 0)], True)

OUT.mkdir(parents=True, exist_ok=True)
for name, code in programs.items():
    if len(code) & 1:
        code.insert(-1, 0x00000013)
    packed = [f"{code[i + 1]:08x}{code[i]:08x}" for i in range(0, len(code), 2)]
    (OUT / f"{name}.hex").write_text("\n".join(packed) + "\n", encoding="ascii")

print(f"generated {len(programs)} M3C programs in {OUT}")
