#include "completion.hpp"

namespace boom {

extern void branch_complete_event(BoomCoreState& state, const MicroOp& uop,
                                  bool mispredict, uint64_t redirect_pc);
extern bool lsu_accept_completion(BoomCoreState& state, const MicroOp& uop,
                                  bool is_load, bool is_store, bool signed_load,
                                  uint64_t memory_address, uint64_t store_data,
                                  uint8_t memory_mask, uint8_t memory_size);
extern bool lsu_finish_load_response(BoomCoreState& state, uint8_t rob_idx,
                                     uint32_t allocation_id,
                                     uint32_t transaction_id);

static bool is_branch(const MicroOp& uop) {
    return uop.branch.is_br || uop.branch.is_jal || uop.branch.is_jalr;
}

static uint64_t sign_extend(uint64_t value, uint8_t bits) {
    if (bits >= 64) return value;
    uint64_t sign = 1ULL << (bits - 1);
    return (value ^ sign) - sign;
}

static uint64_t load_value(uint64_t data, uint64_t address, uint8_t size,
                           bool signed_load) {
    uint8_t bytes = (uint8_t)(1u << (size & 0x3));
    uint8_t bits = (uint8_t)(bytes * 8);
    uint8_t shift = (uint8_t)((address & 0x7) * 8);
    uint64_t mask = bits >= 64 ? ~0ULL : ((1ULL << bits) - 1ULL);
    uint64_t value = (data >> shift) & mask;
    return signed_load ? sign_extend(value, bits) : value;
}

void completion_from_execute(const ExecuteState::AluResult& result,
                             CompletionSourceId source,
                             CompletionEvent& event) {
#pragma HLS INLINE
    event.valid = result.valid;
    event.source = source;
    event.kind = is_branch(result.uop) ? COMPLETION_BRANCH :
        (result.is_store ? COMPLETION_STORE :
         ((result.memory_valid || result.is_load) ? COMPLETION_MEMORY_ADDRESS :
          COMPLETION_EXECUTE));
    event.uop = result.uop;
    event.writes_prf = event.kind != COMPLETION_MEMORY_ADDRESS &&
                       event.kind != COMPLETION_STORE &&
                       result.uop.rename.dst_rtype == DST_INT &&
                       result.uop.rename.pdst != 0;
    event.control_resolved = false;
    event.mispredict = result.mispredict;
    event.redirect_pc = result.redirect_pc;
    event.value = result.result;
    event.exception = result.exception;
    event.exc_cause = result.exc_cause;
    event.memory_valid = result.memory_valid;
    event.is_load = result.is_load;
    event.is_store = result.is_store;
    event.signed_load = result.signed_load;
    event.memory_address = result.memory_address;
    event.store_data = result.store_data;
    event.memory_mask = result.memory_mask;
    event.memory_size = result.memory_size;
    event.transaction_id = 0;
}

void completion_from_load_response(const BoomCoreState& state,
                                   const DmemResponse& response,
                                   CompletionEvent& event) {
#pragma HLS INLINE
    event.valid = false;
    event.kind = COMPLETION_LOAD_RESPONSE;
    event.source = COMPLETION_SOURCE_LSU_LOAD;
    event.transaction_id = response.transaction_id;
    event.writes_prf = false;
    if (!state.lsu.load_response_pending ||
        response.transaction_id != state.lsu.pending_load_transaction_id ||
        state.lsu.pending_load_rob_idx >= ROB_DEPTH) return;

    const RobEntry& entry = state.rob.entries[state.lsu.pending_load_rob_idx];
    if (!entry.valid || !entry.busy || !entry.is_load ||
        !entry.memory_request_sent || entry.memory_completed ||
        entry.uop.queue.rob_allocation_id != state.lsu.pending_load_allocation_id ||
        entry.memory_transaction_id != response.transaction_id) return;
    bool owns_ldq = false;
LOAD_RESPONSE_LDQ_SCAN:
    for (int i = 0; i < LDQ_DEPTH; i++)
        if (state.lsu.ldq[i].valid &&
            state.lsu.ldq[i].rob_idx == state.lsu.pending_load_rob_idx &&
            state.lsu.ldq[i].rob_allocation_id == state.lsu.pending_load_allocation_id)
            owns_ldq = true;
    if (!owns_ldq) return;

    event.valid = true;
    event.uop = entry.uop;
    event.mispredict = false;
    event.redirect_pc = 0;
    event.exception = response.exception;
    event.exc_cause = response.exception_cause ? response.exception_cause : response.exc_cause;
    event.memory_valid = true;
    event.is_load = true;
    event.is_store = false;
    event.signed_load = entry.signed_load;
    event.memory_address = entry.memory_address;
    event.store_data = 0;
    event.memory_size = entry.memory_size;
    event.memory_mask = entry.memory_mask;
    uint64_t data = response.read_data ? response.read_data : response.data;
    event.value = load_value(data, entry.memory_address, entry.memory_size,
                             entry.signed_load);
    event.writes_prf = !event.exception && entry.uop.rename.dst_rtype == DST_INT &&
                       entry.uop.rename.pdst != 0;
}

bool completion_has_rob_owner(const BoomCoreState& state,
                              const CompletionEvent& event) {
    uint8_t index = event.uop.queue.rob_idx;
    return index < ROB_DEPTH && state.rob.entries[index].valid &&
        state.rob.entries[index].uop.queue.rob_allocation_id ==
            event.uop.queue.rob_allocation_id;
}

bool completion_is_valid(const BoomCoreState& state,
                         const CompletionEvent& event) {
    if (!event.valid || event.kind == COMPLETION_NONE ||
        event.source >= COMPLETION_SOURCE_COUNT ||
        !completion_has_rob_owner(state, event)) return false;

    const RobEntry& entry = state.rob.entries[event.uop.queue.rob_idx];
    if (!entry.busy) return false;
    if (event.kind == COMPLETION_MEMORY_ADDRESS)
        return !entry.memory_valid && !entry.memory_completed;
    if (event.kind == COMPLETION_STORE)
        return !entry.memory_completed;
    if (event.kind == COMPLETION_LOAD_RESPONSE)
        return entry.is_load && entry.memory_request_sent &&
            !entry.memory_completed &&
            entry.memory_transaction_id == event.transaction_id;
    return true;
}

static uint8_t completion_age(const BoomCoreState& state,
                              const CompletionEvent& event) {
    return (uint8_t)((event.uop.queue.rob_idx + ROB_DEPTH - state.rob.head) % ROB_DEPTH);
}

CompletionSourceId select_completion_serial(const BoomCoreState& state,
                                            const CompletionEvent& mem_event,
                                            const CompletionEvent& int_event,
                                            bool& selected_valid) {
#pragma HLS INLINE
    bool mem_valid = completion_is_valid(state, mem_event);
    bool int_valid = completion_is_valid(state, int_event);
    selected_valid = mem_valid || int_valid;
    if (!mem_valid) return COMPLETION_SOURCE_INT_EXECUTE;
    if (!int_valid) return COMPLETION_SOURCE_MEM_EXECUTE;
    uint8_t mem_age = completion_age(state, mem_event);
    uint8_t int_age = completion_age(state, int_event);
    return mem_age <= int_age ? COMPLETION_SOURCE_MEM_EXECUTE :
                                COMPLETION_SOURCE_INT_EXECUTE;
}

static bool completion_writes_integer(const CompletionEvent& event) {
#pragma HLS INLINE
    bool completes_rob = event.kind != COMPLETION_MEMORY_ADDRESS || !event.is_load;
    return completes_rob && event.writes_prf &&
        !(event.kind == COMPLETION_LOAD_RESPONSE && event.exception);
}

static bool apply_completion_selected(BoomCoreState& state,
                                      const CompletionEvent& event,
                                      bool writer_selected,
                                      bool physical_write) {
#pragma HLS INLINE
    if (!completion_is_valid(state, event)) return false;
    bool completes_rob = event.kind != COMPLETION_MEMORY_ADDRESS || !event.is_load;
    bool writes_integer = completion_writes_integer(event);
    if (writes_integer && !writer_selected) return false;
    uint8_t index = event.uop.queue.rob_idx;

    if (event.kind == COMPLETION_MEMORY_ADDRESS || event.kind == COMPLETION_STORE)
        if (!lsu_accept_completion(state, event.uop, event.is_load,
                                   event.is_store, event.signed_load,
                                   event.memory_address, event.store_data,
                                   event.memory_mask, event.memory_size)) return false;

    RobEntry& entry = state.rob.entries[index];
    if (completes_rob) entry.busy = false;
    if (event.exception) {
        entry.exception = true;
        entry.uop.exception = true;
        entry.uop.exc_cause = event.exc_cause;
    }

    if (writes_integer && physical_write) {
        prf_seed(state, event.uop.rename.pdst, event.value);
    }
    if (writes_integer) {
        state.rename.int_free_list.busy_table[event.uop.rename.pdst] = false;
    }
    if (event.kind == COMPLETION_LOAD_RESPONSE) {
        entry.memory_completed = true;
        if (!event.exception) entry.memory_data = event.value;
    }
    return true;
}

bool apply_completion(BoomCoreState& state, const CompletionEvent& event) {
#ifndef __SYNTHESIS__
    CompletionEvent resolved = event;
    if (event.kind == COMPLETION_BRANCH && !event.control_resolved) {
        branch_complete_event(state, event.uop, event.mispredict,
                              event.redirect_pc);
        if (!completion_has_rob_owner(state, event)) return true;
        resolved.control_resolved = true;
    }
    return apply_completion_selected(state, resolved, true, true);
#else
    return apply_completion_selected(state, event, true, true);
#endif
}

void build_rob_complete_ports(const BoomCoreState& state,
                              RobCompleteEvent ports[NUM_ROB_COMPLETE_PORTS]) {
#pragma HLS INLINE
    // Fixed HLS completion interface: load response, MEM execute/LSU sideband,
    // and INT execute. The fourth generated response class is FP-only here.
    ports[ROB_COMPLETE_PORT_LSU_LOAD] = state.completion.load_response;
    ports[ROB_COMPLETE_PORT_MEM_EXECUTE] = state.completion.mem_execute;
    ports[ROB_COMPLETE_PORT_INT_EXECUTE] = state.completion.int_execute;
    ports[ROB_COMPLETE_PORT_UNSUPPORTED] = RobCompleteEvent();
}

static bool result_can_forward(const BoomCoreState& state,
                               const RobCompleteEvent& event) {
#pragma HLS INLINE
    return completion_is_valid(state, event) && event.writes_prf &&
        !event.exception && event.uop.rename.pdst != 0 &&
        event.uop.rename.pdst < INT_PHYS_REGS;
}

static bool oldest_precise_exception(
        const BoomCoreState& state,
        const RobCompleteEvent ports[NUM_ROB_COMPLETE_PORTS],
        uint8_t& exception_age) {
#pragma HLS INLINE
    bool valid = false;
    exception_age = ROB_DEPTH;
    for (int i=0; i<COMPLETION_PENDING_SLOTS; i++) {
        if (!completion_is_valid(state, ports[i]) || !ports[i].exception)
            continue;
        uint8_t age = completion_age(state, ports[i]);
        if (!valid || age < exception_age) {
            valid = true;
            exception_age = age;
        }
    }
    for (int i=0; i<ROB_DEPTH; i++) {
        const RobEntry& entry = state.rob.entries[i];
        if (!entry.valid || !entry.exception ||
            entry.uop.queue.rob_idx != i) continue;
        uint8_t age = (uint8_t)((i + ROB_DEPTH - state.rob.head) % ROB_DEPTH);
        if (!valid || age < exception_age) {
            valid = true;
            exception_age = age;
        }
    }
    return valid;
}

static void clear_pending_pdst(BoomCoreState& state, uint8_t pdst) {
#pragma HLS INLINE
    if (state.completion.load_response.valid &&
        state.completion.load_response.uop.rename.pdst == pdst)
        state.completion.load_response = RobCompleteEvent();
    if (state.completion.mem_execute.valid &&
        state.completion.mem_execute.uop.rename.pdst == pdst)
        state.completion.mem_execute = RobCompleteEvent();
    if (state.completion.int_execute.valid &&
        state.completion.int_execute.uop.rename.pdst == pdst)
        state.completion.int_execute = RobCompleteEvent();
}

static void raise_writeback_validation_fault(
        BoomCoreState& state,
        const RobCompleteEvent ports[NUM_ROB_COMPLETE_PORTS], uint8_t pdst) {
#pragma HLS INLINE
    int fault_port = -1;
    uint8_t fault_age = ROB_DEPTH;
    uint8_t participants = 0;
    for (int i=0; i<COMPLETION_PENDING_SLOTS; i++) {
        if (!completion_is_valid(state, ports[i]) ||
            !completion_writes_integer(ports[i]) ||
            ports[i].uop.rename.pdst != pdst) continue;
        participants++;
        uint8_t age = completion_age(state, ports[i]);
        if (fault_port < 0 || age < fault_age ||
            (age == fault_age && i < fault_port)) {
            fault_port = i;
            fault_age = age;
        }
    }
    if (fault_port < 0) return;

    for (int i=0; i<COMPLETION_PENDING_SLOTS; i++) {
        if (!completion_is_valid(state, ports[i]) ||
            !completion_writes_integer(ports[i]) ||
            ports[i].uop.rename.pdst != pdst) continue;
        RobEntry& entry = state.rob.entries[ports[i].uop.queue.rob_idx];
        entry.busy = false;
        entry.exception = true;
        entry.uop.exception = true;
        entry.uop.exc_cause = WRITEBACK_VALIDATION_FAULT_CAUSE;
    }
    const RobCompleteEvent& fault = ports[fault_port];
    state.completion.writeback_conflict = true;
    state.completion.writeback_fault_valid = true;
    state.completion.writeback_fault_pdst = pdst;
    state.completion.writeback_fault_rob_idx = fault.uop.queue.rob_idx;
    state.completion.writeback_fault_allocation_id =
        fault.uop.queue.rob_allocation_id;
    state.completion.writeback_fault_cause = WRITEBACK_VALIDATION_FAULT_CAUSE;
    state.completion.validation_fault_this_cycle = true;
    state.completion.writeback_conflicts++;
    state.completion.writeback_validation_faults++;
    state.completion.writeback_fault_events += participants;
    state.completion.wakeup_conflicts++;
    state.completion.bypass_conflicts++;
    clear_pending_pdst(state, pdst);
}

void build_wakeup_bypass_ports(BoomCoreState& state) {
#pragma HLS INLINE
    RobCompleteEvent ports[NUM_ROB_COMPLETE_PORTS];
    build_rob_complete_ports(state, ports);
    for (int i=0; i<NUM_INT_WAKEUP_PORTS; i++)
        state.completion.wakeups[i] = WakeupEvent();
    for (int i=0; i<NUM_INT_BYPASS_PORTS; i++)
        state.completion.bypass[i] = BypassEvent();
    if (state.completion.validation_fault_this_cycle) {
        state.completion.wakeups_this_cycle = 0;
        state.completion.bypass_this_cycle = 0;
        return;
    }

    bool usable[COMPLETION_PENDING_SLOTS];
    for (int i=0; i<COMPLETION_PENDING_SLOTS; i++) {
        usable[i] = result_can_forward(state, ports[i]);
        if (!usable[i]) state.completion.wakeup_sent[i] = false;
    }
    uint8_t exception_age = ROB_DEPTH;
    bool exception_fence = oldest_precise_exception(state, ports, exception_age);
    if (exception_fence)
        for (int i=0; i<COMPLETION_PENDING_SLOTS; i++)
            if (usable[i] && completion_age(state, ports[i]) > exception_age)
                usable[i] = false;
    bool precise_valid = false;
    uint8_t precise_age = ROB_DEPTH;
    for (int i=0; i<COMPLETION_PENDING_SLOTS; i++) {
        if (!completion_is_valid(state, ports[i])) continue;
        bool precise = ports[i].kind == COMPLETION_BRANCH;
        uint8_t age = completion_age(state, ports[i]);
        if (precise && (!precise_valid || age < precise_age)) {
            precise_valid = true;
            precise_age = age;
        }
    }
    if (precise_valid)
        for (int i=0; i<COMPLETION_PENDING_SLOTS; i++)
            if (usable[i] && completion_age(state, ports[i]) > precise_age)
                usable[i] = false;
    bool conflict_seen = false;
    uint8_t conflict_pdst = 0;
    for (int i=0; i<COMPLETION_PENDING_SLOTS; i++) {
        if (!usable[i]) continue;
        for (int j=i+1; j<COMPLETION_PENDING_SLOTS; j++) {
            if (!usable[j] || ports[i].uop.rename.pdst != ports[j].uop.rename.pdst)
                continue;
            if (ports[i].value != ports[j].value) {
                usable[i] = false;
                usable[j] = false;
                conflict_seen = true;
                conflict_pdst = ports[i].uop.rename.pdst;
            } else {
                usable[j] = false;
                state.completion.wakeup_sent[j] = true;
                state.completion.writeback_deduplications++;
            }
        }
    }
    if (conflict_seen) {
        raise_writeback_validation_fault(state, ports, conflict_pdst);
        state.completion.wakeups_this_cycle = 0;
        state.completion.bypass_this_cycle = 0;
        return;
    }

    uint8_t output = 0;
    for (int i=0; i<COMPLETION_PENDING_SLOTS; i++) {
        if (!usable[i] || output >= NUM_INT_WAKEUP_PORTS) continue;
        const RobCompleteEvent& event = ports[i];
        WakeupEvent& wakeup = state.completion.wakeups[output];
        wakeup.valid = true;
        wakeup.pdst = event.uop.rename.pdst;
        wakeup.value = event.value;
        wakeup.rob_idx = event.uop.queue.rob_idx;
        wakeup.rob_allocation_id = event.uop.queue.rob_allocation_id;
        wakeup.branch_mask = event.uop.branch.br_mask;
        wakeup.source = event.source;
        BypassEvent& bypass = state.completion.bypass[output];
        bypass.valid = true;
        bypass.pdst = wakeup.pdst;
        bypass.value = wakeup.value;
        bypass.rob_idx = wakeup.rob_idx;
        bypass.rob_allocation_id = wakeup.rob_allocation_id;
        bypass.branch_mask = wakeup.branch_mask;
        bypass.source = wakeup.source;
        if (!state.completion.wakeup_sent[i]) {
            state.completion.total_wakeups++;
            state.completion.total_bypass++;
            state.completion.wakeup_sent[i] = true;
        }
        output++;
    }
    state.completion.wakeups_this_cycle = output;
    state.completion.bypass_this_cycle = output;
    if (output > state.completion.peak_wakeups)
        state.completion.peak_wakeups = output;
    if (output > state.completion.peak_bypass)
        state.completion.peak_bypass = output;
}

bool wakeup_lookup(const BoomCoreState& state, uint8_t prs, uint64_t& value,
                   bool& conflict) {
#pragma HLS INLINE
    conflict = false;
    if (prs == 0) { value = 0; return true; }
    bool hit = false;
    for (int i=0; i<NUM_INT_WAKEUP_PORTS; i++) {
        const WakeupEvent& event = state.completion.wakeups[i];
        if (!event.valid || event.pdst != prs) continue;
        if (hit && value != event.value) conflict = true;
        if (!hit) value = event.value;
        hit = true;
    }
    return hit && !conflict;
}

bool bypass_lookup(const BoomCoreState& state, uint8_t prs, uint64_t& value,
                   bool& conflict) {
#pragma HLS INLINE
    conflict = false;
    if (prs == 0) { value = 0; return true; }
    bool hit = false;
    for (int i=0; i<NUM_INT_BYPASS_PORTS; i++) {
        const BypassEvent& event = state.completion.bypass[i];
        if (!event.valid || event.pdst != prs) continue;
        if (hit && value != event.value) conflict = true;
        if (!hit) value = event.value;
        hit = true;
    }
    return hit && !conflict;
}

static void capture_execute_events(BoomCoreState& state) {
#pragma HLS INLINE
    if (state.completion.mem_execute.valid &&
        state.execute.alu_results[MEM_ISSUE_LANE].valid &&
        state.completion.mem_execute.uop.queue.rob_idx ==
            state.execute.alu_results[MEM_ISSUE_LANE].uop.queue.rob_idx &&
        state.completion.mem_execute.uop.queue.rob_allocation_id ==
            state.execute.alu_results[MEM_ISSUE_LANE].uop.queue.rob_allocation_id) {
        state.completion.dropped_completions++;
        state.execute.alu_results[MEM_ISSUE_LANE] = ExecuteState::AluResult();
    }
    if (!state.completion.mem_execute.valid &&
        state.execute.alu_results[MEM_ISSUE_LANE].valid) {
        completion_from_execute(state.execute.alu_results[MEM_ISSUE_LANE],
                                COMPLETION_SOURCE_MEM_EXECUTE,
                                state.completion.mem_execute);
        state.execute.alu_results[MEM_ISSUE_LANE] = ExecuteState::AluResult();
    }
    if (state.completion.int_execute.valid &&
        state.execute.alu_results[INT_ISSUE_LANE].valid &&
        state.completion.int_execute.uop.queue.rob_idx ==
            state.execute.alu_results[INT_ISSUE_LANE].uop.queue.rob_idx &&
        state.completion.int_execute.uop.queue.rob_allocation_id ==
            state.execute.alu_results[INT_ISSUE_LANE].uop.queue.rob_allocation_id) {
        state.completion.dropped_completions++;
        state.execute.alu_results[INT_ISSUE_LANE] = ExecuteState::AluResult();
    }
    if (!state.completion.int_execute.valid &&
        state.execute.alu_results[INT_ISSUE_LANE].valid) {
        completion_from_execute(state.execute.alu_results[INT_ISSUE_LANE],
                                COMPLETION_SOURCE_INT_EXECUTE,
                                state.completion.int_execute);
        state.execute.alu_results[INT_ISSUE_LANE] = ExecuteState::AluResult();
    }
    // Lane 2 is not a canonical integer completion source.
    state.execute.alu_results[FP_ISSUE_LANE] = ExecuteState::AluResult();
}

static void discard_invalid_pending(BoomCoreState& state) {
#pragma HLS INLINE
    if (state.completion.load_response.valid &&
        !completion_is_valid(state, state.completion.load_response)) {
        if (completion_has_rob_owner(state, state.completion.load_response) &&
            !state.rob.entries[state.completion.load_response.uop.queue.rob_idx].busy)
            state.completion.dropped_completions++;
        state.completion.load_response = RobCompleteEvent();
    }
    if (state.completion.mem_execute.valid &&
        !completion_is_valid(state, state.completion.mem_execute)) {
        if (completion_has_rob_owner(state, state.completion.mem_execute) &&
            !state.rob.entries[state.completion.mem_execute.uop.queue.rob_idx].busy)
            state.completion.dropped_completions++;
        state.completion.mem_execute = RobCompleteEvent();
    }
    if (state.completion.int_execute.valid &&
        !completion_is_valid(state, state.completion.int_execute)) {
        if (completion_has_rob_owner(state, state.completion.int_execute) &&
            !state.rob.entries[state.completion.int_execute.uop.queue.rob_idx].busy)
            state.completion.dropped_completions++;
        state.completion.int_execute = RobCompleteEvent();
    }
}

static void mark_control_resolved(BoomCoreState& state,
                                  CompletionSourceId source) {
#pragma HLS INLINE
    if (source == COMPLETION_SOURCE_LSU_LOAD &&
        state.completion.load_response.valid)
        state.completion.load_response.control_resolved = true;
    else if (source == COMPLETION_SOURCE_MEM_EXECUTE &&
             state.completion.mem_execute.valid)
        state.completion.mem_execute.control_resolved = true;
    else if (source == COMPLETION_SOURCE_INT_EXECUTE &&
             state.completion.int_execute.valid)
        state.completion.int_execute.control_resolved = true;
}

static void resolve_oldest_pending_branch(BoomCoreState& state) {
#pragma HLS INLINE
    RobCompleteEvent ports[NUM_ROB_COMPLETE_PORTS];
    build_rob_complete_ports(state, ports);
    int selected = -1;
    uint8_t selected_age = ROB_DEPTH;
    for (int i=0; i<COMPLETION_PENDING_SLOTS; i++) {
        if (!completion_is_valid(state, ports[i]) ||
            ports[i].kind != COMPLETION_BRANCH || ports[i].control_resolved)
            continue;
        uint8_t age = completion_age(state, ports[i]);
        if (selected < 0 || age < selected_age ||
            (age == selected_age && i < selected)) {
            selected = i;
            selected_age = age;
        }
    }
    if (selected < 0) return;
    branch_complete_event(state, ports[selected].uop, ports[selected].mispredict,
                          ports[selected].redirect_pc);
    mark_control_resolved(state, ports[selected].source);
}

static void validate_surviving_writers(BoomCoreState& state) {
#pragma HLS INLINE
    RobCompleteEvent ports[NUM_ROB_COMPLETE_PORTS];
    build_rob_complete_ports(state, ports);
    for (int i=0; i<COMPLETION_PENDING_SLOTS; i++) {
        if (!result_can_forward(state, ports[i])) continue;
        for (int j=i+1; j<COMPLETION_PENDING_SLOTS; j++) {
            if (!result_can_forward(state, ports[j]) ||
                ports[i].uop.rename.pdst != ports[j].uop.rename.pdst ||
                ports[i].value == ports[j].value) continue;
            raise_writeback_validation_fault(state, ports,
                                             ports[i].uop.rename.pdst);
            return;
        }
    }
}

static bool write_conflicts(const BoomCoreState& state,
                            const RobCompleteEvent ports[NUM_ROB_COMPLETE_PORTS],
                            int candidate) {
#pragma HLS INLINE
    if (!completion_writes_integer(ports[candidate])) return false;
    for (int i=0; i<COMPLETION_PENDING_SLOTS; i++) {
        if (i == candidate || !completion_is_valid(state, ports[i]) ||
            !completion_writes_integer(ports[i])) continue;
        if (ports[i].uop.rename.pdst == ports[candidate].uop.rename.pdst &&
            ports[i].value != ports[candidate].value) return true;
    }
    return false;
}

static void apply_writeback_ports(BoomCoreState& state) {
#pragma HLS PIPELINE II=1
    bool duplicate = state.completion.writebacks[0].valid &&
        state.completion.writebacks[1].valid &&
        state.completion.writebacks[0].pdst == state.completion.writebacks[1].pdst;
    if (duplicate) {
        state.completion.duplicate_writebacks++;
        state.completion.dropped_writebacks += 2;
    }
    uint64_t latest = state.int_rf_latest_bank;
    if (state.completion.writebacks[0].valid && !duplicate) {
        uint8_t pdst = state.completion.writebacks[0].pdst;
        state.int_rf_bank0[pdst] = state.completion.writebacks[0].value;
        latest &= ~(1ULL << pdst);
    }
    if (state.completion.writebacks[1].valid && !duplicate) {
        uint8_t pdst = state.completion.writebacks[1].pdst;
        state.int_rf_bank1[pdst] = state.completion.writebacks[1].value;
        latest |= 1ULL << pdst;
    }
    state.int_rf_latest_bank = latest;
}

static void service_pending(BoomCoreState& state) {
    uint8_t write_count = 0;
    bool exception_fence = false;
    uint8_t exception_age = ROB_DEPTH;
    for (int i=0; i<NUM_INT_WRITEBACK_PORTS; i++)
        state.completion.writebacks[i] = WritebackEvent();
    if (state.completion.validation_fault_this_cycle) return;
    {
        RobCompleteEvent initial[NUM_ROB_COMPLETE_PORTS];
        build_rob_complete_ports(state, initial);
        exception_fence = oldest_precise_exception(state, initial,
                                                     exception_age);
    }
SERVICE_FIXED_PENDING:
    for (int service = 0; service < COMPLETION_PENDING_SLOTS; service++) {
        discard_invalid_pending(state);
        RobCompleteEvent ports[NUM_ROB_COMPLETE_PORTS];
        build_rob_complete_ports(state, ports);
        int selected_port = -1;
        uint8_t selected_age = ROB_DEPTH;
        bool precise_valid = false;
        uint8_t precise_age = ROB_DEPTH;
        for (int i=0; i<COMPLETION_PENDING_SLOTS; i++) {
            if (!completion_is_valid(state, ports[i])) continue;
            uint8_t age = completion_age(state, ports[i]);
            if (ports[i].kind == COMPLETION_BRANCH &&
                (!precise_valid || age < precise_age)) {
                precise_valid = true;
                precise_age = age;
            }
        }
        bool selected_duplicate = false;
        for (int i=0; i<COMPLETION_PENDING_SLOTS; i++) {
            if (!completion_is_valid(state, ports[i])) continue;
            uint8_t age = completion_age(state, ports[i]);
            if (exception_fence && age > exception_age) continue;
            if (precise_valid && age > precise_age) continue;
            bool duplicate = false;
            bool write_eligible = true;
            bool candidate_writes = completion_writes_integer(ports[i]);
            if (candidate_writes) {
                if (ports[i].uop.rename.pdst == 0 || write_conflicts(state, ports, i))
                    write_eligible = false;
                for (int w=0; w<NUM_INT_WRITEBACK_PORTS; w++)
                    if (state.completion.writebacks[w].valid &&
                        state.completion.writebacks[w].pdst == ports[i].uop.rename.pdst &&
                        state.completion.writebacks[w].value == ports[i].value)
                        duplicate = true;
                if (!duplicate && write_count >= NUM_INT_WRITEBACK_PORTS)
                    write_eligible = false;
            }
            if (!write_eligible) continue;
            if (selected_port < 0 || age < selected_age ||
                (age == selected_age && i < selected_port)) {
                selected_port = i;
                selected_age = age;
                selected_duplicate = duplicate;
            }
        }
        if (selected_port < 0) break;
        CompletionSourceId selected = ports[selected_port].source;
        bool accepted = false;
        bool writes_prf = completion_writes_integer(ports[selected_port]);
        bool completes_rob = false;
        if (selected == COMPLETION_SOURCE_LSU_LOAD) {
            completes_rob = true;
            uint8_t rob_idx = ports[0].uop.queue.rob_idx;
            uint32_t allocation = ports[0].uop.queue.rob_allocation_id;
            uint32_t transaction = ports[0].transaction_id;
            accepted = apply_completion_selected(state, ports[0], true, false);
            if (accepted) {
                lsu_finish_load_response(state, rob_idx, allocation, transaction);
                state.completion.load_response = RobCompleteEvent();
                state.completion.wakeup_sent[ROB_COMPLETE_PORT_LSU_LOAD] = false;
            }
        } else if (selected == COMPLETION_SOURCE_MEM_EXECUTE) {
            completes_rob = ports[1].kind != COMPLETION_MEMORY_ADDRESS ||
                            !ports[1].is_load;
            accepted = apply_completion_selected(state, ports[1], true, false);
            if (accepted) {
                state.completion.mem_execute = RobCompleteEvent();
                state.completion.wakeup_sent[ROB_COMPLETE_PORT_MEM_EXECUTE] = false;
            }
        } else {
            completes_rob = ports[2].kind != COMPLETION_MEMORY_ADDRESS ||
                            !ports[2].is_load;
            accepted = apply_completion_selected(state, ports[2], true, false);
            if (accepted) {
                state.completion.int_execute = RobCompleteEvent();
                state.completion.wakeup_sent[ROB_COMPLETE_PORT_INT_EXECUTE] = false;
            }
        }
        if (accepted) {
            state.completion.completion_accepts_this_cycle++;
            state.completion.total_completion_accepts++;
            if (completes_rob) {
                state.completion.rob_completes_this_cycle++;
                state.completion.total_rob_completes++;
            }
            if (writes_prf && !selected_duplicate) {
                if (write_count >= NUM_INT_WRITEBACK_PORTS) {
                    state.completion.dropped_writebacks++;
                } else {
                    WritebackEvent& wb = state.completion.writebacks[write_count];
                    wb.valid = true;
                    wb.pdst = ports[selected_port].uop.rename.pdst;
                    wb.value = ports[selected_port].value;
                    wb.rob_idx = ports[selected_port].uop.queue.rob_idx;
                    wb.rob_allocation_id = ports[selected_port].uop.queue.rob_allocation_id;
                    wb.source = ports[selected_port].source;
                    write_count++;
                    state.completion.prf_writes_this_cycle++;
                    state.completion.total_prf_writes++;
                }
            }
            if (ports[selected_port].exception ||
                ports[selected_port].kind == COMPLETION_BRANCH)
                break;
        }
    }
    apply_writeback_ports(state);
    if (state.completion.completion_accepts_this_cycle > state.completion.peak_completion_accepts)
        state.completion.peak_completion_accepts = state.completion.completion_accepts_this_cycle;
    if (state.completion.rob_completes_this_cycle > state.completion.peak_rob_completes)
        state.completion.peak_rob_completes = state.completion.rob_completes_this_cycle;
    if (state.completion.prf_writes_this_cycle > state.completion.peak_prf_writes)
        state.completion.peak_prf_writes = state.completion.prf_writes_this_cycle;
}

static void begin_completion_cycle(BoomCoreState& state) {
    state.completion.completion_accepts_this_cycle = 0;
    state.completion.rob_completes_this_cycle = 0;
    state.completion.prf_writes_this_cycle = 0;
    state.completion.wakeups_this_cycle = 0;
    state.completion.validation_fault_this_cycle = false;
    capture_execute_events(state);
    resolve_oldest_pending_branch(state);
    validate_surviving_writers(state);
    build_wakeup_bypass_ports(state);
    service_pending(state);
}

#ifdef BOOM_HLS_W4A_COMPLETION_DIAGNOSTIC
static void record_w4a_completion(BoomCoreState& state,
                                  const CompletionEvent& event) {
    bool completes_rob = event.kind != COMPLETION_MEMORY_ADDRESS || !event.is_load;
    if (completes_rob) {
        state.completion.rob_completes_this_cycle++;
        state.completion.total_rob_completes++;
    }
    if (event.writes_prf) {
        state.completion.prf_writes_this_cycle++;
        state.completion.wakeups_this_cycle++;
        state.completion.total_prf_writes++;
        state.completion.total_wakeups++;
    }
    if (state.completion.rob_completes_this_cycle > state.completion.peak_rob_completes)
        state.completion.peak_rob_completes = state.completion.rob_completes_this_cycle;
    if (state.completion.prf_writes_this_cycle > state.completion.peak_prf_writes)
        state.completion.peak_prf_writes = state.completion.prf_writes_this_cycle;
    if (state.completion.wakeups_this_cycle > state.completion.peak_wakeups)
        state.completion.peak_wakeups = state.completion.wakeups_this_cycle;
}

static void completion_service_execute_w4a(BoomCoreState& state) {
    state.completion.rob_completes_this_cycle = 0;
    state.completion.prf_writes_this_cycle = 0;
    state.completion.wakeups_this_cycle = 0;
    CompletionEvent mem_event;
    CompletionEvent int_event;
    completion_from_execute(state.execute.alu_results[MEM_ISSUE_LANE],
                            COMPLETION_SOURCE_MEM_EXECUTE, mem_event);
    completion_from_execute(state.execute.alu_results[INT_ISSUE_LANE],
                            COMPLETION_SOURCE_INT_EXECUTE, int_event);
    if (mem_event.valid && !completion_is_valid(state, mem_event))
        state.execute.alu_results[MEM_ISSUE_LANE] = ExecuteState::AluResult();
    if (int_event.valid && !completion_is_valid(state, int_event))
        state.execute.alu_results[INT_ISSUE_LANE] = ExecuteState::AluResult();
    bool selected_valid = false;
    CompletionSourceId selected = select_completion_serial(
        state, mem_event, int_event, selected_valid);
    if (!selected_valid) return;
    if (selected == COMPLETION_SOURCE_MEM_EXECUTE) {
        if (apply_completion(state, mem_event)) {
            state.execute.alu_results[MEM_ISSUE_LANE] = ExecuteState::AluResult();
            record_w4a_completion(state, mem_event);
        }
    } else if (apply_completion(state, int_event)) {
        state.execute.alu_results[INT_ISSUE_LANE] = ExecuteState::AluResult();
        record_w4a_completion(state, int_event);
    }
}
#endif

void completion_service_execute(BoomCoreState& state) {
#ifdef BOOM_HLS_W4A_COMPLETION_DIAGNOSTIC
    completion_service_execute_w4a(state);
#else
    state.completion.completion_accepts_this_cycle = 0;
    state.completion.rob_completes_this_cycle = 0;
    state.completion.prf_writes_this_cycle = 0;
    state.completion.wakeups_this_cycle = 0;
    begin_completion_cycle(state);
#endif
}

void completion_service_cycle(BoomCoreState& state, PipeSignals& pipe) {
#ifndef BOOM_HLS_W4A_COMPLETION_DIAGNOSTIC
    state.completion.completion_accepts_this_cycle = 0;
    state.completion.rob_completes_this_cycle = 0;
    state.completion.prf_writes_this_cycle = 0;
    state.completion.wakeups_this_cycle = 0;
    state.completion.validation_fault_this_cycle = false;
    if (!state.completion.load_response.valid && !pipe.dmem_resp.empty()) {
        DmemResponse response = pipe.dmem_resp.read();
        completion_from_load_response(state, response,
                                      state.completion.load_response);
    }
    capture_execute_events(state);
    resolve_oldest_pending_branch(state);
    validate_surviving_writers(state);
    build_wakeup_bypass_ports(state);
    service_pending(state);
#else
    if (pipe.dmem_resp.empty()) {
        completion_service_execute_w4a(state);
        return;
    }
    state.completion.rob_completes_this_cycle = 0;
    state.completion.prf_writes_this_cycle = 0;
    state.completion.wakeups_this_cycle = 0;
    DmemResponse response = pipe.dmem_resp.read();
    CompletionEvent event;
    completion_from_load_response(state, response, event);
    if (apply_completion(state, event)) {
        lsu_finish_load_response(state, event.uop.queue.rob_idx,
                                 event.uop.queue.rob_allocation_id,
                                 event.transaction_id);
        record_w4a_completion(state, event);
    }
#endif
}

}
