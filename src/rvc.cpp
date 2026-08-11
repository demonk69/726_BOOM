#include "rvc.hpp"

namespace boom {
namespace {

static uint32_t bit(uint16_t value, unsigned index) {
#pragma HLS INLINE
    return (value >> index) & 1u;
}

static uint32_t bits(uint16_t value, unsigned high, unsigned low) {
#pragma HLS INLINE
    return (value >> low) & ((1u << (high - low + 1)) - 1u);
}

static int32_t sign_extend(uint32_t value, unsigned width) {
#pragma HLS INLINE
    const uint32_t sign = 1u << (width - 1);
    return static_cast<int32_t>((value ^ sign) - sign);
}

static uint32_t encode_i(int32_t imm, uint32_t rs1, uint32_t funct3,
                         uint32_t rd, uint32_t opcode) {
#pragma HLS INLINE
    return ((static_cast<uint32_t>(imm) & 0xfffu) << 20) | (rs1 << 15) |
           (funct3 << 12) | (rd << 7) | opcode;
}

static uint32_t encode_r(uint32_t funct7, uint32_t rs2, uint32_t rs1,
                         uint32_t funct3, uint32_t rd, uint32_t opcode) {
#pragma HLS INLINE
    return (funct7 << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) |
           (rd << 7) | opcode;
}

static uint32_t encode_s(int32_t imm, uint32_t rs2, uint32_t rs1,
                         uint32_t funct3, uint32_t opcode) {
#pragma HLS INLINE
    const uint32_t uimm = static_cast<uint32_t>(imm) & 0xfffu;
    return ((uimm >> 5) << 25) | (rs2 << 20) | (rs1 << 15) |
           (funct3 << 12) | ((uimm & 0x1fu) << 7) | opcode;
}

static uint32_t encode_b(int32_t imm, uint32_t rs2, uint32_t rs1,
                         uint32_t funct3) {
#pragma HLS INLINE
    const uint32_t uimm = static_cast<uint32_t>(imm) & 0x1fffu;
    return (((uimm >> 12) & 1u) << 31) | (((uimm >> 5) & 0x3fu) << 25) |
           (rs2 << 20) | (rs1 << 15) | (funct3 << 12) |
           (((uimm >> 1) & 0xfu) << 8) | (((uimm >> 11) & 1u) << 7) |
           0x63u;
}

static uint32_t encode_j(int32_t imm, uint32_t rd) {
#pragma HLS INLINE
    const uint32_t uimm = static_cast<uint32_t>(imm) & 0x1fffffu;
    return (((uimm >> 20) & 1u) << 31) | (((uimm >> 1) & 0x3ffu) << 21) |
           (((uimm >> 11) & 1u) << 20) | (((uimm >> 12) & 0xffu) << 12) |
           (rd << 7) | 0x6fu;
}

static RvcDecodeResult legal(uint16_t compressed, uint32_t instruction) {
#pragma HLS INLINE
    RvcDecodeResult result(compressed);
    result.instruction = instruction;
    result.legal = true;
    return result;
}

}  // namespace

RvcDecodeResult decompress_rvc(uint16_t c) {
    RvcDecodeResult result(c);
    const uint32_t quadrant = c & 3u;
    const uint32_t funct3 = bits(c, 15, 13);
    const uint32_t rd = bits(c, 11, 7);
    const uint32_t rs2 = bits(c, 6, 2);
    const uint32_t rdp = 8u + bits(c, 4, 2);
    const uint32_t rs1p = 8u + bits(c, 9, 7);

    if (quadrant == 0) {
        if (funct3 == 0) {  // C.ADDI4SPN
            const uint32_t imm = (bits(c, 12, 11) << 4) |
                                 (bits(c, 10, 7) << 6) |
                                 (bit(c, 6) << 2) | (bit(c, 5) << 3);
            if (imm == 0) return result;
            return legal(c, encode_i(imm, 2, 0, rdp, 0x13));
        }
        if (funct3 == 1) return result;  // C.FLD is unsupported by this core.
        if (funct3 == 3) {  // C.LD
            const uint32_t imm = (bits(c, 12, 10) << 3) |
                                 (bits(c, 6, 5) << 6);
            return legal(c, encode_i(imm, rs1p, 3, rdp, 0x03));
        }
        if (funct3 == 2) {  // C.LW
            const uint32_t imm = (bits(c, 12, 10) << 3) |
                                 (bit(c, 6) << 2) | (bit(c, 5) << 6);
            return legal(c, encode_i(imm, rs1p, 2, rdp, 0x03));
        }
        if (funct3 == 5) return result;  // C.FSD is unsupported by this core.
        if (funct3 == 7) {  // C.SD
            const uint32_t imm = (bits(c, 12, 10) << 3) |
                                 (bits(c, 6, 5) << 6);
            return legal(c, encode_s(imm, rdp, rs1p, 3, 0x23));
        }
        if (funct3 == 6) {  // C.SW
            const uint32_t imm = (bits(c, 12, 10) << 3) |
                                 (bit(c, 6) << 2) | (bit(c, 5) << 6);
            return legal(c, encode_s(imm, rdp, rs1p, 2, 0x23));
        }
        return result;
    }

    if (quadrant == 1) {
        const int32_t ci_imm = sign_extend((bit(c, 12) << 5) | bits(c, 6, 2), 6);
        if (funct3 == 0)  // C.ADDI / C.NOP / HINT
            return legal(c, encode_i(ci_imm, rd, 0, rd, 0x13));
        if (funct3 == 1) {  // C.ADDIW
            if (rd == 0) return result;
            return legal(c, encode_i(ci_imm, rd, 0, rd, 0x1b));
        }
        if (funct3 == 2)  // C.LI / HINT
            return legal(c, encode_i(ci_imm, 0, 0, rd, 0x13));
        if (funct3 == 3) {
            if (rd == 2) {  // C.ADDI16SP
                const uint32_t raw = (bit(c, 12) << 9) | (bit(c, 6) << 4) |
                                     (bit(c, 5) << 6) | (bits(c, 4, 3) << 7) |
                                     (bit(c, 2) << 5);
                if (raw == 0) return result;
                return legal(c, encode_i(sign_extend(raw, 10), 2, 0, 2, 0x13));
            }
            if (ci_imm == 0) return result;
            return legal(c, ((static_cast<uint32_t>(ci_imm) & 0xfffffu) << 12) |
                            (rd << 7) | 0x37u);
        }
        if (funct3 == 4) {
            const uint32_t subop = bits(c, 11, 10);
            if (subop != 3) {
                if (subop == 2)  // C.ANDI
                    return legal(c, encode_i(ci_imm, rs1p, 7, rs1p, 0x13));
                const uint32_t shamt = (bit(c, 12) << 5) | bits(c, 6, 2);
                const int32_t imm = static_cast<int32_t>(shamt |
                    (subop == 1 ? 0x400u : 0u));
                return legal(c, encode_i(imm, rs1p, 5, rs1p, 0x13));
            }
            const uint32_t arithmetic = bits(c, 6, 5);
            if (bit(c, 12) == 0) {
                const uint32_t funct7 = arithmetic == 0 ? 0x20u : 0u;
                const uint32_t base_funct3[4] = {0, 4, 6, 7};
                return legal(c, encode_r(funct7, rdp, rs1p,
                                         base_funct3[arithmetic], rs1p, 0x33));
            }
            if (arithmetic > 1) return result;
            return legal(c, encode_r(arithmetic == 0 ? 0x20u : 0u, rdp, rs1p,
                                     0, rs1p, 0x3b));
        }
        if (funct3 == 5) {  // C.J
            const uint32_t raw = (bit(c, 12) << 11) | (bit(c, 11) << 4) |
                                 (bits(c, 10, 9) << 8) | (bit(c, 8) << 10) |
                                 (bit(c, 7) << 6) | (bit(c, 6) << 7) |
                                 (bits(c, 5, 3) << 1) | (bit(c, 2) << 5);
            return legal(c, encode_j(sign_extend(raw, 12), 0));
        }
        if (funct3 == 6 || funct3 == 7) {  // C.BEQZ / C.BNEZ
            const uint32_t raw = (bit(c, 12) << 8) | (bits(c, 11, 10) << 3) |
                                 (bits(c, 6, 5) << 6) | (bits(c, 4, 3) << 1) |
                                 (bit(c, 2) << 5);
            return legal(c, encode_b(sign_extend(raw, 9), 0, rs1p,
                                     funct3 == 6 ? 0 : 1));
        }
    }

    if (quadrant == 2) {
        const uint32_t shamt = (bit(c, 12) << 5) | bits(c, 6, 2);
        if (funct3 == 0)  // C.SLLI / HINT
            return legal(c, encode_i(shamt, rd, 1, rd, 0x13));
        if (funct3 == 1) return result;  // C.FLDSP is unsupported by this core.
        if (funct3 == 3) {  // C.LDSP
            const uint32_t imm = (bit(c, 12) << 5) | (bits(c, 6, 5) << 3) |
                                 (bits(c, 4, 2) << 6);
            if (rd == 0) return result;
            return legal(c, encode_i(imm, 2, 3, rd, 0x03));
        }
        if (funct3 == 2) {  // C.LWSP
            const uint32_t imm = (bit(c, 12) << 5) | (bits(c, 6, 4) << 2) |
                                 (bits(c, 3, 2) << 6);
            if (rd == 0) return result;
            return legal(c, encode_i(imm, 2, 2, rd, 0x03));
        }
        if (funct3 == 4) {
            if (bit(c, 12) == 0) {
                if (rs2 == 0) {  // C.JR
                    if (rd == 0) return result;
                    return legal(c, encode_i(0, rd, 0, 0, 0x67));
                }
                return legal(c, encode_r(0, rs2, 0, 0, rd, 0x33));  // C.MV/HINT
            }
            if (rs2 == 0) {
                if (rd == 0) return legal(c, 0x00100073u);  // C.EBREAK
                return legal(c, encode_i(0, rd, 0, 1, 0x67));  // C.JALR
            }
            return legal(c, encode_r(0, rs2, rd, 0, rd, 0x33));  // C.ADD/HINT
        }
        if (funct3 == 5) return result;  // C.FSDSP is unsupported by this core.
        if (funct3 == 7) {  // C.SDSP
            const uint32_t imm = (bits(c, 12, 10) << 3) |
                                 (bits(c, 9, 7) << 6);
            return legal(c, encode_s(imm, rs2, 2, 3, 0x23));
        }
        if (funct3 == 6) {  // C.SWSP
            const uint32_t imm = (bits(c, 12, 9) << 2) |
                                 (bits(c, 8, 7) << 6);
            return legal(c, encode_s(imm, rs2, 2, 2, 0x23));
        }
    }

    return result;
}

RvcDecodeResult decompress_rv64c(uint16_t compressed) {
    return decompress_rvc(compressed);
}

}  // namespace boom
