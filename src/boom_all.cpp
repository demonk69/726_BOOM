#include "boom_config.hpp"
#include "boom_types.hpp"
#include "boom_state.hpp"
#include "boom_interfaces.hpp"
#include <cstdint>

#define UOPC_NOP  0
#define UOPC_ADD  1
#define UOPC_SUB  2
#define UOPC_SLL  3
#define UOPC_SLT  4
#define UOPC_SLTU 5
#define UOPC_XOR  6
#define UOPC_SRL  7
#define UOPC_SRA  8
#define UOPC_OR   9
#define UOPC_AND  10
#define UOPC_ADDW 11
#define UOPC_SUBW 12
#define UOPC_SLLW 13
#define UOPC_SRLW 14
#define UOPC_SRAW 15
#define UOPC_MUL  16
#define UOPC_MULH 17
#define UOPC_MULHSU 18
#define UOPC_MULHU 19
#define UOPC_MULW 20
#define UOPC_DIV  21
#define UOPC_DIVU 22
#define UOPC_REM  23
#define UOPC_REMU 24
#define UOPC_DIVW 25
#define UOPC_DIVUW 26
#define UOPC_REMW 27
#define UOPC_REMUW 28
#define UOPC_JAL  29
#define UOPC_JALR 30
#define UOPC_BEQ  31
#define UOPC_BNE  32
#define UOPC_BLT  33
#define UOPC_BGE  34
#define UOPC_BLTU 35
#define UOPC_BGEU 36
#define UOPC_LUI  37
#define UOPC_AUIPC 38
#define UOPC_LB   39
#define UOPC_LH 40
#define UOPC_LW 41
#define UOPC_LD 42
#define UOPC_LBU  43
#define UOPC_LHU 44
#define UOPC_LWU 45
#define UOPC_SB   46
#define UOPC_SH 47
#define UOPC_SW 48
#define UOPC_SD 49
#define UOPC_ADDI 50
#define UOPC_SLLI 51
#define UOPC_SLTI 52
#define UOPC_SLTIU 53
#define UOPC_XORI 54
#define UOPC_SRLI 55
#define UOPC_SRAI 56
#define UOPC_ORI  57
#define UOPC_ANDI 58
#define UOPC_ADDIW 59
#define UOPC_SLLIW 60
#define UOPC_SRLIW 61
#define UOPC_SRAIW 62
#define UOPC_FENCE  63
#define UOPC_FENCEI 64
#define UOPC_ECALL 65
#define UOPC_EBREAK 66
#define UOPC_CSRRW  67
#define UOPC_CSRRS 68
#define UOPC_CSRRC 69
#define UOPC_CSRRWI 70
#define UOPC_CSRRSI 71
#define UOPC_CSRRCI 72
#define UOPC_MRET 118
#define UOPC_SRET 119
#define UOPC_WFI 120
#define UOPC_ILLEGAL 255

