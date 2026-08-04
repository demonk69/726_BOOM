#include "boom_config.hpp"
#include "boom_types.hpp"
#include "boom_state.hpp"
#include "issue.hpp"
#include "completion.hpp"

namespace boom {

static uint64_t sext32(int64_t v) { return (uint64_t)((int32_t)(v&0xFFFFFFFF)); }
static uint8_t mask_for_size(uint8_t size) { return (uint8_t)((size>=3) ? 0xFF : ((1u << (1u << size)) - 1u)); }

static uint64_t execute_operand(const BoomCoreState& state, uint8_t lane,
                                uint8_t prs, uint64_t issued_data) {
#pragma HLS INLINE
    if (prs == 0) return 0;
    if (prs >= INT_PHYS_REGS) return 0;
    uint64_t value = 0;
    bool conflict = false;
    if (boom::bypass_lookup(state, prs, value, conflict)) return value;
    if (conflict) return 0;
    if (!state.rename.int_free_list.busy_table[prs]) return prf_read(state, prs);
    (void)lane;
    return issued_data;
}

void execute_module(BoomCoreState& state) {
#ifdef BOOM_HLS_W3_DIAGNOSTIC
#pragma HLS INLINE
#endif
    ExecuteState& exe = state.execute;
    const IssueState& iss = state.issue;

    exe.alu_results[FP_ISSUE_LANE]=ExecuteState::AluResult();

    for (int i=0; i<INTEGER_ISSUE_PORTS; i++) {
        if (!iss.issued_valids[i]) continue;
        const MicroOp& uop = iss.issued_uops[i];
        if (classify_issue_port(uop)==ISSUE_PORT_UNSUPPORTED) continue;
        if (exe.alu_results[i].valid) continue;
        if (state.brupdate.valid && state.brupdate.mispredict &&
            ((uop.branch.br_mask & state.brupdate.mispredict_mask) != 0)) continue;

        uint64_t rs1 = execute_operand(state, (uint8_t)i, uop.rename.prs1,
                                       iss.issued_prs1_data[i]);
        uint64_t rs2 = execute_operand(state, (uint8_t)i, uop.rename.prs2,
                                       iss.issued_prs2_data[i]);
        uint64_t pc = uop.debug_pc;

        int64_t op1=(int64_t)rs1, op2=(int64_t)rs2;
        if (uop.ctrl.op1_sel==OP1_PC) op1=(int64_t)pc;
        if (uop.ctrl.op2_sel==OP2_IMM||uop.ctrl.op2_sel==OP2_IMZ) op2=(int64_t)(int32_t)uop.imm_packed;
        if (uop.ctrl.op2_sel==OP2_IMU) op2=(int64_t)uop.imm_packed;

        ExecuteState::AluResult& r = exe.alu_results[i];
        r = ExecuteState::AluResult();
        r.valid=true; r.uop=uop; r.exception=false; r.mispredict=false; r.result=0;

        switch (uop.uopc) {
            case 1: case 50: r.result=(uint64_t)(op1+op2); break;
            case 2: r.result=(uint64_t)(op1-op2); break;
            case 3: case 51: r.result=op1<<(op2&0x3F); break;
            case 4: case 52: r.result=(op1<op2)?1:0; break;
            case 5: case 53: r.result=(rs1<rs2)?1:0; break;
            case 6: case 54: r.result=op1^op2; break;
            case 7: case 55: r.result=rs1>>(op2&0x3F); break;
            case 8: case 56: r.result=op1>>(op2&0x3F); break;
            case 9: case 57: r.result=op1|op2; break;
            case 10: case 58: r.result=op1&op2; break;
            case 11: case 59: r.result=sext32((int64_t)((int32_t)rs1+(int32_t)uop.imm_packed)); break;
            case 12: r.result=sext32((int64_t)((int32_t)rs1-(int32_t)rs2)); break;
            case 13: case 60: r.result=sext32((int64_t)((int32_t)rs1<<((int32_t)(uop.imm_packed&0x1F)))); break;
            case 15: case 62: r.result=sext32((int64_t)((int32_t)rs1>>((int32_t)(uop.imm_packed&0x1F)))); break;
            case 16: r.result=(int64_t)((int32_t)rs1)*(int64_t)((int32_t)rs2); break;
            case 29: r.result=pc+4; r.mispredict=true; r.redirect_pc=(uint64_t)((int64_t)pc+(int64_t)(int32_t)uop.imm_packed); break;
            case 30: r.result=pc+4; r.mispredict=true; r.redirect_pc=(rs1+(int64_t)(int32_t)uop.imm_packed)&~1ULL; break;
            case 31: r.mispredict=(rs1==rs2); r.redirect_pc=pc+(int64_t)(int32_t)uop.imm_packed; break;
            case 32: r.mispredict=(rs1!=rs2); r.redirect_pc=pc+(int64_t)(int32_t)uop.imm_packed; break;
            case 33: r.mispredict=((int64_t)rs1<(int64_t)rs2); r.redirect_pc=pc+(int64_t)(int32_t)uop.imm_packed; break;
            case 34: r.mispredict=((int64_t)rs1>=(int64_t)rs2); r.redirect_pc=pc+(int64_t)(int32_t)uop.imm_packed; break;
            case 35: r.mispredict=(rs1<rs2); r.redirect_pc=pc+(int64_t)(int32_t)uop.imm_packed; break;
            case 36: r.mispredict=(rs1>=rs2); r.redirect_pc=pc+(int64_t)(int32_t)uop.imm_packed; break;
            case 37: r.result=uop.imm_packed; break;
            case 38: r.result=pc+uop.imm_packed; break;
            case 39: case 40: case 41: case 42: case 43: case 44: case 45:
                r.memory_valid=true; r.is_load=true; r.signed_load=uop.mem.mem_signed;
                r.memory_size=uop.mem.mem_size; r.memory_address=rs1+(int64_t)(int32_t)uop.imm_packed;
                r.memory_mask=mask_for_size(uop.mem.mem_size); r.result=r.memory_address; break;
            case 46: case 47: case 48: case 49:
                r.memory_valid=true; r.is_store=true; r.memory_size=uop.mem.mem_size;
                r.memory_address=rs1+(int64_t)(int32_t)uop.imm_packed; r.store_data=rs2;
                r.memory_mask=mask_for_size(uop.mem.mem_size); r.result=r.memory_address; break;
            case 67: case 68: case 69: case 70: case 71: case 72: r.result=0; break;
            default: r.result=0; break;
        }

        if (uop.exception) { r.exception=true; r.exc_cause=uop.exc_cause; }

    }
}

}
