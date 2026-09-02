#include "predecode.hpp"
#include "rvc.hpp"
#include "boom_state.hpp"

#include <cstdint>
#include <cstdio>

namespace boom { void decode_module(BoomCoreState& state); }

static uint32_t encode_b(int32_t imm, uint32_t rs1, uint32_t rs2, uint32_t f3) {
    const uint32_t x = static_cast<uint32_t>(imm) & 0x1fffu;
    return (((x >> 12) & 1u) << 31) | (((x >> 5) & 0x3fu) << 25) |
           (rs2 << 20) | (rs1 << 15) | (f3 << 12) |
           (((x >> 1) & 0xfu) << 8) | (((x >> 11) & 1u) << 7) | 0x63u;
}

static uint32_t encode_j(int32_t imm, uint32_t rd) {
    const uint32_t x = static_cast<uint32_t>(imm) & 0x1fffffu;
    return (((x >> 20) & 1u) << 31) | (((x >> 1) & 0x3ffu) << 21) |
           (((x >> 11) & 1u) << 20) | (((x >> 12) & 0xffu) << 12) |
           (rd << 7) | 0x6fu;
}

static uint32_t encode_i(int32_t imm, uint32_t rs1, uint32_t f3,
                         uint32_t rd, uint32_t opcode) {
    return ((static_cast<uint32_t>(imm) & 0xfffu) << 20) | (rs1 << 15) |
           (f3 << 12) | (rd << 7) | opcode;
}

struct Checker {
    unsigned checks;
    unsigned failures;
    Checker() : checks(0), failures(0) {}
    void expect(bool condition, const char* label) {
        ++checks;
        if (!condition) {
            if (failures < 20) std::printf("FAIL,%s\n", label);
            ++failures;
        }
    }
};

static bool compressed_cfi(uint16_t c, uint8_t& type, bool& call, bool& ret) {
    type = boom::CFI_NONE; call = false; ret = false;
    if ((c & 0xe003u) == 0xa001u) { type = boom::CFI_JAL; return true; }
    if ((c & 0xe003u) == 0xc001u || (c & 0xe003u) == 0xe001u) {
        type = boom::CFI_CONDITIONAL_BRANCH; return true;
    }
    if ((c & 0xe003u) != 0x8002u) return false;
    const uint32_t rd = (c >> 7) & 0x1fu;
    const uint32_t rs2 = (c >> 2) & 0x1fu;
    if (rd == 0 || rs2 != 0) return false;
    type = boom::CFI_JALR;
    if ((c & 0x1000u) != 0) call = true;
    else ret = rd == 1u || rd == 5u;
    return true;
}