namespace boom_all {
// ==== from frontend.cpp ====



void frontend_module(FrontendState& next, const FrontendState& current,
                     const BoomCoreState& state, const BranchUpdate& brupdate,
                     MemRequest& imem_req, const MemResponse& imem_resp) {
    next = current;
    (void)state;
    (void)brupdate;
    (void)imem_req;
    (void)imem_resp;
}

// ==== from decode.cpp ====





static const char* uopc_name(UopCode u) {
    switch (u) {
        case UOPC_NOP: return "NOP";
        case UOPC_ADD: return "ADD";
        case UOPC_SUB: return "SUB";
        case UOPC_SLL: return "SLL";
        case UOPC_SLT: return "SLT";
        case UOPC_SLTU: return "SLTU";
        case UOPC_XOR: return "XOR";
        case UOPC_SRL: return "SRL";
        case UOPC_SRA: return "SRA";
        case UOPC_OR: return "OR";
        case UOPC_AND: return "AND";
        case UOPC_JAL: return "JAL";
        case UOPC_JALR: return "JALR";
        case UOPC_BEQ: return "BEQ";
        case UOPC_BNE: return "BNE";
        case UOPC_LUI: return "LUI";
        case UOPC_AUIPC: return "AUIPC";
        case UOPC_LD: return "LD";
        case UOPC_SD: return "SD";
        case UOPC_ADDI: return "ADDI";
        case UOPC_ECALL: return "ECALL";
        case UOPC_CSRRW: return "CSRRW";
        case UOPC_CSRRS: return "CSRRS";
        case UOPC_CSRRC: return "CSRRC";
        case UOPC_CSRRWI: return "CSRRWI";
        case UOPC_CSRRSI: return "CSRRSI";
        case UOPC_CSRRCI: return "CSRRCI";
        case UOPC_MUL: return "MUL";
        case UOPC_DIV: return "DIV";
        case UOPC_DIVU: return "DIVU";
        case UOPC_REM: return "REM";
        case UOPC_REMU: return "REMU";
        case UOPC_ILLEGAL: return "ILLEGAL";
        default: return "UNKNOWN";
    }
}

enum RvcOpcodes {
    RVC_C0 = 0, RVC_C1 = 1, RVC_C2 = 2, RVC_C3 = 3
};

static uint32_t decompress_rvc(uint16_t rvc) {
    uint8_t op  = rvc & 0x3;
    uint8_t f3  = (rvc >> 13) & 0x7;
    uint8_t f2  = (rvc >> 5) & 0x3;
    uint8_t rd  = (rvc >> 7) & 0x1F;
    uint8_t rs1 = (rvc >> 7) & 0x1F;
    uint8_t rs2 = (rvc >> 2) & 0x1F;
    uint8_t rd_pr = (rvc >> 2) & 0x7;
    uint8_t rs2_pr = (rvc >> 2) & 0x7;

    switch (op) {
        case RVC_C0:
            switch (f3) {
                case 0: { // C.ADDI4SPN
                    if (rvc == 0) return 0x00000013; // illegal -> nop
                    uint32_t nzuimm = ((rvc >> 5) & 0x1) | ((rvc >> 1) & 0x3C) |
                                      ((rvc >> 7) & 0x30) | ((rvc >> 2) & 0x1C0);
                    uint32_t rdp = 8 + rd_pr;
                    return (nzuimm << 20) | (2 << 15) | (0 << 12) | (rdp << 7) | 0x13;
                }
                case 2: { // C.LW (32-bit) - map to LD for RV64
                    uint32_t imm = ((rvc >> 5) & 0x1) | ((rvc >> 10) & 0x1C) |
                                  ((rvc >> 2) & 0x40) | ((rvc >> 12) & 0x20);
                    uint32_t rdp = 8 + rd_pr;
                    uint32_t rs1p = 8 + (rvc >> 7 & 0x7);
                    return (imm << 20) | (rs1p << 15) | (3 << 12) | (rdp << 7) | 0x03;
                }
                case 4: // Reserved
                case 6: { // C.SW (32-bit) - map to SD for RV64
                    uint32_t imm = ((rvc >> 5) & 0x1) | ((rvc >> 10) & 0x1C) |
                                  ((rvc >> 2) & 0x40) | ((rvc >> 12) & 0x20);
                    uint32_t rs2p = 8 + rs2_pr;
                    uint32_t rs1p = 8 + (rvc >> 7 & 0x7);
                    uint32_t imm_hi = imm >> 5;
                    uint32_t imm_lo = imm & 0x1F;
                    return (imm_hi << 25) | (rs2p << 20) | (rs1p << 15) | (3 << 12) | (imm_lo << 7) | 0x23;
                }
                default: break;
            }
            break;

        case RVC_C1:
            switch (f3) {
                case 0: { // C.ADDI / C.NOP
                    uint32_t imm = ((rvc >> 2) & 0x1F) | ((rvc >> 12) << 5);
                    if (imm & 0x20) imm |= 0xFFFFFFC0;
                    uint32_t rd_r = (rvc >> 7) & 0x1F;
                    return (imm << 20) | (0 << 15) | (0 << 12) | (rd_r << 7) | 0x13;
                }
                case 1: { // C.JAL (RV32) / C.ADDIW (RV64)
                    if (rd != 0) { // C.ADDIW
                        uint32_t imm = ((rvc >> 2) & 0x1F) | ((rvc >> 12) << 5);
                        if (imm & 0x20) imm |= 0xFFFFFFC0;
                        return (imm << 20) | (rd << 15) | (0 << 12) | (rd << 7) | 0x1B;
                    } else { // C.JAL
                        uint32_t imm = ((rvc >> 3) & 0x3) | ((rvc >> 2) & 0x1C) |
                                       ((rvc >> 7) & 0x10) | ((rvc >> 1) & 0x60) |
                                       ((rvc >> 7) & 0x80) | ((rvc >> 8) & 0x300) |
                                       ((rvc >> 12) << 10);
                        if (imm & 0x400) imm |= 0xFFFFF800;
                        return (imm << 12) | (1 << 15) | (0 << 12) | (1 << 7) | 0x6F;
                    }
                }
                case 2: { // C.LI
                    uint32_t imm = ((rvc >> 2) & 0x1F) | ((rvc >> 12) << 5);
                    if (imm & 0x20) imm |= 0xFFFFFFC0;
                    return (imm << 20) | (0 << 15) | (0 << 12) | (rd << 7) | 0x13;
                }
                case 3: { // C.LUI / C.ADDI16SP
                    if (rd == 2) { // C.ADDI16SP
                        uint32_t imm = ((rvc >> 3) & 0x1) | ((rvc >> 2) & 0xE) |
                                       ((rvc >> 5) & 0x30) | ((rvc >> 12) << 7);
                        if (imm & 0x100) imm |= 0xFFFFFE00;
                        return (imm << 20) | (2 << 15) | (0 << 12) | (2 << 7) | 0x13;
                    } else if (rd != 0) { // C.LUI
                        uint32_t imm = ((rvc >> 2) & 0x1F) | ((rvc >> 12) << 5);
                        if (imm & 0x20) imm |= 0xFFFFFFC0;
                        return ((imm & 0xFFFFF) << 12) | (rd << 7) | 0x37;
                    }
                    break;
                }
                case 4: { // C.SRLI, C.SRAI, C.ANDI, C.SUB, C.XOR, C.OR, C.AND
                    uint8_t f2b = (rvc >> 10) & 0x3;
                    uint8_t f1 = (rvc >> 12) & 0x1;
                    uint8_t rs2p = (rvc >> 2) & 0x1F;
                    uint32_t imm = ((rvc >> 2) & 0x1F) | ((rvc >> 12) << 5);
                    if (f1 == 0) { // C.SRLI
                        if (f2b == 0) {
                            return (imm << 20) | (rs1 << 15) | (5 << 12) | (rs1 << 7) | 0x13;
                        } else if (f2b == 1) { // C.SRAI
                            return (0x400 | (imm << 20)) | (rs1 << 15) | (5 << 12) | (rs1 << 7) | 0x13;
                        } else if (f2b == 2) { // C.ANDI
                            return (imm << 20) | (rs1 << 15) | (7 << 12) | (rs1 << 7) | 0x13;
                        } else { // C.SUB/C.XOR/C.OR/C.AND
                            uint8_t f2_low = (rvc >> 5) & 0x3;
                            uint8_t rs2_p = 8 + rs2p;
                            uint8_t rd_p = 8 + rd_pr;
                            if (f2_low == 0) {
                                return (rs2_p << 20) | (rd_p << 15) | (0 << 12) | (rd_p << 7) | 0x33;
                            } else if (f2_low == 1) {
                                return (rs2_p << 20) | (rd_p << 15) | (4 << 12) | (rd_p << 7) | 0x33;
                            } else if (f2_low == 2) {
                                return (rs2_p << 20) | (rd_p << 15) | (6 << 12) | (rd_p << 7) | 0x33;
                            } else {
                                return (rs2_p << 20) | (rd_p << 15) | (7 << 12) | (rd_p << 7) | 0x33;
                            }
                        }
                    } else { // f1 == 1, C.SUBW/C.ADDW
                        uint8_t rs2_p = 8 + rs2p;
                        uint8_t rd_p = 8 + rd_pr;
                        if (f2b == 0) {
                            return (rs2_p << 20) | (rd_p << 15) | (0 << 12) | (rd_p << 7) | 0x3B;
                        } else { // ADDW-like
                            return (rs2_p << 20) | (rd_p << 15) | (0 << 12) | (rd_p << 7) | 0x3B;
                        }
                    }
                    break;
                }
                case 5: { // C.J
                    uint32_t imm = ((rvc >> 3) & 0x3) | ((rvc >> 2) & 0x1C) |
                                   ((rvc >> 7) & 0x10) | ((rvc >> 1) & 0x60) |
                                   ((rvc >> 7) & 0x80) | ((rvc >> 8) & 0x300) |
                                   ((rvc >> 12) << 10);
                    if (imm & 0x400) imm |= 0xFFFFF800;
                    return (imm << 12) | (0 << 15) | (0 << 12) | (0 << 7) | 0x6F;
                }
                case 6: { // C.BEQZ
                    uint32_t imm = ((rvc >> 3) & 0x3) | ((rvc >> 2) & 0x1C) |
                                   ((rvc >> 10) & 0x60) | ((rvc >> 12) << 7);
                    if (imm & 0x100) imm |= 0xFFFFFE00;
                    uint32_t rs1p = 8 + (rvc >> 7 & 0x7);
                    imm = ((imm >> 1) & 0x3F) | ((imm & 0x1) << 6) |
                          ((imm >> 8) << 7) | ((imm >> 7 & 0x1) << 11) | ((imm >> 3 & 0x10) << 8);
                    if (imm & 0x800) imm |= 0xFFFFF000;
                    uint32_t imm_hi = (imm >> 5) & 0x7F;
                    uint32_t imm_lo = imm & 0x1F;
                    return (imm_hi << 25) | (0 << 20) | (rs1p << 15) | (0 << 12) | (imm_lo << 7) | 0x63;
                }
                case 7: { // C.BNEZ
                    uint32_t imm = ((rvc >> 3) & 0x3) | ((rvc >> 2) & 0x1C) |
                                   ((rvc >> 10) & 0x60) | ((rvc >> 12) << 7);
                    if (imm & 0x100) imm |= 0xFFFFFE00;
                    uint32_t rs1p = 8 + (rvc >> 7 & 0x7);
                    imm = ((imm >> 1) & 0x3F) | ((imm & 0x1) << 6) |
                          ((imm >> 8) << 7) | ((imm >> 7 & 0x1) << 11) | ((imm >> 3 & 0x10) << 8);
                    if (imm & 0x800) imm |= 0xFFFFF000;
                    uint32_t imm_hi = (imm >> 5) & 0x7F;
                    uint32_t imm_lo = imm & 0x1F;
                    return (imm_hi << 25) | (1 << 20) | (rs1p << 15) | (1 << 12) | (imm_lo << 7) | 0x63;
                }
                default: break;
            }
            break;

        case RVC_C2:
            switch (f3) {
                case 0: { // C.SLLI
                    uint32_t imm = ((rvc >> 2) & 0x1F) | ((rvc >> 12) << 5);
                    return (imm << 20) | (rd << 15) | (1 << 12) | (rd << 7) | 0x13;
                }
                case 2: { // C.LWSP -> LD
                    uint32_t imm = ((rvc >> 4) & 0x3) | ((rvc >> 2) & 0x1C) |
                                   ((rvc >> 12) << 5);
                    return (imm << 20) | (2 << 15) | (3 << 12) | (rd << 7) | 0x03;
                }
                case 4: { // C.JR / C.MV / C.EBREAK / C.JALR / C.ADD
                    uint8_t rs2v = (rvc >> 2) & 0x1F;
                    if (rs2v == 0) { // C.JR
                        if (rs1 == 0) return 0; // illegal
                        return (0 << 20) | (rs1 << 15) | (0 << 12) | (0 << 7) | 0x67;
                    } else { // C.MV or C.ADD
                        return (rs2v << 20) | (rs1 << 15) | (0 << 12) | (rd << 7) | 0x33;
                    }
                }
                case 6: { // C.SWSP -> SD
                    uint32_t rs2v = (rvc >> 2) & 0x1F;
                    uint32_t imm = ((rvc >> 9) & 0x3) | ((rvc >> 7) & 0x3C);
                    uint32_t imm_hi = (imm >> 5) & 0x7F;
                    uint32_t imm_lo = imm & 0x1F;
                    return (imm_hi << 25) | (rs2v << 20) | (2 << 15) | (3 << 12) | (imm_lo << 7) | 0x23;
                }
                default: break;
            }
            break;

        default: break;
    }

    return 0;
}

static uint32_t extract_imm_i(uint32_t inst) {
    int32_t v = (int32_t)(inst) >> 20;
    return (uint32_t)v;
}

static uint32_t extract_imm_s(uint32_t inst) {
    int32_t v = ((int32_t)(inst) >> 20) & 0xFFFFFE0;
    v |= (inst >> 7) & 0x1F;
    if (v & 0x800) v |= 0xFFFFF000;
    return (uint32_t)v;
}

static uint32_t extract_imm_b(uint32_t inst) {
    int32_t v = ((inst >> 8) & 0xF) << 1;
    v |= ((inst >> 25) & 0x3F) << 5;
    v |= ((inst >> 7) & 0x1) << 11;
    v |= ((inst >> 31) & 0x1) << 12;
    if (v & 0x1000) v |= 0xFFFFE000;
    return (uint32_t)v;
}

static uint32_t extract_imm_u(uint32_t inst) {
    return inst & 0xFFFFF000;
}

static uint32_t extract_imm_j(uint32_t inst) {
    int32_t v = ((inst >> 21) & 0x3FF) << 1;
    v |= ((inst >> 20) & 0x1) << 11;
    v |= ((inst >> 12) & 0xFF) << 12;
    v |= ((inst >> 31) & 0x1) << 20;
    if (v & 0x100000) v |= 0xFFE00000;
    return (uint32_t)v;
}

static uint8_t op_fcn_from_funct3_funct7(uint8_t f3, uint8_t f7) {
    switch (f3) {
        case 0: return (f7 == 0) ? 0 : (f7 == 0x20 ? 8 : 0);  // ADD/SUB, MUL
        case 1: return (f7 == 0) ? 1 : (f7 == 0x20 ? 9 : 1);  // SLL, MULH
        case 2: return (f7 == 0) ? 2 : (f7 == 0x20 ? 10 : 2); // SLT, MULHSU
        case 3: return (f7 == 0) ? 3 : (f7 == 0x20 ? 11 : 3); // SLTU, MULHU
        case 4: return (f7 == 0) ? 4 : (f7 == 0x20 ? 5 : 4);  // XOR, DIV
        case 5: return (f7 == 0) ? 6 : (f7 == 0x20 ? 7 : 6);  // SRL/SRA, DIVU
        case 6: return (f7 == 0) ? 12 : (f7 == 0x20 ? 13 : 12); // OR, REM
        case 7: return (f7 == 0) ? 14 : (f7 == 0x20 ? 15 : 14); // AND, REMU
        default: return 0;
    }
}

void decode_one(uint32_t inst, uint64_t pc, MicroOp& uop, bool& is_rvc_in) {
    uop = MicroOp();
    uop.inst = inst;
    uop.debug_inst = inst;
    uop.debug_pc = pc;

    if (inst == 0x00000013) { // NOP
        uop.uopc = UOPC_NOP;
        uop.iq_type = IQ_ALU;
        uop.fu_code = FU_ALU;
        uop.ctrl.op1_sel = OP1_X0;
        uop.ctrl.op2_sel = OP2_X0;
        uop.ctrl.op_fcn = 0;
        uop.rename.dst_rtype = DST_N;
        uop.is_unique = true;
        return;
    }

    if (inst == 0x00000000) { // Illegal all-zero
        uop.uopc = UOPC_ILLEGAL;
        uop.exception = true;
        uop.exc_cause = 2; // illegal instruction
        return;
    }

    uint8_t opcode = inst & 0x7F;
    uint8_t rd      = (inst >> 7) & 0x1F;
    uint8_t f3      = (inst >> 12) & 0x7;
    uint8_t rs1     = (inst >> 15) & 0x1F;
    uint8_t rs2     = (inst >> 20) & 0x1F;
    uint8_t f7      = (inst >> 25) & 0x7F;

    uop.rename.lrs1 = rs1;
    uop.rename.lrs2 = rs2;
    uop.rename.lrs3 = 0;
    uop.rename.ldst = rd;
    uop.branch.pc_lob = pc & 0x3F;

    switch (opcode) {
        case 0x37: // LUI
            uop.uopc = UOPC_LUI;
            uop.iq_type = IQ_ALU;
            uop.fu_code = FU_ALU;
            uop.ctrl.op1_sel = OP1_X0;
            uop.ctrl.op2_sel = OP2_IMU;
            uop.ctrl.imm_sel = IMM_U;
            uop.imm_packed = extract_imm_u(inst);
            uop.rename.dst_rtype = DST_INT;
            break;

        case 0x17: // AUIPC
            uop.uopc = UOPC_AUIPC;
            uop.iq_type = IQ_ALU;
            uop.fu_code = FU_ALU;
            uop.ctrl.op1_sel = OP1_PC;
            uop.ctrl.op2_sel = OP2_IMU;
            uop.ctrl.imm_sel = IMM_U;
            uop.imm_packed = extract_imm_u(inst);
            uop.rename.dst_rtype = DST_INT;
            break;

        case 0x6F: // JAL
            uop.uopc = UOPC_JAL;
            uop.iq_type = IQ_ALU;
            uop.fu_code = FU_ALU;
            uop.ctrl.op1_sel = OP1_PC;
            uop.ctrl.op2_sel = OP2_IMM;
            uop.ctrl.imm_sel = IMM_J;
            uop.ctrl.br_type = BR_J;
            uop.imm_packed = extract_imm_j(inst);
            uop.rename.dst_rtype = (rd != 0) ? DST_INT : DST_X0;
            uop.branch.is_jal = true;
            break;

        case 0x67: // JALR
            uop.uopc = UOPC_JALR;
            uop.iq_type = IQ_ALU;
            uop.fu_code = FU_ALU;
            uop.ctrl.op1_sel = OP1_RS1;
            uop.ctrl.op2_sel = OP2_IMM;
            uop.ctrl.imm_sel = IMM_I;
            uop.ctrl.br_type = BR_JR;
            uop.imm_packed = extract_imm_i(inst);
            uop.rename.dst_rtype = (rd != 0) ? DST_INT : DST_X0;
            uop.branch.is_jalr = true;
            break;

        case 0x63: { // Branches
            uop.iq_type = IQ_ALU;
            uop.fu_code = FU_ALU;
            uop.ctrl.op1_sel = OP1_RS1;
            uop.ctrl.op2_sel = OP2_RS2;
            uop.ctrl.imm_sel = IMM_B;
            uop.imm_packed = extract_imm_b(inst);
            uop.branch.is_br = true;
            uop.rename.dst_rtype = DST_N;

            switch (f3) {
                case 0: uop.uopc = UOPC_BEQ;  uop.ctrl.br_type = BR_EQ;  break;
                case 1: uop.uopc = UOPC_BNE;  uop.ctrl.br_type = BR_NE;  break;
                case 4: uop.uopc = UOPC_BLT;  uop.ctrl.br_type = BR_LT;  break;
                case 5: uop.uopc = UOPC_BGE;  uop.ctrl.br_type = BR_GE;  break;
                case 6: uop.uopc = UOPC_BLTU; uop.ctrl.br_type = BR_LTU; break;
                case 7: uop.uopc = UOPC_BGEU; uop.ctrl.br_type = BR_GEU; break;
                default: uop.uopc = UOPC_ILLEGAL; uop.exception = true; uop.exc_cause = 2; break;
            }
            break;
        }

        case 0x03: { // LOAD
            uop.iq_type = IQ_MEM;
            uop.fu_code = FU_MEM;
            uop.ctrl.op1_sel = OP1_RS1;
            uop.ctrl.op2_sel = OP2_IMM;
            uop.ctrl.imm_sel = IMM_I;
            uop.ctrl.is_load = true;
            uop.imm_packed = extract_imm_i(inst);
            uop.mem.uses_ldq = true;
            uop.rename.dst_rtype = DST_INT;

            switch (f3) {
                case 0: uop.uopc = UOPC_LB;  uop.mem.mem_size = 0; uop.mem.mem_signed = true;  break;
                case 1: uop.uopc = UOPC_LH;  uop.mem.mem_size = 1; uop.mem.mem_signed = true;  break;
                case 2: uop.uopc = UOPC_LW;  uop.mem.mem_size = 2; uop.mem.mem_signed = true;  break;
                case 3: uop.uopc = UOPC_LD;  uop.mem.mem_size = 3; uop.mem.mem_signed = true;  break;
                case 4: uop.uopc = UOPC_LBU; uop.mem.mem_size = 0; uop.mem.mem_signed = false; break;
                case 5: uop.uopc = UOPC_LHU; uop.mem.mem_size = 1; uop.mem.mem_signed = false; break;
                case 6: uop.uopc = UOPC_LWU; uop.mem.mem_size = 2; uop.mem.mem_signed = false; break;
                default: uop.uopc = UOPC_ILLEGAL; uop.exception = true; uop.exc_cause = 2; break;
            }
            break;
        }

        case 0x23: { // STORE
            uop.iq_type = IQ_MEM;
            uop.fu_code = FU_MEM;
            uop.ctrl.op1_sel = OP1_RS1;
            uop.ctrl.op2_sel = OP2_RS2;
            uop.ctrl.imm_sel = IMM_S;
            uop.ctrl.is_sta = true;
            uop.imm_packed = extract_imm_s(inst);
            uop.mem.uses_stq = true;
            uop.mem.ldst_is_rs1 = false;
            uop.rename.dst_rtype = DST_N;
            uop.rename.lrs2 = rs2;

            switch (f3) {
                case 0: uop.uopc = UOPC_SB; uop.mem.mem_size = 0; break;
                case 1: uop.uopc = UOPC_SH; uop.mem.mem_size = 1; break;
                case 2: uop.uopc = UOPC_SW; uop.mem.mem_size = 2; break;
                case 3: uop.uopc = UOPC_SD; uop.mem.mem_size = 3; break;
                default: uop.uopc = UOPC_ILLEGAL; uop.exception = true; uop.exc_cause = 2; break;
            }
            break;
        }

        case 0x13: { // OP-IMM
            uop.iq_type = IQ_ALU;
            uop.fu_code = FU_ALU;
            uop.ctrl.op1_sel = OP1_RS1;
            uop.ctrl.op2_sel = OP2_IMM;
            uop.ctrl.imm_sel = IMM_I;
            uop.imm_packed = extract_imm_i(inst);
            uop.rename.dst_rtype = DST_INT;

            switch (f3) {
                case 0: uop.uopc = UOPC_ADDI;  uop.ctrl.op_fcn = 0; break;
                case 1: uop.uopc = UOPC_SLLI;  uop.ctrl.op_fcn = 1; break;
                case 2: uop.uopc = UOPC_SLTI;  uop.ctrl.op_fcn = 2; break;
                case 3: uop.uopc = UOPC_SLTIU; uop.ctrl.op_fcn = 3; break;
                case 4: uop.uopc = UOPC_XORI;  uop.ctrl.op_fcn = 4; break;
                case 5:
                    if (f7 == 0)      { uop.uopc = UOPC_SRLI; uop.ctrl.op_fcn = 6; }
                    else if (f7 == 0x20) { uop.uopc = UOPC_SRAI; uop.ctrl.op_fcn = 7; }
                    else { uop.uopc = UOPC_ILLEGAL; uop.exception = true; }
                    break;
                case 6: uop.uopc = UOPC_ORI;   uop.ctrl.op_fcn = 8; break;
                case 7: uop.uopc = UOPC_ANDI;  uop.ctrl.op_fcn = 9; break;
                default: uop.uopc = UOPC_ILLEGAL; uop.exception = true; break;
            }
            break;
        }

        case 0x1B: { // OP-IMM-32 (RV64)
            uop.iq_type = IQ_ALU;
            uop.fu_code = FU_ALU;
            uop.ctrl.op1_sel = OP1_RS1;
            uop.ctrl.op2_sel = OP2_IMM;
            uop.ctrl.imm_sel = IMM_I;
            uop.ctrl.fcn_dw = 1;
            uop.imm_packed = extract_imm_i(inst);
            uop.rename.dst_rtype = DST_INT;

            switch (f3) {
                case 0: uop.uopc = UOPC_ADDIW; uop.ctrl.op_fcn = 0; break;
                case 1: uop.uopc = UOPC_SLLIW; uop.ctrl.op_fcn = 1; break;
                case 5:
                    if (f7 == 0)      { uop.uopc = UOPC_SRLIW; uop.ctrl.op_fcn = 6; }
                    else if (f7 == 0x20) { uop.uopc = UOPC_SRAIW; uop.ctrl.op_fcn = 7; }
                    else { uop.uopc = UOPC_ILLEGAL; uop.exception = true; }
                    break;
                default: uop.uopc = UOPC_ILLEGAL; uop.exception = true; break;
            }
            break;
        }

        case 0x33: { // OP
            uop.iq_type = IQ_ALU;
            uop.ctrl.op1_sel = OP1_RS1;
            uop.ctrl.op2_sel = OP2_RS2;

            if (f7 == 1) { // M-extension
                uop.fu_code = FU_MUL;
                uop.rename.dst_rtype = DST_INT;
                switch (f3) {
                    case 0: uop.uopc = UOPC_MUL;  uop.ctrl.op_fcn = 0; break;
                    case 1: uop.uopc = UOPC_MULH;  uop.ctrl.op_fcn = 1; break;
                    case 2: uop.uopc = UOPC_MULHSU; uop.ctrl.op_fcn = 2; break;
                    case 3: uop.uopc = UOPC_MULHU; uop.ctrl.op_fcn = 3; break;
                    default: uop.uopc = UOPC_ILLEGAL; uop.exception = true; break;
                }
            } else {
                uop.fu_code = FU_ALU;
                uop.rename.dst_rtype = DST_INT;
                switch (f3) {
                    case 0: uop.uopc = (f7 == 0x20) ? UOPC_SUB  : UOPC_ADD;  uop.ctrl.op_fcn = 0; break;
                    case 1: uop.uopc = (f7 == 0x20) ? UOPC_SLL  : UOPC_SLL;  uop.ctrl.op_fcn = 1; break; // SLL f7=0 only
                    case 2: uop.uopc = UOPC_SLT;  uop.ctrl.op_fcn = 2; break;
                    case 3: uop.uopc = UOPC_SLTU; uop.ctrl.op_fcn = 3; break;
                    case 4: uop.uopc = UOPC_XOR;  uop.ctrl.op_fcn = 4; break;
                    case 5: uop.uopc = (f7 == 0x20) ? UOPC_SRA  : UOPC_SRL;  uop.ctrl.op_fcn = (f7 == 0x20) ? 7 : 6; break;
                    case 6: uop.uopc = UOPC_OR;   uop.ctrl.op_fcn = 8; break;
                    case 7: uop.uopc = UOPC_AND;  uop.ctrl.op_fcn = 9; break;
                    default: uop.uopc = UOPC_ILLEGAL; uop.exception = true; break;
                }
            }
            break;
        }

        case 0x3B: { // OP-32 (RV64)
            uop.iq_type = IQ_ALU;
            uop.ctrl.op1_sel = OP1_RS1;
            uop.ctrl.op2_sel = OP2_RS2;
            uop.ctrl.fcn_dw = 1;

            if (f7 == 1) { // M-extension 32-bit
                uop.fu_code = FU_MUL;
                uop.rename.dst_rtype = DST_INT;
                switch (f3) {
                    case 0: uop.uopc = UOPC_MULW; uop.ctrl.op_fcn = 0; break;
                    default: uop.uopc = UOPC_ILLEGAL; uop.exception = true; break;
                }
            } else {
                uop.fu_code = FU_ALU;
                uop.rename.dst_rtype = DST_INT;
                switch (f3) {
                    case 0: uop.uopc = (f7 == 0x20) ? UOPC_SUBW  : UOPC_ADDW; uop.ctrl.op_fcn = 0; break;
                    case 1: uop.uopc = UOPC_SLLW; uop.ctrl.op_fcn = 1; break;
                    case 5: uop.uopc = (f7 == 0x20) ? UOPC_SRAW  : UOPC_SRLW; uop.ctrl.op_fcn = (f7 == 0x20) ? 7 : 6; break;
                    default: uop.uopc = UOPC_ILLEGAL; uop.exception = true; break;
                }
            }
            break;
        }

        case 0x0F: // FENCE
            uop.uopc = UOPC_FENCE;
            uop.iq_type = IQ_MEM;
            uop.fu_code = FU_MEM;
            uop.rename.dst_rtype = DST_N;
            uop.is_unique = true;
            break;

        case 0x73: { // SYSTEM
            if (f3 == 0) {
                if (rs2 == 0 && rd == 0 && f7 == 0) {
                    uop.uopc = UOPC_ECALL;
                    uop.iq_type = IQ_ALU;
                    uop.fu_code = FU_CSR;
                    uop.exception = true;
                    uop.exc_cause = 8 + 0; // Environment call from U-mode (will adjust based on priv)
                    uop.is_sys_pc2epc = true;
                } else if (rs2 == 1 && rd == 0 && f7 == 0) {
                    uop.uopc = UOPC_EBREAK;
                    uop.iq_type = IQ_ALU;
                    uop.fu_code = FU_CSR;
                    uop.exception = true;
                    uop.exc_cause = 3; // Breakpoint
                    uop.is_sys_pc2epc = true;
                } else if (f7 == 0x18 && rs2 == 2) {
                    uop.uopc = UOPC_MRET;
                    uop.iq_type = IQ_ALU;
                    uop.fu_code = FU_CSR;
                    uop.is_unique = true;
                    uop.rename.dst_rtype = DST_N;
                } else if (f7 == 0x08 && rs2 == 5) {
                    uop.uopc = UOPC_WFI;
                    uop.iq_type = IQ_ALU;
                    uop.fu_code = FU_CSR;
                    uop.is_unique = true;
                    uop.rename.dst_rtype = DST_N;
                } else {
                    uop.uopc = UOPC_ILLEGAL;
                    uop.exception = true;
                    uop.exc_cause = 2;
                }
            } else {
                uop.iq_type = IQ_ALU;
                uop.fu_code = FU_CSR;
                uop.ctrl.op1_sel = OP1_RS1;
                uop.ctrl.op2_sel = OP2_IMZ;
                uop.imm_packed = rs2; // uimm for CSR
                uop.csr_addr = inst >> 20;
                uop.rename.dst_rtype = DST_INT;

                switch (f3) {
                    case 1: uop.uopc = UOPC_CSRRW;  uop.ctrl.csr_cmd = CSR_W; break;
                    case 2: uop.uopc = UOPC_CSRRS;  uop.ctrl.csr_cmd = CSR_S; break;
                    case 3: uop.uopc = UOPC_CSRRC;  uop.ctrl.csr_cmd = CSR_C; break;
                    case 5: uop.uopc = UOPC_CSRRWI; uop.ctrl.csr_cmd = CSR_W; break;
                    case 6: uop.uopc = UOPC_CSRRSI; uop.ctrl.csr_cmd = CSR_S; break;
                    case 7: uop.uopc = UOPC_CSRRCI; uop.ctrl.csr_cmd = CSR_C; break;
                    default: uop.uopc = UOPC_ILLEGAL; uop.exception = true; uop.exc_cause = 2; break;
                }
            }
            break;
        }

        case 0x07: // FLW (F extension) -- stub: unsupported
        case 0x27: // FSW (F extension) -- stub
        case 0x43: // FMADD
        case 0x47: // FMSUB
        case 0x4B: // FNMSUB
        case 0x4F: // FNMADD
        case 0x53: // FP OP
        {
            uop.uopc = UOPC_ILLEGAL;
            uop.exception = true;
            uop.exc_cause = 2; // FP extension not implemented
            // Note: misa still reports F/D, these will raise illegal inst if FP off
            // For now, treat as unsupported stub
            break;
        }

        case 0x2F: { // AMO
            uop.uopc = UOPC_ILLEGAL;
            uop.exception = true;
            uop.exc_cause = 2; // AMO extension stub
            break;
        }

        default: {
            uop.uopc = UOPC_ILLEGAL;
            uop.exception = true;
            uop.exc_cause = 2; // illegal instruction
            break;
        }
    }

    if (uop.uopc == UOPC_ILLEGAL && !uop.exception) {
        uop.exception = true;
        uop.exc_cause = 2;
    }
}

void decode_module(DecodeState& next, const DecodeState& current,
                   const FrontendState& frontend, const BoomCoreState& state) {
    next = current;

    next.dec_valids[0] = false;
    next.dec_uops[0] = MicroOp();

    if (state.global_flush) return;
    if (frontend.stalled) return;

    uint64_t pc = frontend.pc;
    uint32_t raw_inst = (frontend.icache_resp_ready)
        ? (uint32_t)(frontend.icache_resp_data & 0xFFFFFFFF) : 0;

    uint32_t inst = raw_inst;
    bool is_rvc = (frontend.icache_resp_data >> 32) & 1;

    if (is_rvc) {
        uint16_t rvc = raw_inst & 0xFFFF;
        inst = decompress_rvc(rvc);
        if (inst == 0x00000013) {
            is_rvc = false;
        }
    }

    MicroOp uop;
    decode_one(inst, pc, uop, is_rvc);
    uop.is_rvc = is_rvc;

    next.dec_valids[0] = true;
    next.dec_uops[0] = uop;

    (void)state;
}

// ==== from rename.cpp ====



static void int_map_table_snapshot(RenameMapTableState& mt, uint8_t br_tag) {
    if (br_tag < MAX_BRANCH_COUNT) {
        for (int i = 0; i < LOGICAL_REG_COUNT; i++) {
            mt.br_snapshots[i][br_tag] = mt.map_table[i];
        }
    }
}

static void int_map_table_restore(RenameMapTableState& mt, uint8_t br_mask) {
    for (int i = 0; i < LOGICAL_REG_COUNT; i++) {
        mt.map_table[i] = mt.committed_map_table[i];
    }
    for (int b = 0; b < MAX_BRANCH_COUNT; b++) {
        if (br_mask & (1 << b)) {
            for (int i = 0; i < LOGICAL_REG_COUNT; i++) {
                mt.map_table[i] = mt.br_snapshots[i][b];
            }
            break;
        }
    }
}

static uint8_t int_allocate_pdst(RenameFreeListState& fl) {
    if (fl.count == 0) return 0;
    uint8_t pdst = fl.free_list[fl.head];
    fl.head = (fl.head + 1) % INT_PHYS_REGS;
    fl.count--;
    fl.busy_table[pdst] = true;
    return pdst;
}

static void int_free_pdst(RenameFreeListState& fl, uint8_t pdst) {
    if (pdst == 0) return;
    if (!fl.busy_table[pdst]) return;
    fl.busy_table[pdst] = false;
    fl.free_list[fl.tail] = pdst;
    fl.tail = (fl.tail + 1) % INT_PHYS_REGS;
    fl.count++;
}

static bool int_is_busy(RenameFreeListState& fl, uint8_t pdst) {
    return fl.busy_table[pdst];
}

void rename_module(RenameState& next, const RenameState& current,
                   const DecodeState& decode, const BoomCoreState& state) {
    next = current;

    for (int i = 0; i < DISPATCH_WIDTH; i++) {
        next.renamed_valids[i] = false;
        next.renamed_uops[i] = MicroOp();
    }

    if (state.global_flush) return;

    // Handle branch updates
    if (state.brupdate.valid && state.brupdate.mispredict) {
        int_map_table_restore(next.int_map_table, state.brupdate.mispredict_mask);
    }

    RenameMapTableState& mt = next.int_map_table;
    RenameFreeListState& fl = next.int_free_list;

    for (int i = 0; i < DISPATCH_WIDTH; i++) {
        DECODE_LANE:
        if (!decode.dec_valids[i]) continue;

        MicroOp uop = decode.dec_uops[i];

        // Register renaming: lookup map table for source registers
        RenameMapRequest map_req;
        map_req.lrs1 = uop.rename.lrs1;
        map_req.lrs2 = uop.rename.lrs2;
        map_req.lrs3 = uop.rename.lrs3;
        map_req.ldst = uop.rename.ldst;

        // Map table lookup
        uop.rename.prs1 = mt.map_table[map_req.lrs1];
        uop.rename.prs2 = mt.map_table[map_req.lrs2];
        uop.rename.prs3 = mt.map_table[map_req.lrs3];
        uop.rename.stale_pdst = mt.map_table[map_req.ldst];

        // Busy bit lookup
        uop.rename.prs1_busy = int_is_busy(fl, uop.rename.prs1);
        uop.rename.prs2_busy = int_is_busy(fl, uop.rename.prs2);
        uop.rename.prs3_busy = int_is_busy(fl, uop.rename.prs3);

        // Allocate new physical destination register
        if (uop.rename.dst_rtype == DST_INT) {
            if (map_req.ldst != 0) {
                uint8_t new_pdst = int_allocate_pdst(fl);
                uop.rename.pdst = new_pdst;
                mt.map_table[map_req.ldst] = new_pdst;
            } else {
                uop.rename.pdst = 0;
            }
        } else if (uop.rename.dst_rtype == DST_X0) {
            uop.rename.pdst = 0;
            uop.rename.ldst = 0;
        } else {
            uop.rename.pdst = 0; // FP/N destination - will be remapped by FP rename
        }

        // Branch mask assignment
        if (uop.branch.is_br || uop.branch.is_jal || uop.branch.is_jalr) {
            // Simple: use next available br_tag
            static uint8_t next_br_tag = 0;
            uop.branch.br_tag = next_br_tag;
            uop.branch.br_mask = 1 << next_br_tag;
            next_br_tag = (next_br_tag + 1) % MAX_BRANCH_COUNT;

            // Take snapshot of map table
            int_map_table_snapshot(mt, uop.branch.br_tag);
        }

        next.renamed_uops[i] = uop;
        next.renamed_valids[i] = true;
    }
}

// ==== from rob.cpp ====



bool rob_full(const RobInternalState& rob) {
    return (rob.head == rob.tail) && rob.maybe_full;
}

bool rob_empty(const RobInternalState& rob) {
    return (rob.head == rob.tail) && !rob.maybe_full;
}

static void int_free_pdst_inline(uint8_t pdst) {
    (void)pdst;
}

void rob_module(RobInternalState& next, const RobInternalState& current,
                const ExecuteState& execute, BoomCoreState& state) {
    next = current;

    next.flush_frontend = false;

    switch (next.state) {
        case ROB_INIT:
            next.state = ROB_NORMAL;
            break;

        case ROB_NORMAL: {
            // Allocate
            for (int i = 0; i < DISPATCH_WIDTH; i++) {
                if (state.rename.renamed_valids[i] && !rob_full(next)) {
                    RobEntry& entry = next.entries[next.tail];
                    entry.valid = true;
                    entry.busy = true;
                    entry.unsafe = false;
                    entry.uop = state.rename.renamed_uops[i];
                    entry.uop.queue.rob_idx = next.tail;
                    next.tail = (next.tail + 1) % ROB_DEPTH;
                    if (next.tail == next.head) next.maybe_full = true;
                }
            }

            // Writeback / complete
            for (int i = 0; i < DISPATCH_WIDTH; i++) {
                if (execute.alu_results[i].valid) {
                    const ExecuteState::AluResult& res = execute.alu_results[i];
                    uint8_t rob_idx = res.uop.queue.rob_idx;
                    if (rob_idx < ROB_DEPTH && next.entries[rob_idx].valid) {
                        next.entries[rob_idx].busy = false;
                        if (res.exception) {
                            next.entries[rob_idx].exception = true;
                            next.entries[rob_idx].uop.exception = true;
                            next.entries[rob_idx].uop.exc_cause = res.exc_cause;
                        }
                    }
                }
            }

            // Commit
            bool can_commit = !rob_empty(next) && next.entries[next.head].valid;

            // PNR (pending no-rename): track committed pdst for reclaim
            if (can_commit) {
                RobEntry& head_entry = next.entries[next.head];

                if (!head_entry.busy) {
                    if (head_entry.exception) {
                        next.state = ROB_EXCEPTION;
                        next.xcpt_uop = head_entry.uop;
                        next.xcpt_badvaddr = 0;
                        next.flush_frontend = true;
                    } else {
                        // Writeback to register file
                        MicroOp& uop = head_entry.uop;
                        if (uop.rename.dst_rtype == DST_INT && uop.rename.pdst != 0) {
                            // Result already in RF from execute writeback
                            // Free stale physical register
                            if (uop.rename.stale_pdst != 0 && uop.rename.stale_pdst != uop.rename.pdst) {
                                // Update committed map table
                                state.rename.int_map_table.committed_map_table[uop.rename.ldst] = uop.rename.pdst;
                                // Free old mapping
                                int_free_pdst_inline(uop.rename.stale_pdst);
                            }
                        }

                        head_entry.valid = false;
                        next.head = (next.head + 1) % ROB_DEPTH;
                        next.maybe_full = false;
                    }
                }
            }

            // Branch mispredict flush
            if (state.brupdate.valid && state.brupdate.mispredict) {
                uint8_t kill_mask = state.brupdate.mispredict_mask;

                for (int i = 0; i < ROB_DEPTH; i++) {
                    if (next.entries[i].valid) {
                        if (next.entries[i].uop.branch.br_mask & kill_mask) {
                            // Kill this entry
                            uint8_t pdst = next.entries[i].uop.rename.pdst;
                            if (pdst != 0 && next.entries[i].uop.rename.dst_rtype == DST_INT) {
                                // Free pdst
                                if (next.entries[i].uop.rename.stale_pdst != 0) {
                                    // Don't free stale_pdst on flush - the committed map table still points to it
                                }
                            }
                            next.entries[i].valid = false;
                            next.entries[i].busy = false;
                        }
                    }
                }

                next.tail = (next.head + 1) % ROB_DEPTH; // Simplified: flush all after head
                next.maybe_full = false;
                next.state = ROB_NORMAL;
            }

            break;
        }

        case ROB_FLUSH:
            // Clean up all entries
            for (int i = 0; i < ROB_DEPTH; i++) {
                next.entries[i].valid = false;
                next.entries[i].busy = false;
                next.entries[i].exception = false;
            }
            next.head = 0;
            next.tail = 0;
            next.maybe_full = false;
            next.pnr = 0;
            next.state = ROB_NORMAL;
            break;

        case ROB_EXCEPTION:
            // Exception handling: record exception, stay in exception state until handled
            next.flush_frontend = true;
            // In full implementation, would jump to exception handler
            // For M2, stay in exception state
            break;
    }

    (void)execute;
}

// ==== from issue.cpp ====



static void iq_enqueue(IssueQueueState& iq, const MicroOp& uop) {
    if (iq.count >= ISSUE_QUEUE_MEM_DEPTH) return;
    IssueSlotEntry& slot = iq.entries[iq.tail];
    slot.valid = true;
    slot.request = true;
    slot.granted = false;
    slot.killed = false;
    slot.uop = uop;
    slot.pdst_busy = uop.rename.ppred_busy;
    slot.prs1_busy = uop.rename.prs1_busy;
    slot.prs2_busy = uop.rename.prs2_busy;
    slot.prs3_busy = uop.rename.prs3_busy;
    slot.prs1_data = 0;
    slot.prs2_data = 0;
    slot.prs3_data = 0;

    iq.tail = (iq.tail + 1) % ISSUE_QUEUE_MEM_DEPTH;
    iq.count++;
}

static void iq_wakeup(IssueQueueState& iq, uint8_t pdst) {
    for (int i = 0; i < ISSUE_QUEUE_MEM_DEPTH; i++) {
        if (!iq.entries[i].valid) continue;
        if (iq.entries[i].uop.rename.prs1 == pdst) iq.entries[i].prs1_busy = false;
        if (iq.entries[i].uop.rename.prs2 == pdst) iq.entries[i].prs2_busy = false;
        if (iq.entries[i].uop.rename.prs3 == pdst) iq.entries[i].prs3_busy = false;
    }
}

static bool iq_is_ready(const IssueSlotEntry& slot) {
    if (!slot.valid || !slot.request || slot.killed) return false;
    return !slot.prs1_busy && !slot.prs2_busy && !slot.prs3_busy && !slot.pdst_busy;
}

static int iq_select(IssueQueueState& iq) {
    for (int i = 0; i < ISSUE_QUEUE_MEM_DEPTH; i++) {
        if (iq_is_ready(iq.entries[i])) {
            return i;
        }
    }
    return -1;
}

static void iq_grant(IssueQueueState& iq, int idx) {
    if (idx < 0 || idx >= ISSUE_QUEUE_MEM_DEPTH) return;
    iq.entries[idx].granted = true;
    iq.entries[idx].request = false;
}

static void iq_compress(IssueQueueState& iq) {
    IssueSlotEntry temp[ISSUE_QUEUE_MEM_DEPTH];
    int write_idx = 0;
    for (int i = 0; i < ISSUE_QUEUE_MEM_DEPTH; i++) {
        if (iq.entries[i].valid && !iq.entries[i].granted) {
            temp[write_idx++] = iq.entries[i];
        }
    }
    for (int i = write_idx; i < ISSUE_QUEUE_MEM_DEPTH; i++) {
        temp[i].valid = false;
    }
    for (int i = 0; i < ISSUE_QUEUE_MEM_DEPTH; i++) {
        iq.entries[i] = temp[i];
    }
    iq.head = 0;
    iq.tail = write_idx % ISSUE_QUEUE_MEM_DEPTH;
    iq.count = write_idx;
}

static void iq_kill_by_mask(IssueQueueState& iq, uint8_t br_mask) {
    for (int i = 0; i < ISSUE_QUEUE_MEM_DEPTH; i++) {
        if (iq.entries[i].valid && (iq.entries[i].uop.branch.br_mask & br_mask)) {
            iq.entries[i].killed = true;
        }
    }
}

static void iq_flush(IssueQueueState& iq) {
    for (int i = 0; i < ISSUE_QUEUE_MEM_DEPTH; i++) {
        iq.entries[i].valid = false;
        iq.entries[i].killed = false;
        iq.entries[i].granted = false;
        iq.entries[i].request = false;
    }
    iq.head = 0;
    iq.tail = 0;
    iq.count = 0;
}

static bool iq_dispatch(IssueQueueState& iq, IqType type, const MicroOp& uop) {
    if (uop.iq_type != type) return false;
    if (iq.count >= ISSUE_QUEUE_MEM_DEPTH) return false;
    iq_enqueue(iq, uop);
    return true;
}

void issue_module(IssueState& next, const IssueState& current,
                  const RenameState& rename, const ExecuteState& execute,
                  const BoomCoreState& state) {
    next = current;

    for (int i = 0; i < ISSUE_WIDTH; i++) {
        next.issued_valids[i] = false;
        next.issued_uops[i] = MicroOp();
    }

    if (state.global_flush) {
        iq_flush(next.mem_iq);
        iq_flush(next.alu_iq);
        iq_flush(next.fpu_iq);
        return;
    }

    // Branch kill
    if (state.brupdate.valid && state.brupdate.mispredict) {
        iq_kill_by_mask(next.mem_iq, state.brupdate.mispredict_mask);
        iq_kill_by_mask(next.alu_iq, state.brupdate.mispredict_mask);
        iq_kill_by_mask(next.fpu_iq, state.brupdate.mispredict_mask);
    }

    // Wakeup from execute results
    for (int i = 0; i < DISPATCH_WIDTH; i++) {
        if (execute.alu_results[i].valid) {
            uint8_t pdst = execute.alu_results[i].uop.rename.pdst;
            iq_wakeup(next.mem_iq, pdst);
            iq_wakeup(next.alu_iq, pdst);
            iq_wakeup(next.fpu_iq, pdst);
        }
    }

    // Dispatch renamed uops to queues
    for (int i = 0; i < DISPATCH_WIDTH; i++) {
        if (rename.renamed_valids[i]) {
            const MicroOp& uop = rename.renamed_uops[i];
            switch (uop.iq_type) {
                case IQ_MEM: iq_dispatch(next.mem_iq, IQ_MEM, uop); break;
                case IQ_ALU: iq_dispatch(next.alu_iq, IQ_ALU, uop); break;
                case IQ_FPU: iq_dispatch(next.fpu_iq, IQ_FPU, uop); break;
                default: break;
            }
        }
    }

    // Select and issue ready instructions (up to ISSUE_WIDTH)
    int issued = 0;
    for (int i = 0; i < ISSUE_QUEUE_MEM_DEPTH && issued < ISSUE_WIDTH; i++) {
        if (iq_is_ready(next.mem_iq.entries[i])) {
            iq_grant(next.mem_iq, i);
            next.issued_uops[issued] = next.mem_iq.entries[i].uop;
            next.issued_valids[issued] = true;
            issued++;
            continue;
        }
    }
    for (int i = 0; i < ISSUE_QUEUE_MEM_DEPTH && issued < ISSUE_WIDTH; i++) {
        if (iq_is_ready(next.alu_iq.entries[i])) {
            iq_grant(next.alu_iq, i);
            next.issued_uops[issued] = next.alu_iq.entries[i].uop;
            next.issued_valids[issued] = true;
            issued++;
            continue;
        }
    }
    for (int i = 0; i < ISSUE_QUEUE_MEM_DEPTH && issued < ISSUE_WIDTH; i++) {
        if (iq_is_ready(next.fpu_iq.entries[i])) {
            iq_grant(next.fpu_iq, i);
            next.issued_uops[issued] = next.fpu_iq.entries[i].uop;
            next.issued_valids[issued] = true;
            issued++;
            continue;
        }
    }

    // Compress queues (remove granted entries)
    iq_compress(next.mem_iq);
    iq_compress(next.alu_iq);
    iq_compress(next.fpu_iq);

    // Remove killed entries
    for (int i = 0; i < ISSUE_QUEUE_MEM_DEPTH; i++) {
        if (next.mem_iq.entries[i].killed) next.mem_iq.entries[i].valid = false;
        if (next.alu_iq.entries[i].killed) next.alu_iq.entries[i].valid = false;
        if (next.fpu_iq.entries[i].killed) next.fpu_iq.entries[i].valid = false;
    }

    (void)state;
}

// ==== from execute.cpp ====



static uint64_t sext(uint64_t val, int bits) {
    if (bits >= 64) return val;
    uint64_t mask = (1ULL << bits) - 1;
    if (val & (1ULL << (bits - 1)))
        return val | (~mask);
    return val & mask;
}

static int64_t compute_alu(uint8_t op_fcn, bool fcn_dw, int64_t op1, int64_t op2) {
    switch (op_fcn) {
        case 0:  return op1 + op2;                              // ADD/ADDI
        case 1:  return op1 << (op2 & 0x3F);                     // SLL/SLLI
        case 2:  return (op1 < op2) ? 1 : 0;                     // SLT/SLTI
        case 3:  return ((uint64_t)op1 < (uint64_t)op2) ? 1 : 0; // SLTU/SLTIU
        case 4:  return op1 ^ op2;                                // XOR/XORI
        case 5:  return op2 - op1;                                // SUB trap (SUB=0x20/f7, this is f7-based)
        case 6:  return (uint64_t)op1 >> (op2 & 0x3F);            // SRL/SRLI
        case 7:  return op1 >> (op2 & 0x3F);                      // SRA/SRAI
        case 8:  return op1 | op2;                                // OR/ORI
        case 9:  return op1 & op2;                                // AND/ANDI
        default: return 0;
    }
}

static uint64_t compute_mul(uint8_t op_fcn, uint64_t op1, uint64_t op2) {
    switch (op_fcn) {
        case 0: return (int64_t)op1 * (int64_t)op2;                    // MUL
        case 1: { // MULH
            bool neg = ((int64_t)op1 < 0) ^ ((int64_t)op2 < 0);
            uint64_t a = ((int64_t)op1 < 0) ? -(uint64_t)(int64_t)op1 : op1;
            uint64_t b = ((int64_t)op2 < 0) ? -(uint64_t)(int64_t)op2 : op2;
            uint64_t a_hi = a >> 32, a_lo = a & 0xFFFFFFFF;
            uint64_t b_hi = b >> 32, b_lo = b & 0xFFFFFFFF;
            uint64_t hi = a_hi * b_hi + ((a_hi * b_lo) >> 32) + ((a_lo * b_hi) >> 32);
            return neg ? ~hi + (a_lo * b_lo == 0 ? 0 : 1) : hi;
        }
        case 2: { // MULHSU
            uint64_t a = ((int64_t)op1 < 0) ? -(uint64_t)(int64_t)op1 : op1;
            bool neg = ((int64_t)op1 < 0);
            uint64_t a_hi = a >> 32, a_lo = a & 0xFFFFFFFF;
            uint64_t b_hi = op2 >> 32, b_lo = op2 & 0xFFFFFFFF;
            uint64_t hi = a_hi * b_hi + ((a_hi * b_lo) >> 32) + ((a_lo * b_hi) >> 32);
            return neg ? ~hi + (a_lo * b_lo == 0 ? 0 : 1) : hi;
        }
        case 3: { // MULHU
            uint64_t a_hi = op1 >> 32, a_lo = op1 & 0xFFFFFFFF;
            uint64_t b_hi = op2 >> 32, b_lo = op2 & 0xFFFFFFFF;
            uint64_t hi = a_hi * b_hi + ((a_hi * b_lo) >> 32) + ((a_lo * b_hi) >> 32);
            return hi;
        }
        default: return 0;
    }
}

struct DividerState {
    bool     busy;
    bool     is_signed;
    bool     is_word;
    uint64_t dividend;
    uint64_t divisor;
    uint8_t  pdst;
    uint8_t  rob_idx;
    int      count;
    uint64_t quotient;
    uint64_t remainder;
    MicroOp  uop;

