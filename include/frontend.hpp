#ifndef BOOM_FRONTEND_HPP
#define BOOM_FRONTEND_HPP

#include "boom_interfaces.hpp"
#include "boom_state.hpp"

namespace boom {

void frontend_module(BoomCoreState& state, PipeSignals& pipe);

}  // namespace boom

#endif