int main() {
    Checker c;
    const uint32_t branch_f3[] = {0, 1, 4, 5, 6, 7};
    const int32_t branch_imm[] = {-4096, -254, -2, 0, 2, 126, 4094};
    const uint64_t pcs[] = {0, 2, 0x80000000ull, 0xfffffffffffffffeull};
    for (unsigned f = 0; f < 6; ++f) {
        for (unsigned i = 0; i < 7; ++i) {
            for (unsigned p = 0; p < 4; ++p) {
                const uint32_t inst = encode_b(branch_imm[i], (i + f) & 31u,
                                               (i * 7 + p) & 31u, branch_f3[f]);
                const boom::CfiPredecodeResult r = boom::predecode_cfi(pcs[p], inst, false);
                c.expect(r.valid && r.is_cfi && r.is_conditional, "branch_class");
                c.expect(r.cfi_type == boom::CFI_CONDITIONAL_BRANCH &&
                         !r.is_jal && !r.is_jalr, "branch_flags");
                c.expect(r.static_target_valid &&
                         r.static_target == pcs[p] + static_cast<uint64_t>(branch_imm[i]),
                         "branch_target");
                c.expect(!r.is_call && !r.is_return && r.instruction_length == 4,
                         "branch_metadata");
            }
        }
    }

    const int32_t jal_imm[] = {-1048576, -4096, -2, 0, 2, 4094, 1048574};
    const uint32_t jal_rd[] = {0, 1, 5, 7, 31};
    for (unsigned i = 0; i < 7; ++i) {
        for (unsigned d = 0; d < 5; ++d) {
            const uint64_t pc = pcs[(i + d) & 3u];
            const boom::CfiPredecodeResult r =
                boom::predecode_cfi(pc, encode_j(jal_imm[i], jal_rd[d]), false);
            c.expect(r.is_cfi && r.is_jal && r.cfi_type == boom::CFI_JAL,
                     "jal_class");
            c.expect(r.static_target_valid &&
                     r.static_target == pc + static_cast<uint64_t>(jal_imm[i]),
                     "jal_target");
            c.expect(r.is_call == (jal_rd[d] == 1 || jal_rd[d] == 5), "jal_call");
            c.expect(!r.is_return && !r.is_conditional && !r.is_jalr, "jal_flags");
        }
    }

    const int32_t jalr_imm[] = {-2048, -1, 0, 1, 2047};
    const uint32_t regs[][2] = {{0, 1}, {0, 5}, {1, 2}, {5, 3}, {0, 7}, {9, 11}};
    for (unsigned i = 0; i < 5; ++i) {
        for (unsigned q = 0; q < 6; ++q) {
            const uint32_t rd = regs[q][0], rs1 = regs[q][1];
            const boom::CfiPredecodeResult r = boom::predecode_cfi(
                pcs[q & 3u], encode_i(jalr_imm[i], rs1, 0, rd, 0x67), false);
            c.expect(r.is_cfi && r.is_jalr && r.cfi_type == boom::CFI_JALR,
                     "jalr_class");
            c.expect(!r.static_target_valid && r.static_target == 0, "jalr_no_target");
            c.expect(r.is_call == (rd == 1 || rd == 5), "jalr_call");
            c.expect(r.is_return == (rd == 0 && (rs1 == 1 || rs1 == 5) &&
                                      jalr_imm[i] == 0), "jalr_return");
        }
    }

    const uint32_t non_cfi[] = {
        0x00100093u, 0x002081b3u, 0x022081b3u, 0x0220c1b3u,
        0x0000b103u, 0x0020b023u, 0x123452b7u, 0x12345297u,
        0x00000073u, 0x00100073u, 0xffffffffu,
        encode_b(2, 1, 2, 2), encode_b(-2, 1, 2, 3),
        encode_i(0, 1, 1, 0, 0x67)
    };
    for (unsigned i = 0; i < sizeof(non_cfi) / sizeof(non_cfi[0]); ++i) {
        const boom::CfiPredecodeResult r = boom::predecode_cfi(0x1000, non_cfi[i], false);
        c.expect(!r.is_cfi && r.cfi_type == boom::CFI_NONE, "non_cfi");
        c.expect(!r.static_target_valid, "non_cfi_target");
    }

    const uint16_t directed_rvc[] = {0xc001, 0xe001, 0xa001, 0x8082, 0x8282,
                                     0x9082, 0x9282, 0x9002, 0x8001};
    for (unsigned i = 0; i < sizeof(directed_rvc) / sizeof(directed_rvc[0]); ++i) {
        const boom::RvcDecodeResult d = boom::decompress_rvc(directed_rvc[i]);
        uint8_t type; bool call, ret;
        const bool expected_cfi = compressed_cfi(directed_rvc[i], type, call, ret);
        const boom::CfiPredecodeResult r = boom::predecode_cfi(0x2002, d.instruction, true);
        c.expect(d.legal && r.is_cfi == expected_cfi, "rvc_class");
        c.expect(r.instruction_length == 2, "rvc_length");
        c.expect(!expected_cfi || (r.cfi_type == type && r.is_call == call &&
                                   r.is_return == ret), "rvc_metadata");
        if (expected_cfi && r.static_target_valid) {
            c.expect((r.static_target & 1u) == 0, "rvc_target_alignment");
        }
    }

    unsigned legal = 0, beqz = 0, bnez = 0, jump = 0, jr = 0, jalr = 0;
    unsigned false_positive = 0, exhaustive_errors = 0, decode_cross = 0;
    for (uint32_t raw = 0; raw <= 0xffffu; ++raw) {
        const uint16_t compressed = static_cast<uint16_t>(raw);
        const boom::RvcDecodeResult d = boom::decompress_rvc(compressed);
        if (!d.valid || !d.legal) continue;
        ++legal;
        uint8_t expected_type; bool expected_call, expected_return;
        const bool expected_cfi = compressed_cfi(compressed, expected_type,
                                                 expected_call, expected_return);
        const uint64_t pc = 0x80000000ull + (static_cast<uint64_t>(raw) << 1);
        const boom::CfiPredecodeResult r = boom::predecode_cfi(pc, d.instruction, true);
        bool ok = r.is_cfi == expected_cfi && r.instruction_length == 2;
        if (expected_cfi) {
            ok = ok && r.cfi_type == expected_type && r.is_call == expected_call &&
                 r.is_return == expected_return;
            if ((compressed & 0xe003u) == 0xc001u) ++beqz;
            else if ((compressed & 0xe003u) == 0xe001u) ++bnez;
            else if ((compressed & 0xe003u) == 0xa001u) ++jump;
            else if ((compressed & 0x1000u) != 0) ++jalr;
            else ++jr;

            BoomCoreState state;
            state.frontend.fetch_packet_valid = true;
            state.frontend.fetch_uop.inst = d.instruction;
            state.frontend.fetch_uop.debug_pc = pc;
            state.frontend.fetch_uop.is_rvc = true;
            boom::decode_module(state);
            const MicroOp& u = state.decode.dec_uops[0];
            const bool decode_kind = r.is_conditional ? u.branch.is_br :
                                     (r.is_jal ? u.branch.is_jal : u.branch.is_jalr);
            ok = ok && state.decode.dec_valids[0] && decode_kind &&
                 u.debug_pc == pc && u.is_rvc;
            ++decode_cross;
        } else if (r.is_cfi) {
            ++false_positive;
        }
        if (!ok) ++exhaustive_errors;
    }
    std::printf("RVC_PREDECODE_EXHAUSTIVE,total=65536,legal=%u,C_BEQZ=%u,C_BNEZ=%u,C_J=%u,C_JR=%u,C_JALR=%u,non_cfi_false_positive=%u,decode_cross=%u,errors=%u\n",
                legal, beqz, bnez, jump, jr, jalr, false_positive, decode_cross,
                exhaustive_errors);
    c.expect(beqz == 2048 && bnez == 2048 && jump == 2048, "rvc_count_direct");
    c.expect(jr == 31 && jalr == 31, "rvc_count_indirect");
    c.expect(false_positive == 0 && exhaustive_errors == 0, "rvc_exhaustive");
    c.expect(decode_cross >= 1000, "decode_cross_count");

    std::printf("PREDECODE_DIRECTED,checks=%u,failures=%u\n", c.checks, c.failures);
    if (c.failures != 0 || c.checks < 250) return 1;
    std::printf("GATE5_4_P1_PREDECODE_DIRECTED_PASS\n");
    return 0;
}
