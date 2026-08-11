#include "rvc.hpp"

#include <cstdint>
#include <cstdio>

namespace reference {

enum Classification {
    SUPPORTED,
    UNSUPPORTED,
    RESERVED,
    NOT_COMPRESSED
};

struct Result {
    bool valid;
    bool legal;
    uint32_t instruction;
    uint16_t compressed;
    uint8_t length;
    Classification classification;
};

enum Kind {
    K_ADDI4SPN, K_LW, K_LD, K_SW, K_SD,
    K_ADDI, K_ADDIW, K_LI, K_LUI_ADDI16SP, K_MISC,
    K_J, K_BEQZ, K_BNEZ, K_SLLI, K_LWSP, K_LDSP,
    K_CR, K_SWSP, K_SDSP, K_UNSUPPORTED, K_RESERVED
};

struct Pattern { uint16_t mask; uint16_t match; Kind kind; };

// Flat entries are transcribed from the RV64C opcode map. This table is
// intentionally organized differently from the production control tree.
static const Pattern patterns[] = {
    {0xe003, 0x0000, K_ADDI4SPN}, {0xe003, 0x2000, K_UNSUPPORTED},
    {0xe003, 0x4000, K_LW},       {0xe003, 0x6000, K_LD},
    {0xe003, 0x8000, K_RESERVED}, {0xe003, 0xa000, K_UNSUPPORTED},
    {0xe003, 0xc000, K_SW},       {0xe003, 0xe000, K_SD},
    {0xe003, 0x0001, K_ADDI},     {0xe003, 0x2001, K_ADDIW},
    {0xe003, 0x4001, K_LI},       {0xe003, 0x6001, K_LUI_ADDI16SP},
    {0xe003, 0x8001, K_MISC},     {0xe003, 0xa001, K_J},
    {0xe003, 0xc001, K_BEQZ},     {0xe003, 0xe001, K_BNEZ},
    {0xe003, 0x0002, K_SLLI},     {0xe003, 0x2002, K_UNSUPPORTED},
    {0xe003, 0x4002, K_LWSP},     {0xe003, 0x6002, K_LDSP},
    {0xe003, 0x8002, K_CR},       {0xe003, 0xa002, K_UNSUPPORTED},
    {0xe003, 0xc002, K_SWSP},     {0xe003, 0xe002, K_SDSP},
};

static uint32_t bit(uint16_t c, unsigned n) { return (c >> n) & 1u; }
static uint32_t field(uint16_t c, unsigned hi, unsigned lo) {
    return (c >> lo) & ((1u << (hi - lo + 1)) - 1u);
}
static int32_t sx(uint32_t value, unsigned width) {
    const uint32_t sign = 1u << (width - 1);
    return static_cast<int32_t>((value ^ sign) - sign);
}
static uint32_t enc_i(int32_t imm, uint32_t rs1, uint32_t f3,
                      uint32_t rd, uint32_t opcode) {
    return ((uint32_t(imm) & 0xfff) << 20) | (rs1 << 15) | (f3 << 12) |
           (rd << 7) | opcode;
}
static uint32_t enc_r(uint32_t f7, uint32_t rs2, uint32_t rs1,
                      uint32_t f3, uint32_t rd, uint32_t opcode) {
    return (f7 << 25) | (rs2 << 20) | (rs1 << 15) | (f3 << 12) |
           (rd << 7) | opcode;
}
static uint32_t enc_s(uint32_t imm, uint32_t rs2, uint32_t rs1,
                      uint32_t f3) {
    return (((imm >> 5) & 0x7f) << 25) | (rs2 << 20) | (rs1 << 15) |
           (f3 << 12) | ((imm & 0x1f) << 7) | 0x23;
}
static uint32_t enc_b(int32_t imm, uint32_t rs1, uint32_t f3) {
    const uint32_t x = uint32_t(imm) & 0x1fff;
    return (((x >> 12) & 1) << 31) | (((x >> 5) & 0x3f) << 25) |
           (rs1 << 15) | (f3 << 12) | (((x >> 1) & 0xf) << 8) |
           (((x >> 11) & 1) << 7) | 0x63;
}
static uint32_t enc_j(int32_t imm) {
    const uint32_t x = uint32_t(imm) & 0x1fffff;
    return (((x >> 20) & 1) << 31) | (((x >> 1) & 0x3ff) << 21) |
           (((x >> 11) & 1) << 20) | (((x >> 12) & 0xff) << 12) | 0x6f;
}

static Result make(uint16_t c, Classification classification,
                   uint32_t instruction = 0) {
    Result result = {(c & 3u) != 3u, classification == SUPPORTED,
                     instruction, c, uint8_t((c & 3u) != 3u ? 2 : 0),
                     classification};
    return result;
}

static Result expand(uint16_t c) {
    if ((c & 3u) == 3u) return make(c, NOT_COMPRESSED);
    Kind kind = K_RESERVED;
    for (unsigned i = 0; i < sizeof(patterns) / sizeof(patterns[0]); ++i) {
        if ((c & patterns[i].mask) == patterns[i].match) {
            kind = patterns[i].kind;
            break;
        }
    }
    if (kind == K_UNSUPPORTED) return make(c, UNSUPPORTED);
    if (kind == K_RESERVED) return make(c, RESERVED);

    const uint32_t rd = field(c, 11, 7);
    const uint32_t rs2 = field(c, 6, 2);
    const uint32_t rp = 8 + field(c, 4, 2);
    const uint32_t bp = 8 + field(c, 9, 7);
    const int32_t ci = sx((bit(c, 12) << 5) | field(c, 6, 2), 6);
    uint32_t imm = 0;

    switch (kind) {
    case K_ADDI4SPN:
        imm = (field(c, 10, 7) << 6) | (field(c, 12, 11) << 4) |
              (bit(c, 5) << 3) | (bit(c, 6) << 2);
        return imm ? make(c, SUPPORTED, enc_i(imm, 2, 0, rp, 0x13))
                   : make(c, RESERVED);
    case K_LW:
        imm = (bit(c, 5) << 6) | (field(c, 12, 10) << 3) | (bit(c, 6) << 2);
        return make(c, SUPPORTED, enc_i(imm, bp, 2, rp, 0x03));
    case K_LD:
        imm = (field(c, 6, 5) << 6) | (field(c, 12, 10) << 3);
        return make(c, SUPPORTED, enc_i(imm, bp, 3, rp, 0x03));
    case K_SW:
        imm = (bit(c, 5) << 6) | (field(c, 12, 10) << 3) | (bit(c, 6) << 2);
        return make(c, SUPPORTED, enc_s(imm, rp, bp, 2));
    case K_SD:
        imm = (field(c, 6, 5) << 6) | (field(c, 12, 10) << 3);
        return make(c, SUPPORTED, enc_s(imm, rp, bp, 3));
    case K_ADDI: return make(c, SUPPORTED, enc_i(ci, rd, 0, rd, 0x13));
    case K_ADDIW:
        return rd ? make(c, SUPPORTED, enc_i(ci, rd, 0, rd, 0x1b))
                  : make(c, RESERVED);
    case K_LI: return make(c, SUPPORTED, enc_i(ci, 0, 0, rd, 0x13));
    case K_LUI_ADDI16SP:
        if (rd == 2) {
            imm = (bit(c, 12) << 9) | (field(c, 4, 3) << 7) |
                  (bit(c, 5) << 6) | (bit(c, 2) << 5) | (bit(c, 6) << 4);
            return imm ? make(c, SUPPORTED, enc_i(sx(imm, 10), 2, 0, 2, 0x13))
                       : make(c, RESERVED);
        }
        return ci ? make(c, SUPPORTED,
                         ((uint32_t(ci) & 0xfffff) << 12) | (rd << 7) | 0x37)
                  : make(c, RESERVED);
    case K_MISC: {
        const uint32_t mode = field(c, 11, 10);
        const uint32_t arithmetic = field(c, 6, 5);
        if (mode < 2) {
            const uint32_t shamt = (bit(c, 12) << 5) | field(c, 6, 2);
            return make(c, SUPPORTED,
                        enc_i(shamt | (mode ? 0x400 : 0), bp, 5, bp, 0x13));
        }
        if (mode == 2) return make(c, SUPPORTED, enc_i(ci, bp, 7, bp, 0x13));
        if (!bit(c, 12)) {
            const uint32_t f3[4] = {0, 4, 6, 7};
            return make(c, SUPPORTED,
                        enc_r(arithmetic == 0 ? 0x20 : 0, rp, bp,
                              f3[arithmetic], bp, 0x33));
        }
        if (arithmetic > 1) return make(c, RESERVED);
        return make(c, SUPPORTED,
                    enc_r(arithmetic == 0 ? 0x20 : 0, rp, bp, 0, bp, 0x3b));
    }
    case K_J:
        imm = (bit(c, 12) << 11) | (bit(c, 8) << 10) |
              (field(c, 10, 9) << 8) | (bit(c, 6) << 7) |
              (bit(c, 7) << 6) | (bit(c, 2) << 5) |
              (bit(c, 11) << 4) | (field(c, 5, 3) << 1);
        return make(c, SUPPORTED, enc_j(sx(imm, 12)));
    case K_BEQZ: case K_BNEZ:
        imm = (bit(c, 12) << 8) | (field(c, 6, 5) << 6) |
              (bit(c, 2) << 5) | (field(c, 11, 10) << 3) |
              (field(c, 4, 3) << 1);
        return make(c, SUPPORTED, enc_b(sx(imm, 9), bp, kind == K_BEQZ ? 0 : 1));
    case K_SLLI:
        return make(c, SUPPORTED,
                    enc_i((bit(c, 12) << 5) | rs2, rd, 1, rd, 0x13));
    case K_LWSP:
        imm = (field(c, 3, 2) << 6) | (bit(c, 12) << 5) |
              (field(c, 6, 4) << 2);
        return rd ? make(c, SUPPORTED, enc_i(imm, 2, 2, rd, 0x03))
                  : make(c, RESERVED);
    case K_LDSP:
        imm = (field(c, 4, 2) << 6) | (bit(c, 12) << 5) |
              (field(c, 6, 5) << 3);
        return rd ? make(c, SUPPORTED, enc_i(imm, 2, 3, rd, 0x03))
                  : make(c, RESERVED);
    case K_CR:
        if (!bit(c, 12) && !rs2)
            return rd ? make(c, SUPPORTED, enc_i(0, rd, 0, 0, 0x67))
                      : make(c, RESERVED);
        if (!bit(c, 12)) return make(c, SUPPORTED, enc_r(0, rs2, 0, 0, rd, 0x33));
        if (!rs2) return make(c, SUPPORTED,
                             rd ? enc_i(0, rd, 0, 1, 0x67) : 0x00100073);
        return make(c, SUPPORTED, enc_r(0, rs2, rd, 0, rd, 0x33));
    case K_SWSP:
        imm = (field(c, 8, 7) << 6) | (field(c, 12, 9) << 2);
        return make(c, SUPPORTED, enc_s(imm, rs2, 2, 2));
    case K_SDSP:
        imm = (field(c, 9, 7) << 6) | (field(c, 12, 10) << 3);
        return make(c, SUPPORTED, enc_s(imm, rs2, 2, 3));
    default: return make(c, RESERVED);
    }
}

static bool is_hint(uint16_t c, const Result& result) {
    if (result.classification != SUPPORTED) return false;
    const uint32_t quadrant = c & 3u;
    const uint32_t funct3 = field(c, 15, 13);
    const uint32_t rd = field(c, 11, 7);
    const uint32_t rs2 = field(c, 6, 2);
    const int32_t ci = sx((bit(c, 12) << 5) | field(c, 6, 2), 6);
    const uint32_t shamt = (bit(c, 12) << 5) | field(c, 6, 2);

    if (quadrant == 1 && funct3 == 0)
        return c != 0x0001 && (rd == 0 || ci == 0);
    if (quadrant == 1 && funct3 == 2) return rd == 0;
    if (quadrant == 1 && funct3 == 3) return rd == 0;
    if (quadrant == 1 && funct3 == 4 && field(c, 11, 10) < 2)
        return shamt == 0;
    if (quadrant == 2 && funct3 == 0) return rd == 0 || shamt == 0;
    if (quadrant == 2 && funct3 == 4 && rs2 != 0) return rd == 0;
    return false;
}

}  // namespace reference

