#include <cstddef>
#include <cstdint>
#include <cstdio>

#define private public
#include "predictor.hpp"
#undef private
#include "boom_interfaces.hpp"
#include "boom_state.hpp"

namespace boom {
void rob_commit_module(BoomCoreState&, PipeSignals&);
}

namespace {

struct Errors {
    uint64_t training_eligibility_error;
    uint64_t training_index_error;
    uint64_t training_outcome_error;
    uint64_t counter_transition_error;
    uint64_t valid_bit_error;
    uint64_t duplicate_training_error;
    uint64_t dropped_training_error;
    uint64_t wrong_path_training_error;
    uint64_t squashed_training_error;
    uint64_t exception_training_error;
    uint64_t stale_training_error;
    uint64_t jal_training_error;
    uint64_t jalr_training_error;
    uint64_t request_update_conflict_error;
    uint64_t prediction_after_training_error;
    uint64_t commit_order_error;
    uint64_t reset_error;
    Errors() { clear(); }
    void clear() {
        training_eligibility_error = training_index_error = 0;
        training_outcome_error = counter_transition_error = valid_bit_error = 0;
        duplicate_training_error = dropped_training_error = 0;
        wrong_path_training_error = squashed_training_error = 0;
        exception_training_error = stale_training_error = 0;
        jal_training_error = jalr_training_error = 0;
        request_update_conflict_error = prediction_after_training_error = 0;
        commit_order_error = reset_error = 0;
    }
    uint64_t total() const {
        return training_eligibility_error + training_index_error +
            training_outcome_error + counter_transition_error + valid_bit_error +
            duplicate_training_error + dropped_training_error +
            wrong_path_training_error + squashed_training_error +
            exception_training_error + stale_training_error + jal_training_error +
            jalr_training_error + request_update_conflict_error +
            prediction_after_training_error + commit_order_error + reset_error;
    }
};

uint64_t next_random(uint64_t& state) {
    state ^= state << 13;
    state ^= state >> 7;
    state ^= state << 17;
    return state;
}

uint8_t transition(uint8_t counter, bool taken) {
    return taken ? static_cast<uint8_t>(counter == 3 ? 3 : counter + 1) :
        static_cast<uint8_t>(counter == 0 ? 0 : counter - 1);
}

void exhaustive(Errors& errors, uint64_t& checks) {
    for (uint8_t counter = 0; counter < 4; ++counter)
    for (unsigned valid = 0; valid < 2; ++valid)
    for (unsigned taken = 0; taken < 2; ++taken)
    for (unsigned eligible = 0; eligible < 2; ++eligible)
    for (unsigned stale = 0; stale < 2; ++stale)
    for (unsigned conditional = 0; conditional < 2; ++conditional) {
        boom::PredictorFoundation<256> predictor;
        const uint64_t pc = 0x140;
        const uint16_t index = static_cast<uint16_t>((pc >> 1) & 255u);
        predictor.valid_[index] = valid != 0;
        predictor.counters_[index] = counter;
        boom::PredictorStepInput input;
        input.active_generation = 9;
        input.update.valid = eligible != 0;
        input.update.commit_qualified = eligible != 0;
        input.update.cfi_type = conditional ? boom::CFI_CONDITIONAL_BRANCH :
            boom::CFI_JAL;
        input.update.pc = pc;
        input.update.metadata_token = index;
        input.update.taken = taken != 0;
        input.update.generation = stale ? 8 : 9;
        predictor.step(input);
        const bool train = eligible && !stale && conditional;
        const uint8_t logical_old = valid ? counter : 1;
        const uint8_t expected = train ? transition(logical_old, taken != 0) : counter;
        ++checks;
        if (predictor.valid_[index] != (train || valid)) ++errors.valid_bit_error;
        ++checks;
        if (predictor.counters_[index] != expected) ++errors.counter_transition_error;
    }
}

void campaign(uint64_t seed, uint64_t cycles, Errors& errors,
              uint64_t& updates, uint64_t& checks) {
    BoomCoreState state;
    PipeSignals pipe;
    state.rob.state = ROB_NORMAL;
    state.product_ftq_enabled = true;
    state.predictor_generation = 17;
    state.frontend.reset_done = true;
    bool valid[256] = {};
    uint8_t counters[256] = {};
    uint64_t rng = seed ? seed : 1;
    const uint32_t generation = 17;
    for (uint64_t cycle = 0; cycle < cycles; ++cycle) {
        const uint64_t bits = next_random(rng);
        const uint64_t pc = (bits & 0xfffffu) << 1;
        const uint16_t index = static_cast<uint16_t>((pc >> 1) & 255u);
        const bool actual_taken = ((bits >> 24) & 1u) != 0;
        const bool do_reset = ((bits >> 25) & 255u) == 0;
        const bool architectural_commit = ((bits >> 33) & 7u) != 0;
        const bool squashed = !architectural_commit && ((bits >> 36) & 1u);
        const bool exception = architectural_commit && ((bits >> 37) & 31u) == 0;
        const bool resolved = ((bits >> 42) & 31u) != 0;
        const bool stale_ftq = ((bits >> 47) & 31u) == 0;
        const bool stale_predictor = ((bits >> 52) & 31u) == 0;
        const bool metadata_match = ((bits >> 57) & 15u) != 0;
        const uint8_t kind = static_cast<uint8_t>((bits >> 61) & 3u);
        const bool conditional = kind == 0;

        if (do_reset) {
            boom::PredictorStepInput predictor_reset;
            predictor_reset.reset = true;
            predictor_reset.active_generation = generation;
            state.predictor.step(predictor_reset);
            boom::FtqStepInput ftq_reset;
            ftq_reset.reset = true;
            state.ftq.step(ftq_reset);
            state.rob = RobInternalState();
            state.rob.state = ROB_NORMAL;
            state.predictor_update_pending = boom::PredictorUpdate();
            for (unsigned i = 0; i < 256; ++i) valid[i] = false;
            ++checks;
            if (state.predictor_update_pending.valid) ++errors.reset_error;
            continue;
        }

        const uint8_t cfi_type = conditional ? boom::CFI_CONDITIONAL_BRANCH :
            (kind == 1 ? boom::CFI_JAL : (kind == 2 ? boom::CFI_JALR : boom::CFI_NONE));
        boom::FtqStepInput allocation;
        allocation.alloc_valid = true;
        allocation.allocation.packet_base_pc = pc;
        allocation.allocation.packet_valid_mask = 1;
        allocation.allocation.prediction_valid = conditional;
        allocation.allocation.cfi_lane = 0;
        allocation.allocation.cfi_type = cfi_type;
        allocation.allocation.predictor_metadata_index = static_cast<uint8_t>(
            metadata_match ? index : (index ^ 1u));
        allocation.allocation.predictor_generation = stale_predictor ?
            generation - 1 : generation;
        const boom::FtqStepOutput allocated = state.ftq.step(allocation);
        ++checks;
        if (!allocated.alloc_accepted) {
            ++errors.commit_order_error;
            continue;
        }

        MicroOp uop;
        uop.debug_pc = pc;
        uop.inst = conditional ? 0x00000063u :
            (kind == 1 ? 0x0000006fu : (kind == 2 ? 0x00000067u : 0x00000013u));
        uop.uopc = conditional ? 31 : (kind == 1 ? 29 : (kind == 2 ? 30 : 1));
        uop.branch.is_br = conditional;
        uop.branch.is_jal = kind == 1;
        uop.branch.is_jalr = kind == 2;
        uop.ftq_valid = true;
        uop.ftq_idx = allocated.alloc_ftq_idx;
        uop.ftq_lane = 0;
        uop.ftq_generation = allocated.alloc_generation + (stale_ftq ? 1u : 0u);
        uop.queue.rob_idx = state.rob.head;
        uop.queue.rob_allocation_id = static_cast<uint32_t>(cycle + 1);

        const uint64_t attempts_before = state.bim_training.attempts;
        const uint64_t accepted_before = state.bim_training.accepted;
        const uint64_t dropped_before = state.bim_training.dropped;
        const uint64_t stale_before = state.bim_training.stale_rejected;
        if (architectural_commit) {
            RobEntry& entry = state.rob.entries[state.rob.head];
            entry = RobEntry();
            entry.valid = true;
            entry.busy = false;
            entry.exception = exception;
            entry.uop = uop;
            entry.branch_resolved = resolved;
            entry.branch_actual_taken = actual_taken;
            state.rob.tail = static_cast<uint8_t>((state.rob.head + 1) % ROB_DEPTH);
            state.rob.maybe_full = false;
            boom::rob_commit_module(state, pipe);
        }

        const bool train = architectural_commit && !exception && conditional &&
            resolved && !stale_ftq && !stale_predictor && metadata_match;
        const bool attempted = architectural_commit && !exception && conditional;
        const bool dropped = attempted && !resolved;
        const bool stale_rejected = attempted && resolved && !train;
        const bool emitted = state.predictor_update_pending.valid;
        ++checks;
        if (emitted != train) ++errors.training_eligibility_error;
        if (emitted && state.predictor_update_pending.metadata_token != index)
            ++errors.training_index_error;
        if (emitted && state.predictor_update_pending.taken != actual_taken)
            ++errors.training_outcome_error;
        if (emitted && !architectural_commit) ++errors.wrong_path_training_error;
        if (emitted && squashed) ++errors.squashed_training_error;
        if (emitted && exception) ++errors.exception_training_error;
        if (emitted && (stale_ftq || stale_predictor || !metadata_match))
            ++errors.stale_training_error;
        if (emitted && kind == 1) ++errors.jal_training_error;
        if (emitted && kind == 2) ++errors.jalr_training_error;
        if (state.bim_training.attempts - attempts_before != (attempted ? 1u : 0u) ||
            state.bim_training.accepted - accepted_before != (train ? 1u : 0u))
            ++errors.training_eligibility_error;
        if (state.bim_training.dropped - dropped_before != (dropped ? 1u : 0u))
            ++errors.dropped_training_error;
        if (state.bim_training.stale_rejected - stale_before !=
            (stale_rejected ? 1u : 0u)) ++errors.stale_training_error;
        if (state.bim_training.duplicate != 0) ++errors.duplicate_training_error;

        boom::PredictorStepInput predictor_input;
        predictor_input.active_generation = generation;
        predictor_input.update = state.predictor_update_pending;
        predictor_input.req_valid = true;
        predictor_input.request.pc = pc;
        predictor_input.request.cfi_type = boom::CFI_CONDITIONAL_BRANCH;
        predictor_input.request.generation = generation;
        predictor_input.request.request_token = cycle;
        state.predictor.step(predictor_input);
        state.predictor_update_pending = boom::PredictorUpdate();

        if (train) {
            const uint8_t old_counter = valid[index] ? counters[index] : 1;
            counters[index] = transition(old_counter, actual_taken);
            valid[index] = true;
            ++updates;
        }
        ++checks;
        if (state.predictor.valid_[index] != valid[index]) ++errors.valid_bit_error;
        ++checks;
        if (valid[index] && state.predictor.counters_[index] != counters[index])
            ++errors.counter_transition_error;

        const bool expected_taken = (valid[index] ? counters[index] : 1) >= 2;
        const boom::PredictorStepOutput pending = state.predictor.peek(false);
        ++checks;
        if (!pending.resp_valid || pending.response.taken != expected_taken)
            ++errors.request_update_conflict_error;
        if (!pending.resp_valid || pending.response.taken != expected_taken)
            ++errors.prediction_after_training_error;
        boom::PredictorStepInput consume;
        consume.active_generation = generation;
        consume.resp_ready = true;
        state.predictor.step(consume);

        boom::FtqStepInput retire;
        retire.retire = state.ftq_retire_pending;
        boom::FtqStepOutput retired = state.ftq.step(retire);
        state.ftq_retire_pending = boom::FtqLaneEvent();
        if (!retired.retire_accepted) {
            boom::FtqStepInput cleanup;
            cleanup.retire.valid = true;
            cleanup.retire.ftq_idx = allocated.alloc_ftq_idx;
            cleanup.retire.generation = allocated.alloc_generation;
            cleanup.retire.lane = 0;
            state.ftq.step(cleanup);
        }
        while (!pipe.commit_trace.empty()) (void)pipe.commit_trace.read();
        state.frontend_redirect = FrontendRedirect();
        state.ftq_redirect_pending = boom::FtqRedirect();
        state.global_flush = false;
        state.rob.state = ROB_NORMAL;
    }
}

}  // namespace

int main() {
    Errors errors;
    uint64_t checks = 0;
    uint64_t updates = 0;
    exhaustive(errors, checks);
    const uint64_t exhaustive_errors = errors.total();
    std::printf("PF5_SMALL_STATE_EXHAUSTIVE combinations=128 checks=%llu errors=%llu\n",
                static_cast<unsigned long long>(checks),
                static_cast<unsigned long long>(exhaustive_errors));
    for (uint64_t seed = 1; seed <= 256; ++seed)
        campaign(seed, 8192, errors, updates, checks);
    const uint64_t random_errors = errors.total();
    std::printf("PF5_RANDOM seeds=256 cycles_per_seed=8192 updates=%llu errors=%llu\n",
                static_cast<unsigned long long>(updates),
                static_cast<unsigned long long>(random_errors));
    uint64_t long_run_updates = 0;
    campaign(0x5a17u, 1000000, errors, long_run_updates, checks);
    std::printf("PF5_LONG_RUN steps=1000000 updates=%llu errors=%llu\n",
                static_cast<unsigned long long>(long_run_updates),
                static_cast<unsigned long long>(errors.total()));
    return errors.total() == 0 ? 0 : 1;
}
