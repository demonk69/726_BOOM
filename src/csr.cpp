#include "boom_config.hpp"
#include "boom_types.hpp"
#include "boom_state.hpp"

namespace boom {

void csr_module(BoomCoreState& state) {
    state.csr.cycle = state.csr.cycle + 1;
}

}
