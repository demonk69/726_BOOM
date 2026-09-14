#include "boom_config.hpp"
#include "boom_types.hpp"
#include "boom_state.hpp"
#include "boom_interfaces.hpp"
#include "fetch_buffer.hpp"
#include "fetch_packet.hpp"
#include "predecode.hpp"
#include "predictor.hpp"
#include "rvc.hpp"

namespace boom {

static bool architectural_redirect_owner_valid(const BoomCoreState& state) {
    const FrontendRedirect& redirect = state.frontend_redirect;
    if (redirect.cause == FRONTEND_REDIRECT_EXCEPTION)
        return state.exception_commit.valid &&
               state.exception_commit.target == redirect.target_pc;
    if (redirect.cause == FRONTEND_REDIRECT_DEBUG ||
        redirect.cause == FRONTEND_REDIRECT_INTERRUPT ||
        redirect.cause == FRONTEND_REDIRECT_ERET) return true;
    if (redirect.rob_idx >= ROB_DEPTH) return false;
    const RobEntry& owner = state.rob.entries[redirect.rob_idx];
    return owner.valid && owner.uop.queue.rob_allocation_id == redirect.allocation_id;
}

static MicroOp fetch_fault(uint64_t pc, uint64_t cause, bool access_fault) {
#pragma HLS INLINE
    MicroOp fault;
    fault.debug_pc = pc;
    fault.exception = true;
    fault.exc_cause = cause;
    fault.exc.exception = true;
    fault.exc.exc_cause = cause;
    fault.exc.xcpt_ae_if = access_fault;
    fault.exc.xcpt_ma_if = !access_fault && cause == 0;
    return fault;
}

static FetchInstruction buffer_entry(const MicroOp& uop, uint32_t fetch_id) {
#pragma HLS INLINE
    FetchInstruction entry;
    entry.pc = uop.debug_pc;
    entry.instruction = uop.inst;
    entry.original_instruction = uop.debug_inst;
    entry.fetch_id = fetch_id;
    entry.is_rvc = uop.is_rvc;
    entry.exception = uop.exception;
    entry.exception_cause = uop.exc_cause;
    entry.exception_access_fault = uop.exc.xcpt_ae_if;
    entry.exception_misaligned = uop.exc.xcpt_ma_if;
    entry.ftq_valid = uop.ftq_valid;
    entry.ftq_idx = uop.ftq_idx;
    entry.ftq_generation = uop.ftq_generation;
    entry.ftq_lane = uop.ftq_lane;
    entry.ftq_halfword_offset = uop.ftq_halfword_offset;
    return entry;
}

static MicroOp buffer_uop(const FetchInstruction& entry) {
#pragma HLS INLINE
    MicroOp uop;
    uop.debug_pc = entry.pc;
    uop.inst = entry.instruction;
    uop.debug_inst = entry.original_instruction;
    uop.is_rvc = entry.is_rvc;
    uop.exception = entry.exception;
    uop.exc_cause = entry.exception_cause;
    uop.exc.exception = entry.exception;
    uop.exc.exc_cause = entry.exception_cause;
    uop.exc.xcpt_ae_if = entry.exception_access_fault;
    uop.exc.xcpt_ma_if = entry.exception_misaligned;
    uop.ftq_valid = entry.ftq_valid;
    uop.ftq_idx = entry.ftq_idx;
    uop.ftq_generation = entry.ftq_generation;
    uop.ftq_lane = entry.ftq_lane;
    uop.ftq_halfword_offset = entry.ftq_halfword_offset;
    return uop;
}

static void clear_prediction_context(FrontendState& fe) {
    fe.pending_predecode = CfiPacketPredecodeResult();
    fe.original_packet_mask = 0;
    fe.final_admission_mask = 0;
    fe.prediction_pending = false;
    fe.predictor_request_sent = false;
    fe.prediction_epoch = 0;
    fe.prediction_generation = 0;
    fe.prediction_token = 0;
    fe.prediction_resolved = false;
    fe.predictor_metadata_index = 0;
}

static FtqAllocation packet_ftq_allocation(const BoomCoreState& state) {
    const FrontendState& fe = state.frontend;
    FtqAllocation allocation;
    allocation.packet_base_pc = fe.pending_packet.slots[0].pc;
    allocation.packet_valid_mask = fe.final_admission_mask;
    allocation.predictor_generation = state.predictor_generation;
    if (!fe.pending_predecode.packet_has_cfi) return allocation;

    const CfiPredecodeResult& cfi = fe.pending_predecode.selected_cfi_result;
    const uint8_t lane = fe.pending_predecode.selected_cfi_lane;
    allocation.cfi_lane = lane;
    allocation.cfi_type = cfi.cfi_type;
    allocation.predictor_metadata_index = cfi.cfi_type == CFI_CONDITIONAL_BRANCH ?
        fe.predictor_metadata_index : static_cast<uint8_t>(
            (fe.pending_packet.slots[lane].pc >> 1) & 0xffu);
    if (cfi.cfi_type == CFI_CONDITIONAL_BRANCH) {
        allocation.prediction_valid = fe.predictor_prediction_valid;
        allocation.predicted_taken = fe.predictor_predicted_taken;
        allocation.target_valid = fe.predictor_target_valid;
        allocation.predicted_target = fe.predictor_target;
    } else if (cfi.cfi_type == CFI_JAL && cfi.static_target_valid) {
        allocation.prediction_valid = true;
        allocation.predicted_taken = true;
        allocation.target_valid = true;
        allocation.predicted_target = cfi.static_target;
    }
    return allocation;
}

static void stamp_ftq_reference(FetchPacket& packet, uint64_t base_pc,
                                uint8_t idx, uint32_t generation) {
    for (uint8_t lane = 0; lane < FETCH_PACKET_WIDTH; ++lane) {
        if ((packet.valid_mask & static_cast<uint8_t>(1u << lane)) == 0) continue;
        FetchInstruction& instruction = packet.slots[lane];
        instruction.ftq_valid = true;
        instruction.ftq_idx = idx;
        instruction.ftq_generation = generation;
        instruction.ftq_lane = lane;
        instruction.ftq_halfword_offset = static_cast<uint8_t>(
            ((instruction.pc - base_pc) >> 1) & 3u);
    }
}

static bool fetch_buffer_can_accept(const FetchBufferState& buffer,
                                    const FetchPacket& packet,
                                    bool dequeue_ready) {
    const uint8_t incoming = static_cast<uint8_t>(
        (packet.valid_mask & 1u) + ((packet.valid_mask >> 1) & 1u));
    const bool dequeue_fire = buffer.count != 0 && dequeue_ready;
    const uint8_t available = static_cast<uint8_t>(FETCH_BUFFER_DEPTH - buffer.count +
                                                    (dequeue_fire ? 1 : 0));
    return packet.valid && incoming <= available;
}

template <bool ProductFtq>
static void frontend_mode_module(BoomCoreState& state, PipeSignals& pipe) {
    FrontendState& fe = state.frontend;

    fe.predictor_request_accepted = false;
    fe.predictor_response_valid = false;
    fe.predictor_response_stale = false;
    fe.packet_accept = false;
    fe.ftq_alloc_accepted = false;
    fe.accepted_packet_mask = 0;

    const bool reset_redirect = !fe.reset_done;
    if (reset_redirect) {
        fe.pc = RESET_VECTOR;
        fe.fetch_id = 0;
        fe.epoch++;
        fe.reset_done = true;
        fe.request_sent = false;
        fe.response_received = false;
        fe.halfword_valid = false;
        fe.fetch_packet_valid = false;
        fe.producer_valid = false;
        fe.pending_packet = FetchPacket();
        clear_prediction_context(fe);
        fetch_buffer_reset(fe.fetch_buffer);
    }

    const bool architectural_redirect_requested = !reset_redirect && state.frontend_redirect.valid;
    const bool architectural_redirect = architectural_redirect_requested &&
        architectural_redirect_owner_valid(state);
    if (architectural_redirect_requested && !architectural_redirect)
        state.frontend_redirect.valid = false;
    const bool branch_redirect = !reset_redirect && !architectural_redirect &&
        state.brupdate.valid && state.brupdate.mispredict;
    const bool generic_flush = !reset_redirect && !architectural_redirect &&
        !branch_redirect && state.global_flush;
    const bool redirect = reset_redirect || architectural_redirect ||
        branch_redirect || generic_flush || fe.flush;
    bool misaligned_redirect = false;
    if (redirect) {
        const uint64_t target = architectural_redirect ? state.frontend_redirect.target_pc :
            (branch_redirect ? state.brupdate.jalr_target : fe.pc);
        if (!reset_redirect) fe.epoch++;
        fe.pc = target;
        fe.request_sent = false;
        fe.response_received = false;
        fe.halfword_valid = false;
        fe.fetch_packet_valid = false;
        fe.producer_valid = false;
        fe.pending_packet = FetchPacket();
        clear_prediction_context(fe);
        fetch_buffer_reset(fe.fetch_buffer);
        state.decode.dec_valids[0] = false;
        state.decode.dec_uops[0] = MicroOp();
        fe.flush = false;
        if (reset_redirect || architectural_redirect) state.frontend_redirect.valid = false;

        misaligned_redirect = !reset_redirect && (target & 0x1ULL) != 0;
        if (misaligned_redirect) {
            fe.producer_uop = fetch_fault(target, 0, false);
            fe.producer_fetch_id = fe.fetch_id;
            fe.producer_valid = true;
            fe.pending_packet.valid = true;
            fe.pending_packet.valid_mask = 1;
            fe.pending_packet.slots[0] = buffer_entry(fe.producer_uop,
                                                       fe.producer_fetch_id);
        }
    }

    // Responses are always drained. A redirect in this cycle invalidates the
    // transaction before matching, so redirect wins over response.
    if (!pipe.imem_resp.empty()) {
        ImemResponse resp = pipe.imem_resp.read();
        if (!redirect && fe.request_sent &&
            resp.fetch_id == fe.pending_fetch_id &&
            resp.epoch == fe.pending_epoch &&
            resp.address == fe.pending_address) {
            fe.resp_address = resp.address;
            fe.resp_instruction = resp.instruction;
            fe.resp_exception = resp.exception;
            fe.resp_exc_cause = resp.exc_cause;
            fe.resp_fetch_id = resp.fetch_id;
            fe.response_received = true;
            fe.request_sent = false;
        }
    }

    // Recoverable PF1 takes return the ROB to ROB_NORMAL; preserve the
    // existing fence for externally seeded terminal exception states.
    if (state.rob.state == ROB_EXCEPTION) {
        PredictorStepInput predictor_input;
        predictor_input.active_generation = state.predictor_generation;
        predictor_input.resp_ready = true;
        predictor_input.update = state.predictor_update_pending;
        const PredictorStepOutput predictor_output = state.predictor.step(predictor_input);
        state.predictor_update_pending = PredictorUpdate();
        fe.predictor_response_valid = predictor_output.resp_valid;
        fe.predictor_response_stale = predictor_output.resp_valid;
        fe.fetch_packet_valid = false;
        fe.producer_valid = false;
        fe.pending_packet = FetchPacket();
        clear_prediction_context(fe);
        fe.response_received = false;
        fe.request_sent = false;
        fe.halfword_valid = false;
        fe.stalled = false;
        fetch_buffer_reset(fe.fetch_buffer);
        return;
    }

    // Keep direct scalar seeding used by the accepted B2 harness as a lane-0
    // compatibility input. Product response parsing writes pending_packet.
    if (fe.producer_valid && !fe.pending_packet.valid) {
        fe.pending_packet.valid = true;
        fe.pending_packet.valid_mask = 1;
        fe.pending_packet.slots[0] = buffer_entry(fe.producer_uop,
                                                   fe.producer_fetch_id);
        fe.original_packet_mask = 1;
        fe.final_admission_mask = 1;
    }
    const bool decode_ready = !state.rename.dispatch_packets[0].valid &&
        !state.decode.dec_valids[0] && !redirect;

    FetchBufferResult buffer;
    bool buffer_stepped = false;
    if (!ProductFtq && fe.pending_packet.valid &&
        !fe.prediction_pending) {
        buffer = fetch_buffer_step(fe.fetch_buffer, fe.pending_packet,
                                   decode_ready, redirect);
        buffer_stepped = true;
        fe.fetch_packet_valid = buffer.dequeue_valid;
        if (buffer.dequeue_valid) fe.fetch_uop = buffer_uop(buffer.dequeue_bits);
        fe.packet_accept = buffer.enqueue_fire;
        if (buffer.enqueue_fire) {
            fe.pending_packet = FetchPacket();
            fe.producer_valid = false;
            clear_prediction_context(fe);
        }
    }

    bool packet_built = false;
    if (!misaligned_redirect && !fe.pending_packet.valid && fe.response_received) {
        if (fe.halfword_valid && fe.halfword_epoch != fe.epoch) {
            fe.response_received = false;
            fe.halfword_valid = false;
        } else {
            FetchPacketInput input;
            input.pc = fe.pc;
            input.instruction = fe.resp_instruction;
            input.fetch_id = fe.resp_fetch_id;
            input.exception = fe.resp_exception;
            input.exception_cause = fe.resp_exc_cause;
            input.carry_valid = fe.halfword_valid;
            input.carry = fe.halfword;
            input.carry_pc = fe.halfword_pc;
            const FetchPacketResult built = build_fetch_packet(input);

            fe.pc = built.next_pc;
            fe.halfword_valid = built.carry_valid;
            fe.halfword = built.carry;
            fe.halfword_pc = built.carry_pc;
            fe.halfword_epoch = fe.epoch;
            fe.response_received = false;
            fe.pending_packet = built.packet;
            fe.original_packet_mask = built.packet.valid_mask;
            fe.final_admission_mask = built.packet.valid_mask;
            fe.pending_predecode = CfiPacketPredecodeResult();
            fe.prediction_pending = false;
            fe.predictor_request_sent = false;
            fe.predictor_prediction_valid = false;
            fe.predictor_predicted_taken = false;
            fe.predictor_target_valid = false;
            fe.predictor_target = 0;
            packet_built = built.packet.valid;

            if (built.packet.valid) {
                uint8_t predecode_mask = built.packet.valid_mask;
                if ((built.packet.valid_mask & 1u) != 0 &&
                    built.packet.slots[0].exception) {
                    fe.final_admission_mask = 1;
                    predecode_mask = 0;
                } else if ((built.packet.valid_mask & 2u) != 0 &&
                           built.packet.slots[1].exception) {
                    fe.final_admission_mask = built.packet.valid_mask;
                    predecode_mask = 1;
                }
                fe.pending_packet.valid_mask = fe.final_admission_mask;
                const FetchInstruction& lane0 = built.packet.slots[0];
                const FetchInstruction& lane1 = built.packet.slots[1];
                fe.pending_predecode = predecode_cfi_packet(
                    predecode_mask,
                    lane0.pc, lane0.instruction, lane0.is_rvc,
                    lane1.pc, lane1.instruction, lane1.is_rvc);
                if (fe.pending_predecode.packet_has_cfi) {
                    const CfiPredecodeResult& cfi =
                        fe.pending_predecode.selected_cfi_result;
                    fe.pending_predecode.younger_lane_mask =
                        fe.pending_predecode.selected_cfi_lane == 0 ?
                        static_cast<uint8_t>(fe.final_admission_mask & 2u) : 0;
                    if (cfi.cfi_type == CFI_CONDITIONAL_BRANCH) {
                        fe.prediction_pending = true;
                        fe.prediction_epoch = fe.epoch;
                        fe.prediction_generation = state.predictor_generation;
                        fe.prediction_token = fe.next_prediction_token++;
                    } else if (cfi.cfi_type == CFI_JAL &&
                               cfi.static_target_valid) {
                        fe.final_admission_mask = mask_younger_packet_lanes(
                            fe.original_packet_mask, fe.pending_predecode);
                        fe.pending_packet.valid_mask = fe.final_admission_mask;
                        fe.pc = cfi.static_target;
                        fe.halfword_valid = false;
                    }
                }
            }

            fe.producer_valid = built.packet.valid;
            if (built.packet.valid) {
                fe.producer_fetch_id = built.packet.slots[0].fetch_id;
                fe.producer_uop = buffer_uop(built.packet.slots[0]);
            }
        }
    }

    PredictorStepInput predictor_input;
    predictor_input.active_generation = state.predictor_generation;
    predictor_input.update = state.predictor_update_pending;
    if (fe.prediction_pending && !fe.predictor_request_sent) {
        const uint8_t lane = fe.pending_predecode.selected_cfi_lane;
        const CfiPredecodeResult& cfi = fe.pending_predecode.selected_cfi_result;
        predictor_input.req_valid = true;
        predictor_input.request.pc = fe.pending_packet.slots[lane].pc;
        predictor_input.request.cfi_lane = lane;
        predictor_input.request.cfi_type = cfi.cfi_type;
        predictor_input.request.static_target_valid = cfi.static_target_valid;
        predictor_input.request.static_target = cfi.static_target;
        predictor_input.request.generation = fe.prediction_generation;
        predictor_input.request.request_token = fe.prediction_token;
    }
    const bool legacy_prediction_can_admit = fe.prediction_pending &&
        fe.predictor_request_sent && !packet_built && !redirect &&
        fetch_buffer_can_accept(fe.fetch_buffer, fe.pending_packet, decode_ready);
    predictor_input.resp_ready = ProductFtq ||
        !fe.prediction_pending || legacy_prediction_can_admit;
    const PredictorStepOutput predictor_output = state.predictor.peek(false);
    state.predictor.step(predictor_input);
    state.predictor_update_pending = PredictorUpdate();
    fe.predictor_response_valid = predictor_output.resp_valid ||
        (fe.prediction_pending && fe.prediction_resolved);

    if (predictor_input.req_valid && predictor_output.req_ready) {
        fe.predictor_request_sent = true;
        fe.predictor_request_accepted = true;
    }

    const bool matching_prediction = predictor_output.resp_valid &&
        fe.prediction_pending && fe.predictor_request_sent &&
        predictor_output.response.generation == fe.prediction_generation &&
        predictor_output.response.request_token == fe.prediction_token &&
        predictor_output.response.cfi_lane == fe.pending_predecode.selected_cfi_lane &&
        predictor_output.response.cfi_type == CFI_CONDITIONAL_BRANCH &&
        fe.prediction_epoch == fe.epoch;
    if (predictor_output.resp_valid && !matching_prediction)
        fe.predictor_response_stale = true;
    if (matching_prediction) {
        fe.predictor_prediction_valid = predictor_output.response.prediction_valid;
        fe.predictor_predicted_taken = predictor_output.response.taken;
        fe.predictor_target_valid = predictor_output.response.target_valid;
        fe.predictor_target = predictor_output.response.target;
        fe.predictor_metadata_index = predictor_output.response.metadata_token;
        fe.prediction_resolved = true;
        if (ProductFtq && predictor_output.response.prediction_valid &&
            predictor_output.response.taken &&
            predictor_output.response.target_valid) {
            fe.final_admission_mask = mask_younger_packet_lanes(
                fe.final_admission_mask, fe.pending_predecode);
            fe.pending_packet.valid_mask = fe.final_admission_mask;
            fe.pc = predictor_output.response.target;
            fe.halfword_valid = false;
        }
    }

    const bool packet_final_valid = !packet_built && fe.pending_packet.valid &&
        (!fe.prediction_pending || fe.prediction_resolved);
    const bool fb_ready = packet_final_valid &&
        fetch_buffer_can_accept(fe.fetch_buffer, fe.pending_packet, decode_ready);

    FtqStepInput ftq_input;
    FetchPacket admission_packet;
    if (ProductFtq) {
        ftq_input.retire = state.ftq_retire_pending;
        ftq_input.redirect = state.ftq_redirect_pending;
        if (redirect && !reset_redirect && !ftq_input.redirect.valid)
            ftq_input.reset = true;
        ftq_input.read_valid = state.ftq_read_request.valid;
        ftq_input.read_ftq_idx = state.ftq_read_request.ftq_idx;
        ftq_input.read_generation = state.ftq_read_request.generation;
        if (packet_final_valid && fb_ready && !redirect && !ftq_input.redirect.valid) {
            ftq_input.alloc_valid = fe.final_admission_mask != 0;
            ftq_input.allocation = packet_ftq_allocation(state);
        }
        state.ftq_last_output = state.ftq.step(ftq_input);
        fe.ftq_alloc_ready = state.ftq_last_output.alloc_ready;
        fe.ftq_alloc_accepted = state.ftq_last_output.alloc_accepted;
        fe.ftq_alloc_idx = state.ftq_last_output.alloc_ftq_idx;
        fe.ftq_alloc_generation = state.ftq_last_output.alloc_generation;
        state.ftq_retire_pending = FtqLaneEvent();
        state.ftq_redirect_pending = FtqRedirect();
        state.ftq_read_request = FtqLaneEvent();
        if (ftq_input.redirect.valid) {
            if (state.ftq_last_output.retire_rejected)
                state.ftq_retire_pending = ftq_input.retire;
            if (!state.ftq_last_output.redirect_accepted)
                state.ftq_exception_retire_deferred = FtqLaneEvent();
        }
        if (state.ftq_exception_retire_deferred.valid &&
            !state.ftq_retire_pending.valid) {
            state.ftq_retire_pending = state.ftq_exception_retire_deferred;
            state.ftq_exception_retire_deferred = FtqLaneEvent();
        }
        if (state.ftq_last_output.alloc_accepted) {
            admission_packet = fe.pending_packet;
            stamp_ftq_reference(admission_packet,
                                ftq_input.allocation.packet_base_pc,
                                state.ftq_last_output.alloc_ftq_idx,
                                state.ftq_last_output.alloc_generation);
        }
    } else if (packet_final_valid && fb_ready && !redirect) {
        admission_packet = fe.pending_packet;
    }
    if (!buffer_stepped) {
        buffer = fetch_buffer_step(fe.fetch_buffer, admission_packet,
                                   decode_ready, redirect);
        fe.fetch_packet_valid = buffer.dequeue_valid;
        if (buffer.dequeue_valid) fe.fetch_uop = buffer_uop(buffer.dequeue_bits);
        fe.packet_accept = ProductFtq ?
            state.ftq_last_output.alloc_accepted && buffer.enqueue_fire :
            buffer.enqueue_fire;
        if (fe.packet_accept) {
            fe.accepted_packet_mask = admission_packet.valid_mask;
            fe.pending_packet = FetchPacket();
            fe.producer_valid = false;
            clear_prediction_context(fe);
        }
    }
    fe.stalled = fe.pending_packet.valid;

    if (misaligned_redirect) return;

    // An odd redirect remains fenced after its one-shot fault is consumed.
    // Only a later redirect or reset may establish a fetchable PC.
    if ((fe.pc & 0x1ULL) != 0) return;

    const bool bypass_build_can_issue = packet_built && !fe.prediction_pending;
    if ((!fe.stalled || bypass_build_can_issue) && !fe.request_sent &&
        !fe.response_received) {
        ImemRequest req;
        req.address = fe.halfword_valid ?
            ((fe.halfword_pc + 2) & ~0x3ULL) : (fe.pc & ~0x3ULL);
        req.fetch_id = fe.fetch_id;
        req.epoch = fe.epoch;
        req.kill = false;
        if (!pipe.imem_req.full()) {
            pipe.imem_req.write(req);
            fe.pending_fetch_id = fe.fetch_id;
            fe.pending_epoch = fe.epoch;
            fe.pending_address = req.address;
            fe.fetch_id++;
            fe.request_sent = true;
        }
    }
}

void frontend_module(BoomCoreState& state, PipeSignals& pipe) {
    if (state.product_ftq_enabled) frontend_mode_module<true>(state, pipe);
    else frontend_mode_module<false>(state, pipe);
}

void frontend_product_module(BoomCoreState& state, PipeSignals& pipe) {
    frontend_mode_module<true>(state, pipe);
}

}
