#include "boom_config.hpp"
#include "boom_types.hpp"
#include "boom_state.hpp"
#include "boom_interfaces.hpp"

namespace boom {

static bool architectural_redirect_owner_valid(const BoomCoreState& state) {
    const FrontendRedirect& redirect = state.frontend_redirect;
    if (redirect.cause == FRONTEND_REDIRECT_DEBUG ||
        redirect.cause == FRONTEND_REDIRECT_INTERRUPT) return true;
    if (redirect.rob_idx >= ROB_DEPTH) return false;
    const RobEntry& owner = state.rob.entries[redirect.rob_idx];
    return owner.valid && owner.uop.queue.rob_allocation_id == redirect.allocation_id;
}

void frontend_module(BoomCoreState& state, PipeSignals& pipe) {
    FrontendState& fe = state.frontend;

    const bool reset_redirect = !fe.reset_done;
    if (reset_redirect) {
        fe.pc = RESET_VECTOR;
        fe.fetch_id = 0;
        fe.epoch++;
        fe.reset_done = true;
        fe.request_sent = false;
        fe.response_received = false;
        fe.fetch_packet_valid = false;
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
    const bool target_redirect = architectural_redirect || branch_redirect;
    if (redirect) {
        const uint64_t target = architectural_redirect ? state.frontend_redirect.target_pc :
            (branch_redirect ? state.brupdate.jalr_target : fe.pc);
        if (!reset_redirect) fe.epoch++;
        fe.pc = target;
        fe.request_sent = false;
        fe.response_received = false;
        fe.fetch_packet_valid = false;
        fe.flush = false;
        state.global_flush = false;
        if (reset_redirect || architectural_redirect) state.frontend_redirect.valid = false;

        if (target_redirect && (target & 0x3ULL) != 0) {
            MicroOp fault;
            fault.debug_pc = target;
            fault.exception = true;
            fault.exc_cause = 0;
            fault.exc.exception = true;
            fault.exc.exc_cause = 0;
            fault.exc.xcpt_ma_if = true;
            fe.fetch_uop = fault;
            fe.fetch_packet_valid = true;
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
            fe.response_received = true;
            fe.request_sent = false;
        }
    }

    if (state.rob.state == ROB_EXCEPTION) {
        fe.fetch_packet_valid = false;
        return;
    }

    if (target_redirect && (fe.pc & 0x3ULL) != 0) return;

    fe.stalled = state.rename.dispatch_packets[0].valid || state.decode.dec_valids[0];
    if (fe.stalled) return;

    if (fe.fetch_packet_valid) {
        fe.fetch_packet_valid = false;
    }

    if (fe.response_received) {
        MicroOp uop;
        uop.debug_pc = fe.resp_address;
        uop.inst = fe.resp_instruction;
        uop.is_rvc = false;
        uop.exception = fe.resp_exception;
        uop.exc_cause = fe.resp_exc_cause;
        uop.exc.exception = fe.resp_exception;
        uop.exc.exc_cause = fe.resp_exc_cause;
        uop.exc.xcpt_ae_if = fe.resp_exception;
        fe.fetch_uop = uop;
        fe.fetch_packet_valid = true;
        fe.pc = fe.resp_address + 4;
        fe.response_received = false;
    }

    if (!fe.request_sent) {
        ImemRequest req;
        req.address = fe.pc;
        req.fetch_id = fe.fetch_id;
        req.epoch = fe.epoch;
        req.kill = false;
        if (!pipe.imem_req.full()) {
            pipe.imem_req.write(req);
            fe.pending_fetch_id = fe.fetch_id;
            fe.pending_epoch = fe.epoch;
            fe.pending_address = fe.pc;
            fe.fetch_id++;
            fe.request_sent = true;
        }
    }
}

}