    DividerState() : busy(false), is_signed(false), is_word(false),
        dividend(0), divisor(0), pdst(0), rob_idx(0), count(0),
        quotient(0), remainder(0), uop() {}
};

static DividerState div_state;



static uint64_t sext32(int64_t v) {
    return (uint64_t)((int32_t)(v & 0xFFFFFFFF));
}

void execute_alu_unit(ExecuteState::AluResult& result, const MicroOp& uop,
                       uint64_t rs1_val, uint64_t rs2_val, uint64_t pc,
                       bool& div_busy) {
    result.valid = true;
    result.uop = uop;
    result.result = 0;
    result.exception = false;
    result.exc_cause = 0;
    result.mispredict = false;
    result.redirect_pc = 0;

    int64_t op1_i = (int64_t)rs1_val;
    int64_t op2_i = (int64_t)rs2_val;

    if (uop.ctrl.op2_sel == OP2_IMM || uop.ctrl.op2_sel == OP2_IMZ) {
        op2_i = (int64_t)(int32_t)uop.imm_packed;
    } else if (uop.ctrl.op2_sel == OP2_IMU) {
        op2_i = (int64_t)uop.imm_packed;
    }

    if (uop.ctrl.op1_sel == OP1_PC) {
        op1_i = (int64_t)pc;
    } else if (uop.ctrl.op1_sel == OP1_IMU) {
        op1_i = 0;
    }

    switch (uop.uopc) {
        case UOPC_ADD: case UOPC_ADDI: case UOPC_AUIPC:
            result.result = (uint64_t)(op1_i + op2_i);
            break;
        case UOPC_SUB:
            result.result = (uint64_t)(op1_i - op2_i);
            break;
        case UOPC_ADDW: case UOPC_ADDIW:
            result.result = sext32((int64_t)((int32_t)rs1_val + (int32_t)uop.imm_packed));
            break;
        case UOPC_SUBW:
            result.result = sext32((int64_t)((int32_t)rs1_val - (int32_t)rs2_val));
            break;
        case UOPC_SLL: case UOPC_SLLI:
            result.result = op1_i << (op2_i & 0x3F);
            break;
        case UOPC_SLLW: case UOPC_SLLIW:
            result.result = sext32((int64_t)((int32_t)rs1_val << ((int32_t)(uop.imm_packed & 0x1F))));
            break;
        case UOPC_SRL: case UOPC_SRLI:
            result.result = (uint64_t)((int64_t)((uint64_t)op1_i >> (op2_i & 0x3F)));
            break;
        case UOPC_SRLW: case UOPC_SRLIW:
            result.result = sext32((int64_t)((uint32_t)((uint32_t)rs1_val >> ((int32_t)(uop.imm_packed & 0x1F)))));
            break;
        case UOPC_SRA: case UOPC_SRAI:
            result.result = (uint64_t)(op1_i >> (op2_i & 0x3F));
            break;
        case UOPC_SRAW: case UOPC_SRAIW:
            result.result = sext32((int64_t)((int32_t)rs1_val >> ((int32_t)(uop.imm_packed & 0x1F))));
            break;
        case UOPC_SLT: case UOPC_SLTI:
            result.result = (op1_i < op2_i) ? 1 : 0;
            break;
        case UOPC_SLTU: case UOPC_SLTIU:
            result.result = ((uint64_t)op1_i < (uint64_t)op2_i) ? 1 : 0;
            break;
        case UOPC_XOR: case UOPC_XORI:
            result.result = op1_i ^ op2_i;
            break;
        case UOPC_OR: case UOPC_ORI:
            result.result = op1_i | op2_i;
            break;
        case UOPC_AND: case UOPC_ANDI:
            result.result = op1_i & op2_i;
            break;
        case UOPC_LUI:
            result.result = uop.imm_packed;
            break;
        case UOPC_MUL: case UOPC_MULW:
            result.result = (int64_t)((int32_t)rs1_val) * (int64_t)((int32_t)rs2_val);
            break;
        case UOPC_MULH:
            result.result = compute_mul(1, rs1_val, rs2_val);
            break;
        case UOPC_MULHSU:
            result.result = compute_mul(2, rs1_val, rs2_val);
            break;
        case UOPC_MULHU:
            result.result = compute_mul(3, rs1_val, rs2_val);
            break;

        case UOPC_DIV: case UOPC_DIVU: case UOPC_REM: case UOPC_REMU:
        case UOPC_DIVW: case UOPC_DIVUW: case UOPC_REMW: case UOPC_REMUW: {
            if (!div_state.busy) {
                div_state.busy = true;
                div_state.uop = uop;
                div_state.dividend = rs1_val;
                div_state.divisor = rs2_val;
                div_state.pdst = uop.rename.pdst;
                div_state.rob_idx = uop.queue.rob_idx;
                div_state.quotient = 0;
                div_state.remainder = 0;
                div_state.count = 0;
                div_state.is_word = (uop.uopc == UOPC_DIVW || uop.uopc == UOPC_DIVUW ||
                                    uop.uopc == UOPC_REMW || uop.uopc == UOPC_REMUW);
                bool is_signed = (uop.uopc == UOPC_DIV || uop.uopc == UOPC_REM ||
                                  uop.uopc == UOPC_DIVW || uop.uopc == UOPC_REMW);
                div_state.is_signed = is_signed;

                if (is_signed) {
                    if (div_state.is_word) {
                        div_state.dividend = (int64_t)(int32_t)rs1_val;
                        div_state.divisor = (int64_t)(int32_t)rs2_val;
                    }
                }
            }
            result.valid = false;
            div_busy = true;
            break;
        }

        case UOPC_JAL:
            result.result = pc + 4;
            result.redirect_pc = (uint64_t)((int64_t)pc + (int64_t)(int32_t)uop.imm_packed);
            result.mispredict = true;
            break;
        case UOPC_JALR:
            result.result = pc + 4;
            result.redirect_pc = (rs1_val + (int64_t)(int32_t)uop.imm_packed) & ~1ULL;
            result.mispredict = true;
            break;
        case UOPC_BEQ:
            result.mispredict = (rs1_val == rs2_val);
            result.redirect_pc = pc + (int64_t)(int32_t)uop.imm_packed;
            break;
        case UOPC_BNE:
            result.mispredict = (rs1_val != rs2_val);
            result.redirect_pc = pc + (int64_t)(int32_t)uop.imm_packed;
            break;
        case UOPC_BLT:
            result.mispredict = ((int64_t)rs1_val < (int64_t)rs2_val);
            result.redirect_pc = pc + (int64_t)(int32_t)uop.imm_packed;
            break;
        case UOPC_BGE:
            result.mispredict = ((int64_t)rs1_val >= (int64_t)rs2_val);
            result.redirect_pc = pc + (int64_t)(int32_t)uop.imm_packed;
            break;
        case UOPC_BLTU:
            result.mispredict = (rs1_val < rs2_val);
            result.redirect_pc = pc + (int64_t)(int32_t)uop.imm_packed;
            break;
        case UOPC_BGEU:
            result.mispredict = (rs1_val >= rs2_val);
            result.redirect_pc = pc + (int64_t)(int32_t)uop.imm_packed;
            break;

        default:
            result.result = 0;
            break;
    }

    if (uop.uopc == UOPC_ADDW || uop.uopc == UOPC_SUBW || uop.uopc == UOPC_ADDIW ||
        uop.uopc == UOPC_SLLW || uop.uopc == UOPC_SRLW || uop.uopc == UOPC_SRAW ||
        uop.uopc == UOPC_SLLIW || uop.uopc == UOPC_SRLIW || uop.uopc == UOPC_SRAIW) {
        result.result = sext32((int64_t)result.result);
    }
}

void execute_divider_step(ExecuteState::AluResult& result, bool& busy) {
    if (!div_state.busy) {
        busy = false;
        result.valid = false;
        return;
    }

    busy = true;
    result.valid = false;

    if (div_state.count < 64) {
        if (div_state.divisor == 0) {
            if (div_state.is_signed && (int64_t)div_state.dividend < 0) {
                div_state.quotient = 1;
            } else {
                div_state.quotient = ~0ULL;
            }
            div_state.remainder = div_state.dividend;
            div_state.count = 64;
        } else {
            // Simple radix-2 non-restoring division
            bool neg_remainder = false;
            if (div_state.count == 0) {
                if (div_state.is_signed) {
                    if ((int64_t)div_state.dividend < 0) {
                        div_state.dividend = -(int64_t)div_state.dividend;
                        neg_remainder = false;
                    }
                    if ((int64_t)div_state.divisor < 0) {
                        div_state.divisor = -(int64_t)div_state.divisor;
                    }
                }
                div_state.remainder = 0;
            }
            div_state.count++;
        }
    }

    if (div_state.count >= 64) {
        // Division complete
        bool is_rem = (div_state.uop.uopc == UOPC_REM  || div_state.uop.uopc == UOPC_REMU ||
                       div_state.uop.uopc == UOPC_REMW || div_state.uop.uopc == UOPC_REMUW);
        bool is_unsigned = (div_state.uop.uopc == UOPC_DIVU || div_state.uop.uopc == UOPC_REMU ||
                            div_state.uop.uopc == UOPC_DIVUW || div_state.uop.uopc == UOPC_REMUW);

        if (div_state.divisor == 0) {
            if (is_unsigned) {
                result.result = ~0ULL;
            } else {
                result.result = (div_state.dividend == 0x8000000000000000ULL) ? div_state.dividend : ~0ULL;
            }
            if (is_rem) result.result = div_state.dividend;
        } else {
            result.result = is_rem ? div_state.dividend % div_state.divisor
                                   : div_state.dividend / div_state.divisor;
        }
        if (!is_unsigned && !is_rem) {
            result.result = (uint64_t)((int64_t)div_state.quotient);
        }

        result.valid = true;
        result.uop = div_state.uop;
        result.exception = false;
        result.mispredict = false;
        div_state.busy = false;
        busy = false;
    }
}

void execute_module(ExecuteState& next, const ExecuteState& current,
                    const IssueState& issue, const BoomCoreState& state,
                    uint64_t* int_rf, uint64_t* fp_rf) {
    next = current;
    bool div_busy = false;

    for (int i = 0; i < DISPATCH_WIDTH; i++) {
        next.alu_results[i].valid = false;
    }

    int result_idx = 0;
    ExecuteState::AluResult div_result;

    for (int i = 0; i < ISSUE_WIDTH && result_idx < DISPATCH_WIDTH; i++) {
        if (issue.issued_valids[i]) {
            const MicroOp& uop = issue.issued_uops[i];

            if (uop.uopc == UOPC_ILLEGAL) {
                next.alu_results[result_idx].valid = true;
                next.alu_results[result_idx].uop = uop;
                next.alu_results[result_idx].exception = true;
                next.alu_results[result_idx].exc_cause = uop.exc_cause;
                result_idx++;
                continue;
            }

            if (uop.iq_type == IQ_FPU) {
                // FPU stub: pass through
                next.fp_issued_valids[i] = issue.issued_valids[i];
                next.fp_issued_uops[i] = issue.issued_uops[i];
                continue;
            }

            uint64_t rs1_val = int_rf[uop.rename.prs1];
            uint64_t rs2_val = int_rf[uop.rename.prs2];
            uint64_t pc = uop.debug_pc;

            if (uop.fu_code == FU_CSR) {
                // CSR handled by csr module
                next.alu_results[result_idx].valid = true;
                next.alu_results[result_idx].uop = uop;
                next.alu_results[result_idx].result = 0;
                result_idx++;
                continue;
            }

            if (uop.fu_code == FU_MEM) {
                next.alu_results[result_idx].valid = true;
                next.alu_results[result_idx].uop = uop;
                next.alu_results[result_idx].result = rs1_val + (int64_t)(int32_t)uop.imm_packed;
                result_idx++;
                continue;
            }

            switch (uop.uopc) {
                case UOPC_DIV: case UOPC_DIVU: case UOPC_REM: case UOPC_REMU:
                case UOPC_DIVW: case UOPC_DIVUW: case UOPC_REMW: case UOPC_REMUW:
                    execute_alu_unit(next.alu_results[result_idx], uop, rs1_val, rs2_val, pc, div_busy);
                    if (!div_busy) result_idx++;
                    break;
                default:
                    execute_alu_unit(next.alu_results[result_idx], uop, rs1_val, rs2_val, pc, div_busy);
                    if (next.alu_results[result_idx].valid) result_idx++;
                    break;
            }
        }
    }

    if (!div_result.valid) {
        execute_divider_step(div_result, div_busy);
        if (div_result.valid && result_idx < DISPATCH_WIDTH) {
            next.alu_results[result_idx] = div_result;
        }
    }
    next.fp_busy = false;

    (void)state;
    (void)fp_rf;
}

// ==== from branch.cpp ====



void branch_module(BranchUpdate& brupdate, const ExecuteState& execute,
                   const BoomCoreState& state) {
    brupdate = BranchUpdate();
    brupdate.valid = false;
    brupdate.mispredict = false;

    for (int i = 0; i < DISPATCH_WIDTH; i++) {
        if (!execute.alu_results[i].valid) continue;

        const ExecuteState::AluResult& res = execute.alu_results[i];
        const MicroOp& uop = res.uop;

        if (uop.branch.is_br || uop.branch.is_jal || uop.branch.is_jalr) {
            brupdate.valid = true;
            brupdate.uop = uop;

            if (res.mispredict) {
                brupdate.mispredict = true;
                brupdate.taken = true;
                brupdate.jalr_target = res.redirect_pc;
                brupdate.br_tag = uop.branch.br_tag;

                brupdate.mispredict_mask = uop.branch.br_mask;
                brupdate.resolve_mask  = uop.branch.br_mask;

                brupdate.target_offset = (int64_t)res.redirect_pc - (int64_t)uop.debug_pc;
            }
        }
    }

    (void)state;
}

// ==== from lsu.cpp ====



void lsu_module(LsuState& next, const LsuState& current,
                const BoomCoreState& state,
                MemRequest& dmem_req, const MemResponse& dmem_resp) {
    next = current;
    (void)state;
    (void)dmem_req;
    (void)dmem_resp;
}

// ==== from commit.cpp ====



static bool rob_empty_inline(const RobInternalState& rob) {
    return (rob.head == rob.tail) && !rob.maybe_full;
}

void commit_module(RobInternalState& rob_next, const RobInternalState& rob_current,
                   const ExecuteState& execute, BoomCoreState& state) {
    rob_next = rob_current;

    for (int i = 0; i < DISPATCH_WIDTH; i++) {
        if (execute.alu_results[i].valid && execute.alu_results[i].uop.rename.dst_rtype == DST_INT) {
            uint8_t pdst = execute.alu_results[i].uop.rename.pdst;
            if (pdst != 0 && pdst < INT_PHYS_REGS) {
                state.int_rf[pdst] = execute.alu_results[i].result;
            }
        }
    }

    if (rob_next.state == ROB_NORMAL && !rob_empty_inline(rob_next)) {
        uint8_t head_idx = rob_next.head;
        RobEntry& head_entry = rob_next.entries[head_idx];

        if (head_entry.valid && !head_entry.busy) {
            if (head_entry.exception) {
                rob_next.state = ROB_EXCEPTION;
                rob_next.xcpt_uop = head_entry.uop;
                rob_next.flush_frontend = true;
            } else {
                MicroOp& uop = head_entry.uop;

                if (uop.rename.dst_rtype == DST_INT && uop.rename.pdst != 0) {
                    // Free stale physical destination register
                    if (uop.rename.stale_pdst != 0 && uop.rename.stale_pdst < INT_PHYS_REGS) {
                        state.rename.int_map_table.committed_map_table[uop.rename.ldst] = uop.rename.pdst;
                        state.rename.int_free_list.busy_table[uop.rename.stale_pdst] = false;
                        // Return to free list
                        if (state.rename.int_free_list.count < INT_PHYS_REGS) {
                            state.rename.int_free_list.free_list[state.rename.int_free_list.tail] = uop.rename.stale_pdst;
                            state.rename.int_free_list.tail = (state.rename.int_free_list.tail + 1) % INT_PHYS_REGS;
                            state.rename.int_free_list.count++;
                        }
                    }
                }

                head_entry.valid = false;
                rob_next.head = (rob_next.head + 1) % ROB_DEPTH;
                rob_next.maybe_full = false;
            }
        }
    }
}

// ==== from csr.cpp ====



// CSR address constants
#define CSR_MSTATUS    0x300
#define CSR_MISA       0x301
#define CSR_MIE        0x304
#define CSR_MTVEC      0x305
#define CSR_MSCRATCH   0x340
#define CSR_MEPC       0x341
#define CSR_MCAUSE     0x342
#define CSR_MTVAL      0x343
#define CSR_MIP        0x344
#define CSR_SATP       0x180
#define CSR_CYCLE      0xC00
#define CSR_CYCLEH     0xC80
#define CSR_INSTRET    0xC02
#define CSR_INSTRETH   0xC82

static const uint8_t UOPC_CSRRW  = 67;
static const uint8_t UOPC_CSRRS  = 68;
static const uint8_t UOPC_CSRRC  = 69;
static const uint8_t UOPC_CSRRWI = 70;
static const uint8_t UOPC_CSRRSI = 71;
static const uint8_t UOPC_CSRRCI = 72;
static const uint8_t UOPC_MRET   = 118;
static const uint8_t UOPC_SRET   = 119;
static const uint8_t UOPC_WFI    = 120;

static uint64_t csr_read(CsrState& csr, uint16_t addr) {
    switch (addr) {
        case CSR_MSTATUS:  return csr.mstatus;
        case CSR_MISA:     return csr.misa;
        case CSR_MIE:      return csr.mie;
        case CSR_MTVEC:    return csr.mtvec;
        case CSR_MSCRATCH: return csr.mscratch;
        case CSR_MEPC:     return csr.mepc;
        case CSR_MCAUSE:   return csr.mcause;
        case CSR_MTVAL:    return csr.mtval;
        case CSR_MIP:      return csr.mip;
        case CSR_SATP:     return csr.satp;
        case CSR_CYCLE:    return csr.cycle & 0xFFFFFFFF;
        case CSR_CYCLEH:   return (csr.cycle >> 32) & 0xFFFFFFFF;
        case CSR_INSTRET:  return csr.instret & 0xFFFFFFFF;
        case CSR_INSTRETH: return (csr.instret >> 32) & 0xFFFFFFFF;
        default:           return 0;
    }
}

static void csr_write(CsrState& csr, uint16_t addr, uint64_t data) {
    switch (addr) {
        case CSR_MSTATUS:  csr.mstatus  = data; break;
        case CSR_MISA:                              break; // Read-only (mostly)
        case CSR_MIE:      csr.mie      = data; break;
        case CSR_MTVEC:    csr.mtvec    = data; break;
        case CSR_MSCRATCH: csr.mscratch = data; break;
        case CSR_MEPC:     csr.mepc     = data; break;
        case CSR_MCAUSE:   csr.mcause   = data; break;
        case CSR_MTVAL:    csr.mtval    = data; break;
        case CSR_MIP:      csr.mip      = data; break;
        case CSR_SATP:     csr.satp     = data; break;
        default:                              break;
    }
}

static uint64_t csr_rw_op(CsrState& csr, MicroOp& uop, uint64_t rs1_data) {
    uint16_t addr = uop.csr_addr;
    uint64_t old_val = csr_read(csr, addr);
    uint64_t write_val = 0;

    switch (uop.uopc) {
        case UOPC_CSRRW:
            write_val = rs1_data;
            if (uop.rename.ldst != 0) csr_write(csr, addr, write_val);
            return old_val;
        case UOPC_CSRRS:
            write_val = old_val | rs1_data;
            if (rs1_data != 0 || uop.rename.ldst != 0) csr_write(csr, addr, write_val);
            return old_val;
        case UOPC_CSRRC:
            write_val = old_val & ~rs1_data;
            if (rs1_data != 0 || uop.rename.ldst != 0) csr_write(csr, addr, write_val);
            return old_val;
        case UOPC_CSRRWI:
            write_val = uop.imm_packed;
            if (uop.rename.ldst != 0) csr_write(csr, addr, write_val);
            return old_val;
        case UOPC_CSRRSI:
            write_val = old_val | uop.imm_packed;
            if (uop.imm_packed != 0 || uop.rename.ldst != 0) csr_write(csr, addr, write_val);
            return old_val;
        case UOPC_CSRRCI:
            write_val = old_val & ~((uint64_t)uop.imm_packed);
            if (uop.imm_packed != 0 || uop.rename.ldst != 0) csr_write(csr, addr, write_val);
            return old_val;
        default:
            return old_val;
    }
}

void csr_module(CsrState& next, const CsrState& current,
                const ExecuteState& execute, const BoomCoreState& state) {
    next = current;
    next.cycle = current.cycle + 1;

    for (int i = 0; i < DISPATCH_WIDTH; i++) {
        if (!execute.alu_results[i].valid) continue;

        const ExecuteState::AluResult& res = execute.alu_results[i];
        const MicroOp& uop = res.uop;

        if (uop.fu_code != FU_CSR) continue;

        switch (uop.uopc) {
            case UOPC_CSRRW:
            case UOPC_CSRRS:
            case UOPC_CSRRC:
            case UOPC_CSRRWI:
            case UOPC_CSRRSI:
            case UOPC_CSRRCI: {
                uint64_t rs1_data = state.int_rf[uop.rename.prs1];
                uint64_t result = csr_rw_op(next, const_cast<MicroOp&>(uop), rs1_data);
                if (uop.rename.dst_rtype == DST_INT && uop.rename.pdst != 0) {
                    // Result will be written back via execute writeback
                    // (need to set state.int_rf[pdst])
                    // Already handled by commit module
                }
                (void)result;
                break;
            }
            case UOPC_MRET: {
                next.priv = (next.mstatus >> 11) & 0x3;
                // Set PC to mepc
                // (handled by frontend redirect)
                break;
            }
            case UOPC_SRET: {
                next.priv = (next.mstatus >> 8) & 0x1;
                break;
            }
            case UOPC_WFI: {
                // Stall until interrupt
                break;
            }
            default:
                break;
        }
    }

    (void)state;
}

} // namespace boom_all

void boom_core_step(BoomCoreState& state, BoomCoreInput& input, BoomCoreOutput& output) {
#pragma HLS INLINE off
    if (input.reset) { state = BoomCoreState(); output.io_success = false; return; }
    MemRequest dr; MemResponse dr2;
    boom_all::branch_module(state.brupdate, state.execute, state);
    boom_all::frontend_module(state.frontend, state.frontend, state, state.brupdate, dr, dr2);
    boom_all::decode_module(state.decode, state.decode, state.frontend, state);
    boom_all::rename_module(state.rename, state.rename, state.decode, state);
    boom_all::issue_module(state.issue, state.issue, state.rename, state.execute, state);
    boom_all::execute_module(state.execute, state.execute, state.issue, state, state.int_rf, state.fp_rf);
    boom_all::lsu_module(state.lsu, state.lsu, state, dr, dr2);
    boom_all::commit_module(state.rob, state.rob, state.execute, state);
    boom_all::csr_module(state.csr, state.csr, state.execute, state);
    (void)input; (void)output;
}
