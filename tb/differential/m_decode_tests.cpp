#include "boom_state.hpp"
#include "issue.hpp"
#include <cstdint>
#include <cstdio>

namespace boom {
void decode_module(BoomCoreState& state);
}

struct DecodeVector {
    const char* name;
    uint8_t opcode;
    uint8_t funct3;
    uint8_t funct7;
    uint8_t uopc;
    uint8_t fu;
    bool word;
};

static int tests = 0;
static int failures = 0;

static uint32_t encode_r_type(uint8_t opcode, uint8_t rd, uint8_t funct3,
                              uint8_t rs1, uint8_t rs2, uint8_t funct7) {
    return ((uint32_t)funct7 << 25) | ((uint32_t)rs2 << 20) |
           ((uint32_t)rs1 << 15) | ((uint32_t)funct3 << 12) |
           ((uint32_t)rd << 7) | opcode;
}

static MicroOp decode(uint32_t inst, uint64_t pc) {
    BoomCoreState state;
    state.frontend.fetch_packet_valid = true;
    state.frontend.fetch_uop.inst = inst;
    state.frontend.fetch_uop.debug_pc = pc;
    boom::decode_module(state);
    if (!state.decode.dec_valids[0]) {
        std::printf("FAIL: decode did not produce a uop for 0x%08x\n", inst);
        failures++;
    }
    return state.decode.dec_uops[0];
}

static void check_legal(const DecodeVector& vector, uint8_t rd = 5) {
    const uint8_t rs1 = 6;
    const uint8_t rs2 = 7;
    const uint64_t pc = 0x80000120ull;
    const uint32_t inst = encode_r_type(vector.opcode, rd, vector.funct3,
                                        rs1, rs2, vector.funct7);
    const MicroOp uop = decode(inst, pc);
    const bool ok = uop.uopc == vector.uopc && uop.fu_code == vector.fu &&
        uop.iq_type == IQ_ALU && uop.ctrl.op1_sel == OP1_RS1 &&
        uop.ctrl.op2_sel == OP2_RS2 &&
        ((vector.fu != FU_MUL && vector.fu != FU_DIV) ||
         uop.ctrl.op_fcn == vector.funct3) &&
        uop.ctrl.fcn_dw == (vector.word ? 1 : 0) &&
        uop.rename.lrs1 == rs1 && uop.rename.lrs2 == rs2 &&
        uop.rename.ldst == rd && uop.rename.dst_rtype == DST_INT &&
        !uop.exception && uop.exc_cause == 0 && uop.inst == inst &&
        uop.debug_pc == pc &&
        ((vector.fu != FU_MUL && vector.fu != FU_DIV) ||
         classify_issue_port(uop) == ISSUE_PORT_INT);
    tests++;
    if (!ok) {
        failures++;
        std::printf("FAIL: legal vector %s rd=%u uopc=%u fu=%u exception=%u\n",
                    vector.name, rd, uop.uopc, uop.fu_code, uop.exception);
    }
}

static void check_illegal(const char* name, uint8_t opcode, uint8_t funct3,
                          uint8_t funct7) {
    const MicroOp uop = decode(encode_r_type(opcode, 5, funct3, 6, 7, funct7),
                               0x80000200ull);
    const bool ok = uop.uopc == 255 && uop.exception && uop.exc_cause == 2 &&
        uop.fu_code != FU_MUL && uop.fu_code != FU_DIV &&
        uop.rename.dst_rtype == DST_N &&
        classify_issue_port(uop) == ISSUE_PORT_UNSUPPORTED;
    tests++;
    if (!ok) {
        failures++;
        std::printf("FAIL: illegal vector %s uopc=%u fu=%u dst=%u cause=%llu\n",
                    name, uop.uopc, uop.fu_code, uop.rename.dst_rtype,
                    (unsigned long long)uop.exc_cause);
    }
}

