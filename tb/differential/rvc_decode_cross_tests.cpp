#include "rvc.hpp"
#include "boom_state.hpp"

#include <cstdint>
#include <cstdio>

namespace boom { void decode_module(BoomCoreState& state); }

struct Expected {
    uint8_t uopc;
    uint8_t iq;
    uint8_t fu;
    uint8_t dst_type;
    uint8_t op1;
    uint8_t op2;
    uint32_t immediate;
    bool branch;
    bool jal;
    bool jalr;
    bool load;
    bool store;
    uint8_t memory_size;
    bool exception;
    uint64_t exc_cause;
};

static uint32_t imm_i(uint32_t inst) { return uint32_t(int32_t(inst) >> 20); }
static uint32_t imm_s(uint32_t inst) {
    uint32_t value = ((inst >> 25) << 5) | ((inst >> 7) & 0x1f);
    return value & 0x800 ? value | 0xfffff000u : value;
}
static uint32_t imm_b(uint32_t inst) {
    uint32_t value = ((inst >> 31) << 12) | (((inst >> 7) & 1) << 11) |
                     (((inst >> 25) & 0x3f) << 5) | (((inst >> 8) & 0xf) << 1);
    return value & 0x1000 ? value | 0xffffe000u : value;
}
static uint32_t imm_j(uint32_t inst) {
    uint32_t value = ((inst >> 31) << 20) | (((inst >> 12) & 0xff) << 12) |
                     (((inst >> 20) & 1) << 11) | (((inst >> 21) & 0x3ff) << 1);
    return value & 0x100000 ? value | 0xffe00000u : value;
}

static bool expected_decode(uint32_t inst, Expected& e, const char*& family) {
    const uint8_t opcode = inst & 0x7f;
    const uint8_t f3 = (inst >> 12) & 7;
    const uint8_t f7 = (inst >> 25) & 0x7f;
    const uint8_t rd = (inst >> 7) & 0x1f;
    e = Expected{0, IQ_ALU, FU_ALU, DST_INT, OP1_RS1, OP2_RS2, 0,
                 false, false, false, false, false, 0, false, 0};
    if (inst == 0x00100073u) {
        family = "ebreak"; e.uopc = 66; e.fu = FU_CSR; e.dst_type = DST_N;
        e.exception = true; e.exc_cause = 3; return true;
    }
    switch (opcode) {
    case 0x37:
        family="lui"; e.uopc=37; e.op1=OP1_X0; e.op2=OP2_IMU;
        e.immediate=inst&0xfffff000u; return true;
    case 0x6f:
        family="jal"; e.uopc=29; e.op1=OP1_PC; e.op2=OP2_IMM;
        e.dst_type=rd ? DST_INT : DST_X0; e.immediate=imm_j(inst); e.jal=true; return true;
    case 0x67:
        family="jalr"; e.uopc=30; e.op2=OP2_IMM;
        e.dst_type=rd ? DST_INT : DST_X0; e.immediate=imm_i(inst); e.jalr=true; return true;
    case 0x63:
        family=f3 ? "bnez" : "beqz"; e.uopc=f3 ? 32 : 31;
        e.dst_type=DST_N; e.immediate=imm_b(inst); e.branch=true; return f3 <= 1;
    case 0x03:
        family=f3==2 ? "lw" : "ld"; e.uopc=f3==2 ? 41 : 42;
        e.iq=IQ_MEM; e.fu=FU_MEM; e.op2=OP2_IMM; e.immediate=imm_i(inst);
        e.load=true; e.memory_size=f3; return f3==2 || f3==3;
    case 0x23:
        family=f3==2 ? "sw" : "sd"; e.uopc=f3==2 ? 48 : 49;
        e.iq=IQ_MEM; e.fu=FU_MEM; e.immediate=imm_s(inst);
        e.dst_type=DST_N; e.store=true; e.memory_size=f3; return f3==2 || f3==3;
    case 0x13:
        e.op2=OP2_IMM; e.immediate=imm_i(inst);
        if (f3==0) { family="addi"; e.uopc=50; return true; }
        if (f3==1) { family="slli"; e.uopc=51; return true; }
        if (f3==7) { family="andi"; e.uopc=58; return true; }
        if (f3==5 && (f7==0 || f7==1)) { family="srli"; e.uopc=55; return true; }
        if (f3==5 && (f7==0x20 || f7==0x21)) { family="srai"; e.uopc=56; return true; }
        family="unsupported_shift"; return false;
    case 0x1b:
        family="addiw"; e.uopc=59; e.op2=OP2_IMM; e.immediate=imm_i(inst); return f3==0;
    case 0x33:
        family="alu";
        if (f3==0) e.uopc=f7==0x20 ? 2 : 1;
        else if (f3==4) e.uopc=6;
        else if (f3==6) e.uopc=9;
        else if (f3==7) e.uopc=10;
        else return false;
        return f7==0 || (f7==0x20 && f3==0);
    case 0x3b:
        family="aluw"; e.uopc=f7==0x20 ? 12 : 11;
        return f3==0 && (f7==0 || f7==0x20);
    default: family="unexpected"; return false;
    }
}

