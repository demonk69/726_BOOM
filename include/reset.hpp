#ifndef BOOM_RESET_HPP
#define BOOM_RESET_HPP

#include "boom_state.hpp"
#include <cstdint>

enum ResetPhase : uint8_t {
    RESET_CONTROL = 0,
    RESET_FRONTEND,
    RESET_RENAME_MAP,
    RESET_FREE_BUSY,
    RESET_ROB,
    RESET_IQ,
    RESET_BRANCH,
    RESET_EXECUTE,
    RESET_LSU,
    RESET_CSR,
    RESET_OUTPUTS,
    RESET_DONE
};

struct ResetControllerState {
    uint8_t phase;
    uint8_t index;
    bool completed;

    ResetControllerState() : phase(RESET_CONTROL), index(0), completed(false) {}
};

void boom_core_reset_step(BoomCoreState& state, ResetControllerState& reset_ctrl);

#endif