int main() {
    const DecodeVector m[] = {
        {"MUL",    0x33, 0, 0x01, 16, FU_MUL, false},
        {"MULH",   0x33, 1, 0x01, 17, FU_MUL, false},
        {"MULHSU", 0x33, 2, 0x01, 18, FU_MUL, false},
        {"MULHU",  0x33, 3, 0x01, 19, FU_MUL, false},
        {"DIV",    0x33, 4, 0x01, 21, FU_DIV, false},
        {"DIVU",   0x33, 5, 0x01, 22, FU_DIV, false},
        {"REM",    0x33, 6, 0x01, 23, FU_DIV, false},
        {"REMU",   0x33, 7, 0x01, 24, FU_DIV, false},
        {"MULW",   0x3b, 0, 0x01, 20, FU_MUL, true},
        {"DIVW",   0x3b, 4, 0x01, 25, FU_DIV, true},
        {"DIVUW",  0x3b, 5, 0x01, 26, FU_DIV, true},
        {"REMW",   0x3b, 6, 0x01, 27, FU_DIV, true},
        {"REMUW",  0x3b, 7, 0x01, 28, FU_DIV, true},
    };
    for (unsigned i = 0; i < sizeof(m) / sizeof(m[0]); i++) check_legal(m[i]);

    check_legal(m[0], 0);
    check_legal(m[4], 0);
    check_legal(m[8], 0);
    check_legal(m[12], 0);

    check_illegal("OP f7=02 f3=000", 0x33, 0, 0x02);
    check_illegal("OP f7=20 f3=001", 0x33, 1, 0x20);
    check_illegal("OP f7=7f f3=100", 0x33, 4, 0x7f);
    check_illegal("OP32 reserved M f3=001", 0x3b, 1, 0x01);
    check_illegal("OP32 reserved M f3=010", 0x3b, 2, 0x01);
    check_illegal("OP32 reserved M f3=011", 0x3b, 3, 0x01);
    check_illegal("OP32 f7=02 f3=000", 0x3b, 0, 0x02);
    check_illegal("OP32 f7=20 f3=100", 0x3b, 4, 0x20);
    check_illegal("OP invalid ALU funct7", 0x33, 6, 0x20);
    check_illegal("OP32 invalid ALU funct7", 0x3b, 5, 0x7f);

    const DecodeVector rv64i[] = {
        {"ADD",  0x33, 0, 0x00, 1,  FU_ALU, false},
        {"SUB",  0x33, 0, 0x20, 2,  FU_ALU, false},
        {"SLL",  0x33, 1, 0x00, 3,  FU_ALU, false},
        {"SLT",  0x33, 2, 0x00, 4,  FU_ALU, false},
        {"SLTU", 0x33, 3, 0x00, 5,  FU_ALU, false},
        {"XOR",  0x33, 4, 0x00, 6,  FU_ALU, false},
        {"SRL",  0x33, 5, 0x00, 7,  FU_ALU, false},
        {"SRA",  0x33, 5, 0x20, 8,  FU_ALU, false},
        {"OR",   0x33, 6, 0x00, 9,  FU_ALU, false},
        {"AND",  0x33, 7, 0x00, 10, FU_ALU, false},
        {"ADDW", 0x3b, 0, 0x00, 11, FU_ALU, true},
        {"SUBW", 0x3b, 0, 0x20, 12, FU_ALU, true},
        {"SLLW", 0x3b, 1, 0x00, 13, FU_ALU, true},
        {"SRLW", 0x3b, 5, 0x00, 14, FU_ALU, true},
        {"SRAW", 0x3b, 5, 0x20, 15, FU_ALU, true},
    };
    for (unsigned i = 0; i < sizeof(rv64i) / sizeof(rv64i[0]); i++) check_legal(rv64i[i]);

    if (failures != 0) {
        std::printf("M1 RV64M decode: %d passed, %d failed\n", tests - failures, failures);
        return 1;
    }
    std::printf("M1 RV64M decode: %d passed, 0 failed\n", tests);
    return 0;
}