int main() {
    unsigned checked=0, failures=0;
    unsigned arithmetic=0, control=0, memory=0;
    for (uint32_t raw=0; raw<=0xffffu; ++raw) {
        const boom::RvcDecodeResult expanded=boom::decompress_rvc(uint16_t(raw));
        if (!expanded.valid || !expanded.legal) continue;
        Expected expected;
        const char* family="";
        if (!expected_decode(expanded.instruction, expected, family)) {
            if (failures<20) std::printf("CROSS_UNEXPECTED c=%04x inst=%08x family=%s\n",
                                         raw,expanded.instruction,family);
            ++failures;
            continue;
        }
        BoomCoreState state;
        state.frontend.fetch_packet_valid=true;
        state.frontend.fetch_uop.inst=expanded.instruction;
        state.frontend.fetch_uop.debug_pc=0x80000000ull + 2ull*checked;
        boom::decode_module(state);
        const MicroOp& u=state.decode.dec_uops[0];
        const bool sources=u.rename.lrs1==((expanded.instruction>>15)&31) &&
                           u.rename.lrs2==((expanded.instruction>>20)&31);
        const bool destination=u.rename.ldst==((expanded.instruction>>7)&31) &&
                               u.rename.dst_rtype==expected.dst_type;
        const bool controls=u.uopc==expected.uopc && u.iq_type==expected.iq &&
                            u.fu_code==expected.fu && u.ctrl.op1_sel==expected.op1 &&
                            u.ctrl.op2_sel==expected.op2 && u.imm_packed==expected.immediate;
        const bool branches=u.branch.is_br==expected.branch &&
                            u.branch.is_jal==expected.jal && u.branch.is_jalr==expected.jalr;
        const bool mem=u.ctrl.is_load==expected.load && u.ctrl.is_sta==expected.store &&
                       (!expected.load || (u.mem.uses_ldq && u.mem.mem_signed &&
                                           u.mem.mem_size==expected.memory_size)) &&
                       (!expected.store || (u.mem.uses_stq &&
                                            u.mem.mem_size==expected.memory_size));
        const bool ok=state.decode.dec_valids[0] && sources && destination && controls &&
                       branches && mem && u.exception==expected.exception &&
                       u.exc_cause==expected.exc_cause;
        ++checked;
        if (expected.load || expected.store) ++memory;
        else if (expected.branch || expected.jal || expected.jalr) ++control;
        else ++arithmetic;
        if (!ok) {
            if (failures<20)
                std::printf("CROSS_FAIL c=%04x inst=%08x family=%s uopc=%u/%u exc=%u\n",
                            raw,expanded.instruction,family,u.uopc,expected.uopc,u.exception);
            ++failures;
        }
    }
    std::printf("RVC_DECODE_CROSS checked=%u failures=%u arithmetic=%u control=%u memory=%u\n",
                checked,failures,arithmetic,control,memory);
    std::printf("RVC_DECODE_GAPS_CLOSED checked=%u expected=38551\n", checked);
    if (checked!=38551 || failures) return 1;
    std::printf("GATE5_2_R1_RVC_DECODE_CROSS_PASS\n");
    return 0;
}