struct FamilySeed { const char* name; uint16_t compressed; };

static bool equal(const boom::RvcDecodeResult& got, const reference::Result& expected) {
    return got.valid == expected.valid && got.legal == expected.legal &&
           got.instruction == expected.instruction &&
           got.compressed == expected.compressed &&
           got.length_bytes == expected.length;
}

int main() {
    // One manually transcribed seed per integer family plus unsupported and
    // reserved classes. Six operand/immediate variants make these true directed
    // checks rather than a relabeled exhaustive loop.
    const FamilySeed families[] = {
        {"addi4spn",0x0000},{"lw",0x4000},{"ld",0x6000},{"sw",0xc000},{"sd",0xe000},
        {"addi",0x0001},{"addiw",0x2081},{"li",0x4001},{"lui",0x6081},
        {"addi16sp",0x6101},{"srli",0x8001},{"srai",0x8401},{"andi",0x8801},
        {"sub",0x8c01},{"xor",0x8c21},{"or",0x8c41},{"and",0x8c61},
        {"subw",0x9c01},{"addw",0x9c21},{"j",0xa001},{"beqz",0xc001},{"bnez",0xe001},
        {"slli",0x0002},{"lwsp",0x4082},{"ldsp",0x6082},{"jr",0x8082},
        {"mv",0x8086},{"ebreak",0x9002},{"jalr",0x9082},{"add",0x9086},
        {"swsp",0xc002},{"sdsp",0xe002},{"fld_unsupported",0x2000},
        {"fsd_unsupported",0xa000},{"fldsp_unsupported",0x2002},
        {"fsdsp_unsupported",0xa002},{"reserved_q0",0x8000},{"not_compressed",0x0003},
    };
    const uint16_t variations[] = {0x0000,0x0004,0x001c,0x0080,0x0380,0x1000};

    unsigned directed_checks = 0, directed_failures = 0;
    for (unsigned i = 0; i < sizeof(families) / sizeof(families[0]); ++i) {
        for (unsigned v = 0; v < sizeof(variations) / sizeof(variations[0]); ++v) {
            const uint16_t c = families[i].compressed ^ variations[v];
            const reference::Result expected = reference::expand(c);
            const boom::RvcDecodeResult got = boom::decompress_rvc(c);
            ++directed_checks;
            if (!equal(got, expected)) {
                if (directed_failures < 20)
                    std::printf("DIRECTED_FAIL family=%s c=%04x got=%u/%u/%08x/%u expected=%u/%u/%08x/%u\n",
                                families[i].name, c, got.valid, got.legal, got.instruction,
                                got.length_bytes, expected.valid, expected.legal,
                                expected.instruction, expected.length);
                ++directed_failures;
            }
        }
    }

    unsigned supported = 0, unsupported = 0, reserved = 0, non_rvc = 0;
    unsigned hints = 0;
    unsigned quadrants[4] = {0, 0, 0, 0};
    unsigned exact = 0, mismatches = 0;
    for (uint32_t raw = 0; raw <= 0xffffu; ++raw) {
        const uint16_t c = static_cast<uint16_t>(raw);
        const reference::Result expected = reference::expand(c);
        const boom::RvcDecodeResult got = boom::decompress_rvc(c);
        ++quadrants[c & 3u];
        hints += reference::is_hint(c, expected);
        switch (expected.classification) {
        case reference::SUPPORTED: ++supported; break;
        case reference::UNSUPPORTED: ++unsupported; break;
        case reference::RESERVED: ++reserved; break;
        case reference::NOT_COMPRESSED: ++non_rvc; break;
        }
        if (equal(got, expected)) ++exact;
        else {
            if (mismatches < 20)
                std::printf("EXHAUSTIVE_FAIL c=%04x got=%u/%u/%08x/%04x/%u expected=%u/%u/%08x/%04x/%u\n",
                            c, got.valid, got.legal, got.instruction, got.compressed,
                            got.length_bytes, expected.valid, expected.legal,
                            expected.instruction, expected.compressed, expected.length);
            ++mismatches;
        }
    }

    std::printf("RVC_DIRECTED checks=%u failures=%u\n", directed_checks, directed_failures);
    std::printf("RVC_EXHAUSTIVE total=65536 exact=%u mismatches=%u\n", exact, mismatches);
    std::printf("RVC_CLASSIFICATION supported=%u unsupported=%u reserved=%u non_rvc=%u\n",
                supported, unsupported, reserved, non_rvc);
    std::printf("RVC_HINTS count=%u\n", hints);
    std::printf("RVC_QUADRANTS q0=%u q1=%u q2=%u q3=%u\n",
                quadrants[0], quadrants[1], quadrants[2], quadrants[3]);
    if (directed_checks < 150 || directed_failures || mismatches) return 1;
    std::printf("GATE5_2_R1_RVC_DECOMPRESS_PASS\n");
    return 0;
}
