#include "boom_config.hpp"
#include "boom_types.hpp"
#include "boom_state.hpp"

namespace boom {

#define UOPC_NOP      0
#define UOPC_ADD      1
#define UOPC_SUB      2
#define UOPC_SLL      3
#define UOPC_SLT      4
#define UOPC_SLTU     5
#define UOPC_XOR      6
#define UOPC_SRL      7
#define UOPC_SRA      8
#define UOPC_OR       9
#define UOPC_AND      10
#define UOPC_ADDW     11
#define UOPC_SUBW     12
#define UOPC_SLLW     13
#define UOPC_SRLW     14
#define UOPC_SRAW     15
#define UOPC_MUL      16
#define UOPC_MULH     17
#define UOPC_MULHSU   18
#define UOPC_MULHU    19
#define UOPC_MULW     20
#define UOPC_DIV      21
#define UOPC_DIVU     22
#define UOPC_REM      23
#define UOPC_REMU     24
#define UOPC_DIVW     25
#define UOPC_DIVUW    26
#define UOPC_REMW     27
#define UOPC_REMUW    28
#define UOPC_JAL      29
#define UOPC_JALR     30
#define UOPC_BEQ      31
#define UOPC_BNE      32
#define UOPC_BLT      33
#define UOPC_BGE      34
#define UOPC_BLTU     35
#define UOPC_BGEU     36
#define UOPC_LUI      37
#define UOPC_AUIPC    38
#define UOPC_LB       39
#define UOPC_LH       40
#define UOPC_LW       41
#define UOPC_LD       42
#define UOPC_LBU      43
#define UOPC_LHU      44
#define UOPC_LWU      45
#define UOPC_SB       46
#define UOPC_SH       47
#define UOPC_SW       48
#define UOPC_SD       49
#define UOPC_ADDI     50
#define UOPC_SLLI     51
#define UOPC_SLTI     52
#define UOPC_SLTIU    53
#define UOPC_XORI     54
#define UOPC_SRLI     55
#define UOPC_SRAI     56
#define UOPC_ORI      57
#define UOPC_ANDI     58
#define UOPC_ADDIW    59
#define UOPC_SLLIW    60
#define UOPC_SRLIW    61
#define UOPC_SRAIW    62
#define UOPC_FENCE    63
#define UOPC_FENCEI   64
#define UOPC_ECALL    65
#define UOPC_EBREAK   66
#define UOPC_CSRRW    67
#define UOPC_CSRRS    68
#define UOPC_CSRRC    69
#define UOPC_CSRRWI   70
#define UOPC_CSRRSI   71
#define UOPC_CSRRCI   72
#define UOPC_MRET     118
#define UOPC_WFI      120
#define UOPC_ILLEGAL  255

static uint32_t extract_imm_i(uint32_t inst) { int32_t v=(int32_t)(inst)>>20; return (uint32_t)v; }
static uint32_t extract_imm_s(uint32_t inst) {
    int32_t v=((int32_t)(inst)>>20)&0xFFFFFE0; v|=(inst>>7)&0x1F; if(v&0x800) v|=0xFFFFF000; return (uint32_t)v; }
static uint32_t extract_imm_b(uint32_t inst) {
    int32_t v=((inst>>8)&0xF)<<1; v|=((inst>>25)&0x3F)<<5; v|=((inst>>7)&0x1)<<11; v|=((inst>>31)&0x1)<<12;
    if(v&0x1000) v|=0xFFFFE000; return (uint32_t)v; }
static uint32_t extract_imm_u(uint32_t inst) { return inst&0xFFFFF000; }
static uint32_t extract_imm_j(uint32_t inst) {
    int32_t v=((inst>>21)&0x3FF)<<1; v|=((inst>>20)&0x1)<<11; v|=((inst>>12)&0xFF)<<12; v|=((inst>>31)&0x1)<<20;
    if(v&0x100000) v|=0xFFE00000; return (uint32_t)v; }

void decode_module(BoomCoreState& state) {
    DecodeState& dec = state.decode;
    dec.dec_valids[0] = false;

    if (state.global_flush) return;
    if (!state.frontend.fetch_packet_valid) return;

    uint64_t pc = state.frontend.fetch_uop.debug_pc;
    uint32_t inst = state.frontend.fetch_uop.inst;

    MicroOp uop;
    uop.ctrl = DecodeControl();
    uop.branch = BranchInfo();
    uop.mem = MemoryInfo();
    uop.inst = inst;
    uop.debug_pc = pc;
    uop.rename.lrs1 = (inst>>15)&0x1F;
    uop.rename.lrs2 = (inst>>20)&0x1F;
    uop.rename.ldst = (inst>>7)&0x1F;
    uop.branch.pc_lob = pc & 0x3F;

    uint8_t opcode = inst & 0x7F;
    uint8_t f3 = (inst>>12)&0x7;
    uint8_t f7 = (inst>>25)&0x7F;
    uint8_t rd = (inst>>7)&0x1F;

    switch (opcode) {
        case 0x37: uop.uopc=UOPC_LUI; uop.iq_type=IQ_ALU; uop.fu_code= FU_ALU;
            uop.ctrl.op1_sel=OP1_X0; uop.ctrl.op2_sel=OP2_IMU; uop.imm_packed=extract_imm_u(inst);
            uop.rename.dst_rtype=DST_INT; break;
        case 0x17: uop.uopc=UOPC_AUIPC; uop.iq_type=IQ_ALU; uop.fu_code= FU_ALU;
            uop.ctrl.op1_sel=OP1_PC; uop.ctrl.op2_sel=OP2_IMU; uop.imm_packed=extract_imm_u(inst);
            uop.rename.dst_rtype=DST_INT; break;
        case 0x6F: uop.uopc=UOPC_JAL; uop.iq_type=IQ_ALU; uop.fu_code= FU_ALU;
            uop.ctrl.op1_sel=OP1_PC; uop.ctrl.op2_sel=OP2_IMM; uop.ctrl.br_type=BR_J;
            uop.imm_packed=extract_imm_j(inst); uop.rename.dst_rtype=(rd!=0)?DST_INT:DST_X0;
            uop.branch.is_jal=true; break;
        case 0x67: uop.uopc=UOPC_JALR; uop.iq_type=IQ_ALU; uop.fu_code= FU_ALU;
            uop.ctrl.op1_sel=OP1_RS1; uop.ctrl.op2_sel=OP2_IMM; uop.ctrl.br_type=BR_JR;
            uop.imm_packed=extract_imm_i(inst); uop.rename.dst_rtype=(rd!=0)?DST_INT:DST_X0;
            uop.branch.is_jalr=true; break;
        case 0x63: uop.iq_type=IQ_ALU; uop.fu_code= FU_ALU;
            uop.ctrl.op1_sel=OP1_RS1; uop.ctrl.op2_sel=OP2_RS2; uop.imm_packed=extract_imm_b(inst);
            uop.branch.is_br=true; uop.rename.dst_rtype=DST_N;
            switch(f3) { case 0:uop.uopc=UOPC_BEQ;uop.ctrl.br_type=BR_EQ;break;
            case 1:uop.uopc=UOPC_BNE;uop.ctrl.br_type=BR_NE;break;
            case 4:uop.uopc=UOPC_BLT;uop.ctrl.br_type=BR_LT;break;
            case 5:uop.uopc=UOPC_BGE;uop.ctrl.br_type=BR_GE;break;
            case 6:uop.uopc=UOPC_BLTU;uop.ctrl.br_type=BR_LTU;break;
            case 7:uop.uopc=UOPC_BGEU;uop.ctrl.br_type=BR_GEU;break;
            default:uop.uopc=UOPC_ILLEGAL;uop.exception=true;uop.exc_cause=2;break;} break;
        case 0x03: uop.iq_type=IQ_MEM; uop.fu_code= FU_MEM;
            uop.ctrl.op1_sel=OP1_RS1; uop.ctrl.op2_sel=OP2_IMM; uop.ctrl.is_load=true; uop.ctrl.is_sta=false;
            uop.imm_packed=extract_imm_i(inst); uop.rename.dst_rtype=DST_INT;
            uop.mem.mem_signed=true; uop.mem.uses_ldq=true;
            switch(f3) { case 0:uop.uopc=UOPC_LB;uop.mem.mem_size=0;break;
            case 1:uop.uopc=UOPC_LH;uop.mem.mem_size=1;break;
            case 2:uop.uopc=UOPC_LW;uop.mem.mem_size=2;break;
            case 3:uop.uopc=UOPC_LD;uop.mem.mem_size=3;break;
            case 4:uop.uopc=UOPC_LBU;uop.mem.mem_size=0;uop.mem.mem_signed=false;break;
            case 5:uop.uopc=UOPC_LHU;uop.mem.mem_size=1;uop.mem.mem_signed=false;break;
            case 6:uop.uopc=UOPC_LWU;uop.mem.mem_size=2;uop.mem.mem_signed=false;break;
            default:uop.uopc=UOPC_ILLEGAL;uop.exception=true;uop.exc_cause=2;break;} break;
        case 0x23: uop.iq_type=IQ_MEM; uop.fu_code= FU_MEM;
            uop.ctrl.op1_sel=OP1_RS1; uop.ctrl.op2_sel=OP2_RS2; uop.ctrl.is_load=false; uop.ctrl.is_sta=true;
            uop.imm_packed=extract_imm_s(inst); uop.mem.uses_stq=true; uop.rename.dst_rtype=DST_N;
            switch(f3) { case 0:uop.uopc=UOPC_SB;uop.mem.mem_size=0;break;
            case 1:uop.uopc=UOPC_SH;uop.mem.mem_size=1;break;
            case 2:uop.uopc=UOPC_SW;uop.mem.mem_size=2;break;
            case 3:uop.uopc=UOPC_SD;uop.mem.mem_size=3;break;
            default:uop.uopc=UOPC_ILLEGAL;uop.exception=true;uop.exc_cause=2;break;} break;
        case 0x13: uop.iq_type=IQ_ALU; uop.fu_code= FU_ALU;
            uop.ctrl.op1_sel=OP1_RS1; uop.ctrl.op2_sel=OP2_IMM; uop.imm_packed=extract_imm_i(inst);
            uop.rename.dst_rtype=DST_INT;
            switch(f3) { case 0:uop.uopc=UOPC_ADDI;break;
            case 2:uop.uopc=UOPC_SLTI;break; case 3:uop.uopc=UOPC_SLTIU;break;
            case 4:uop.uopc=UOPC_XORI;break; case 6:uop.uopc=UOPC_ORI;break;
            case 7:uop.uopc=UOPC_ANDI;break;
            case 1:uop.uopc=UOPC_SLLI;break;
            case 5:uop.uopc=(f7==0)?UOPC_SRLI:UOPC_SRAI;break;
            default:uop.uopc=UOPC_ILLEGAL;uop.exception=true;break;} break;
        case 0x1B: uop.iq_type=IQ_ALU; uop.fu_code= FU_ALU;
            uop.ctrl.op1_sel=OP1_RS1; uop.ctrl.op2_sel=OP2_IMM; uop.imm_packed=extract_imm_i(inst);
            uop.ctrl.fcn_dw=1; uop.rename.dst_rtype=DST_INT;
            switch(f3) { case 0:uop.uopc=UOPC_ADDIW;break;
            case 1:uop.uopc=UOPC_SLLIW;break;
            case 5:uop.uopc=(f7==0)?UOPC_SRLIW:UOPC_SRAIW;break;
            default:uop.uopc=UOPC_ILLEGAL;uop.exception=true;break;} break;
        case 0x33: uop.iq_type=IQ_ALU; uop.ctrl.op1_sel=OP1_RS1; uop.ctrl.op2_sel=OP2_RS2;
            if (f7==1) { uop.fu_code= FU_MUL; uop.rename.dst_rtype=DST_INT;
                switch(f3) { case 0:uop.uopc=UOPC_MUL;break;
                default:uop.uopc=UOPC_ILLEGAL;uop.exception=true;break;}}
            else { uop.fu_code= FU_ALU; uop.rename.dst_rtype=DST_INT;
                switch(f3) { case 0:uop.uopc=(f7==0x20)?UOPC_SUB:UOPC_ADD;break;
                case 1:uop.uopc=UOPC_SLL;break; case 2:uop.uopc=UOPC_SLT;break;
                case 3:uop.uopc=UOPC_SLTU;break; case 4:uop.uopc=UOPC_XOR;break;
                case 5:uop.uopc=(f7==0x20)?UOPC_SRA:UOPC_SRL;break;
                case 6:uop.uopc=UOPC_OR;break; case 7:uop.uopc=UOPC_AND;break;
                default:uop.uopc=UOPC_ILLEGAL;uop.exception=true;break;}} break;
        case 0x3B: uop.iq_type=IQ_ALU; uop.ctrl.op1_sel=OP1_RS1; uop.ctrl.op2_sel=OP2_RS2;
            uop.fu_code= FU_ALU; uop.rename.dst_rtype=DST_INT; uop.ctrl.fcn_dw=1;
            switch(f3) { case 0:uop.uopc=(f7==0x20)?UOPC_SUBW:UOPC_ADDW;break;
            case 1:uop.uopc=UOPC_SLLW;break;
            case 5:uop.uopc=(f7==0x20)?UOPC_SRAW:UOPC_SRLW;break;
            default:uop.uopc=UOPC_ILLEGAL;uop.exception=true;break;} break;
        case 0x0F: uop.uopc=UOPC_FENCE; uop.iq_type=IQ_MEM; uop.fu_code= FU_MEM;
            uop.rename.dst_rtype=DST_N; break;
        case 0x73: if(f3==0) { if(rd==0&&(inst>>20)==0) {uop.uopc=UOPC_ECALL;
            uop.iq_type=IQ_ALU; uop.fu_code= FU_CSR; uop.exception=true; uop.exc_cause=11;
            uop.is_sys_pc2epc=true;} else {uop.uopc=UOPC_MRET; uop.iq_type=IQ_ALU;
            uop.fu_code= FU_CSR; uop.rename.dst_rtype=DST_N;}}
            else { uop.iq_type=IQ_ALU; uop.fu_code= FU_CSR;
            uop.ctrl.op1_sel=OP1_RS1; uop.ctrl.op2_sel=OP2_IMZ;
            uop.csr_addr=inst>>20; uop.rename.dst_rtype=DST_INT;
            switch(f3) { case 1:uop.uopc=UOPC_CSRRW;uop.ctrl.csr_cmd=CSR_W;break;
            case 2:uop.uopc=UOPC_CSRRS;uop.ctrl.csr_cmd=CSR_S;break;
            case 3:uop.uopc=UOPC_CSRRC;uop.ctrl.csr_cmd=CSR_C;break;
            case 5:uop.uopc=UOPC_CSRRWI;uop.ctrl.csr_cmd=CSR_W;break;
            case 6:uop.uopc=UOPC_CSRRSI;uop.ctrl.csr_cmd=CSR_S;break;
            case 7:uop.uopc=UOPC_CSRRCI;uop.ctrl.csr_cmd=CSR_C;break;
            default:uop.uopc=UOPC_ILLEGAL;uop.exception=true;break;}} break;
        default: uop.uopc=UOPC_ILLEGAL; uop.exception=true; uop.exc_cause=2; uop.iq_type=IQ_ALU; uop.fu_code=FU_ALU; break;
    }

    if (uop.uopc==UOPC_ILLEGAL && !uop.exception) { uop.exception=true; uop.exc_cause=2; }

    dec.dec_uops[0] = uop;
    dec.dec_valids[0] = true;
}

}
