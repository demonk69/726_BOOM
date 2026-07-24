#include "boom_config.hpp"
#include "boom_types.hpp"
#include "boom_state.hpp"

namespace boom {

void branch_module(BoomCoreState& state) {
    for (int i=0; i<DISPATCH_WIDTH; i++) {
        if (!state.execute.alu_results[i].valid) continue;
        const ExecuteState::AluResult& r = state.execute.alu_results[i];
        if (r.mispredict && (r.uop.branch.is_br || r.uop.branch.is_jal || r.uop.branch.is_jalr)) {
            state.brupdate.valid = true;
            state.brupdate.mispredict = true;
            state.brupdate.jalr_target = r.redirect_pc;
            state.brupdate.mispredict_mask = 1;
        }
    }
}

}
