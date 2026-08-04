#ifndef COMPLETION_HPP
#define COMPLETION_HPP

#include "boom_state.hpp"
#include "boom_interfaces.hpp"

namespace boom {
void completion_from_execute(const ExecuteState::AluResult& result,
                             CompletionSourceId source,
                             CompletionEvent& event);
void completion_from_load_response(const BoomCoreState& state,
                                   const DmemResponse& response,
                                   CompletionEvent& event);
bool completion_has_rob_owner(const BoomCoreState& state,
                              const CompletionEvent& event);
bool completion_is_valid(const BoomCoreState& state,
                         const CompletionEvent& event);
CompletionSourceId select_completion_serial(const BoomCoreState& state,
                                            const CompletionEvent& mem_event,
                                            const CompletionEvent& int_event,
                                            bool& selected_valid);
bool apply_completion(BoomCoreState& state, const CompletionEvent& event);
void build_rob_complete_ports(const BoomCoreState& state,
                               RobCompleteEvent ports[NUM_ROB_COMPLETE_PORTS]);
void build_wakeup_bypass_ports(BoomCoreState& state);
bool wakeup_lookup(const BoomCoreState& state, uint8_t prs, uint64_t& value,
                   bool& conflict);
bool bypass_lookup(const BoomCoreState& state, uint8_t prs, uint64_t& value,
                   bool& conflict);
void completion_service_execute(BoomCoreState& state);
void completion_service_cycle(BoomCoreState& state, PipeSignals& pipe);
}

#endif
