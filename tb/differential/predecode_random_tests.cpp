#include "predecode.hpp"

#include <cstdint>
#include <cstdio>

static uint64_t rng_state = 0x54cf1123456789abull;
static uint32_t random32() {
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 7;
    rng_state ^= rng_state << 17;
    return static_cast<uint32_t>(rng_state ^ (rng_state >> 32));
}

static int64_t sx(uint32_t value, unsigned width) {
    const uint64_t sign = uint64_t(1) << (width - 1);
    return static_cast<int64_t>((static_cast<uint64_t>(value) ^ sign) - sign);
}

static int64_t ref_b_imm(uint32_t inst) {
    const uint32_t value = ((inst >> 31) << 12) | (((inst >> 7) & 1u) << 11) |
        (((inst >> 25) & 0x3fu) << 5) | (((inst >> 8) & 0xfu) << 1);
    return sx(value, 13);
}

static int64_t ref_j_imm(uint32_t inst) {
    const uint32_t value = ((inst >> 31) << 20) | (((inst >> 12) & 0xffu) << 12) |
        (((inst >> 20) & 1u) << 11) | (((inst >> 21) & 0x3ffu) << 1);
    return sx(value, 21);
}

struct Ref {
    bool cfi, cond, jal, jalr, call, ret, target_valid;
    uint8_t type;
    uint64_t target;
};

static Ref reference(uint64_t pc, uint32_t inst) {
    Ref r = {false, false, false, false, false, false, false,
             boom::CFI_NONE, 0};
    const uint32_t op = inst & 0x7fu, f3 = (inst >> 12) & 7u;
    const uint32_t rd = (inst >> 7) & 31u, rs1 = (inst >> 15) & 31u;
    const bool link = rd == 1u || rd == 5u;
    if (op == 0x63u && (f3 == 0 || f3 == 1 || f3 >= 4)) {
        r.cfi = r.cond = r.target_valid = true;
        r.type = boom::CFI_CONDITIONAL_BRANCH;
        r.target = pc + static_cast<uint64_t>(ref_b_imm(inst));
    } else if (op == 0x6fu) {
        r.cfi = r.jal = r.target_valid = true; r.call = link;
        r.type = boom::CFI_JAL;
        r.target = pc + static_cast<uint64_t>(ref_j_imm(inst));
    } else if (op == 0x67u && f3 == 0) {
        r.cfi = r.jalr = true; r.call = link; r.type = boom::CFI_JALR;
        r.ret = rd == 0 && (rs1 == 1 || rs1 == 5) && (inst >> 20) == 0;
    }
    return r;
}

static uint32_t encode_b(uint32_t raw, uint32_t rs1, uint32_t rs2, uint32_t f3) {
    raw &= 0x1fffu;
    return (((raw >> 12) & 1u) << 31) | (((raw >> 5) & 0x3fu) << 25) |
        (rs2 << 20) | (rs1 << 15) | (f3 << 12) |
        (((raw >> 1) & 0xfu) << 8) | (((raw >> 11) & 1u) << 7) | 0x63u;
}

static uint32_t encode_j(uint32_t raw, uint32_t rd) {
    raw &= 0x1fffffu;
    return (((raw >> 20) & 1u) << 31) | (((raw >> 1) & 0x3ffu) << 21) |
        (((raw >> 11) & 1u) << 20) | (((raw >> 12) & 0xffu) << 12) |
        (rd << 7) | 0x6fu;
}

static bool check_one(uint64_t pc, uint32_t inst, bool is_rvc,
                      uint64_t& class_errors, uint64_t& target_errors,
                      uint64_t& false_positives) {
    const Ref e = reference(pc, inst);
    const boom::CfiPredecodeResult g = boom::predecode_cfi(pc, inst, is_rvc);
    const bool classification = g.valid && g.is_cfi == e.cfi && g.cfi_type == e.type &&
        g.is_conditional == e.cond && g.is_jal == e.jal && g.is_jalr == e.jalr &&
        g.is_call == e.call && g.is_return == e.ret &&
        g.static_target_valid == e.target_valid &&
        g.instruction_length == (is_rvc ? 2 : 4);
    const bool target = !e.target_valid || g.static_target == e.target;
    if (!classification) ++class_errors;
    if (!target) ++target_errors;
    if (!e.cfi && g.is_cfi) ++false_positives;
    return classification && target;
}

int main() {
    uint64_t structured = 0, random_count = 0;
    uint64_t class_errors = 0, target_errors = 0, false_positives = 0;
    const uint32_t bpatterns[] = {0, 2, 0x7fe, 0x800, 0xffe, 0x1000, 0x1ffe};
    const uint32_t jpatterns[] = {0, 2, 0x7fe, 0x800, 0xffffe, 0x100000, 0x1ffffe};
    for (uint32_t f3 = 0; f3 < 8; ++f3)
        for (uint32_t rs1 = 0; rs1 < 32; ++rs1)
            for (uint32_t rs2 = 0; rs2 < 32; rs2 += 7)
                for (unsigned i = 0; i < 7; ++i, ++structured)
                    check_one(0xfffffffffffff000ull + structured,
                              encode_b(bpatterns[i], rs1, rs2, f3), false,
                              class_errors, target_errors, false_positives);
    for (uint32_t rd = 0; rd < 32; ++rd)
        for (unsigned i = 0; i < 7; ++i, ++structured)
            check_one(0x80000000ull + structured, encode_j(jpatterns[i], rd), false,
                      class_errors, target_errors, false_positives);
    for (uint32_t rd = 0; rd < 32; ++rd)
        for (uint32_t rs1 = 0; rs1 < 32; ++rs1)
            for (uint32_t f3 = 0; f3 < 8; ++f3, ++structured) {
                const uint32_t imm = (rd * 131u + rs1 * 17u + f3) & 0xfffu;
                const uint32_t inst = (imm << 20) | (rs1 << 15) | (f3 << 12) |
                                      (rd << 7) | 0x67u;
                check_one(structured << 1, inst, false, class_errors,
                          target_errors, false_positives);
            }
    for (; random_count < 1000000; ++random_count) {
        const uint32_t inst = random32();
        const uint64_t pc = (static_cast<uint64_t>(random32()) << 32) | random32();
        check_one(pc, inst, (random32() & 1u) != 0, class_errors,
                  target_errors, false_positives);
    }
    const uint64_t errors = class_errors + target_errors;
    std::printf("PREDECODE_RANDOM,structured=%llu,random=%llu,classification_error=%llu,target_error=%llu,false_positive=%llu,total_error=%llu\n",
                static_cast<unsigned long long>(structured),
                static_cast<unsigned long long>(random_count),
                static_cast<unsigned long long>(class_errors),
                static_cast<unsigned long long>(target_errors),
                static_cast<unsigned long long>(false_positives),
                static_cast<unsigned long long>(errors));
    if (errors != 0 || false_positives != 0) return 1;
    std::printf("GATE5_4_P1_PREDECODE_RANDOM_PASS\n");
    return 0;
}
