#include "boom_state.hpp"
#include "completion.hpp"

#include <cstdint>
#include <cstdio>

namespace boom { void execute_module(BoomCoreState& state); }

static bool jalr_link(bool is_rvc, uint64_t expected) {
    BoomCoreState state;
    MicroOp& uop = state.issue.issued_uops[INT_ISSUE_LANE];
    state.issue.issued_valids[INT_ISSUE_LANE] = true;
    uop.uopc = 30;
    uop.iq_type = IQ_ALU;
    uop.fu_code = FU_ALU;
    uop.ctrl.op1_sel = OP1_RS1;
    uop.ctrl.op2_sel = OP2_IMM;
    uop.rename.dst_rtype = DST_INT;
    uop.debug_pc = 0x80000102ULL;
    uop.is_rvc = is_rvc;
    boom::execute_module(state);
    const ExecuteState::AluResult& result = state.execute.alu_results[INT_ISSUE_LANE];
    return result.valid && result.result == expected && result.mispredict;
}

int main() {
    const bool compressed = jalr_link(true, 0x80000104ULL);
    const bool base = jalr_link(false, 0x80000106ULL);
    std::printf("RVC_JALR_LINK compressed=%s base=%s\n",
                compressed ? "PASS" : "FAIL", base ? "PASS" : "FAIL");
    return compressed && base ? 0 : 1;
}
