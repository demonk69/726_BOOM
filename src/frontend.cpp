#include "boom_config.hpp"
#include "boom_types.hpp"
#include "boom_state.hpp"
#include "boom_interfaces.hpp"
#include "fetch_buffer.hpp"
#include "rvc.hpp"

namespace boom {

static bool architectural_redirect_owner_valid(const BoomCoreState& state) {
    const FrontendRedirect& redirect = state.frontend_redirect;
    if (redirect.cause == FRONTEND_REDIRECT_DEBUG ||
        redirect.cause == FRONTEND_REDIRECT_INTERRUPT) return true;
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
    return uop;
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
        fe.halfword_valid = false;
        fe.fetch_packet_valid = false;
        fe.producer_valid = false;
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

    if (state.rob.state == ROB_EXCEPTION) {
        fe.fetch_packet_valid = false;
        fe.producer_valid = false;
        fetch_buffer_reset(fe.fetch_buffer);
        return;
    }

    FetchPacket packet;
    if (fe.producer_valid) {
        packet.valid = true;
        packet.valid_mask = 1;
        packet.slots[0] = buffer_entry(fe.producer_uop, fe.producer_fetch_id);
    }
    const bool decode_ready = !state.rename.dispatch_packets[0].valid &&
        !state.decode.dec_valids[0] && !redirect;
    const FetchBufferResult buffer = fetch_buffer_step(
        fe.fetch_buffer, packet, decode_ready, redirect);
    fe.fetch_packet_valid = buffer.dequeue_valid;
    if (buffer.dequeue_valid) fe.fetch_uop = buffer_uop(buffer.dequeue_bits);
    if (buffer.enqueue_fire) fe.producer_valid = false;
    fe.stalled = fe.producer_valid;

    if (misaligned_redirect || fe.stalled) return;

    // An odd redirect remains fenced after its one-shot fault is consumed.
    // Only a later redirect or reset may establish a fetchable PC.
    if ((fe.pc & 0x1ULL) != 0) return;

    if (fe.response_received && fe.halfword_valid) {
        if (fe.halfword_epoch != fe.epoch) {
            fe.response_received = false;
            fe.halfword_valid = false;
        } else if (fe.resp_exception) {
            fe.producer_uop = fetch_fault(fe.halfword_pc, fe.resp_exc_cause, true);
        } else {
            MicroOp uop;
            uop.debug_pc = fe.halfword_pc;
            uop.inst = (fe.resp_instruction << 16) | fe.halfword;
            uop.is_rvc = false;
            fe.producer_uop = uop;
        }
        if (fe.halfword_valid) {
            fe.producer_fetch_id = fe.resp_fetch_id;
            fe.producer_valid = true;
            fe.pc = fe.halfword_pc + 4;
            fe.halfword_valid = false;
            fe.response_received = false;
        }
    } else if (fe.response_received) {
        const uint64_t parcel_pc = fe.pc;
        const bool upper = (parcel_pc & 0x2ULL) != 0;
        const uint16_t parcel = upper ?
            static_cast<uint16_t>(fe.resp_instruction >> 16) :
            static_cast<uint16_t>(fe.resp_instruction);

        if (fe.resp_exception) {
            fe.producer_uop = fetch_fault(parcel_pc, fe.resp_exc_cause, true);
            fe.producer_fetch_id = fe.resp_fetch_id;
            fe.producer_valid = true;
            fe.pc = parcel_pc + 4;
            fe.response_received = false;
        } else if ((parcel & 0x3u) != 0x3u) {
            const RvcDecodeResult rvc = decompress_rvc(parcel);
            MicroOp uop;
            uop.debug_pc = parcel_pc;
            uop.debug_inst = parcel;
            uop.is_rvc = true;
            if (!rvc.legal) {
                uop.exception = true;
                uop.exc_cause = 2;
                uop.exc.exception = true;
                uop.exc.exc_cause = 2;
            } else {
                uop.inst = rvc.instruction;
            }
            fe.producer_uop = uop;
            fe.producer_fetch_id = fe.resp_fetch_id;
            fe.producer_valid = true;
            fe.pc = parcel_pc + 2;
            if (upper) fe.response_received = false;
        } else if ((parcel & 0x1fu) == 0x1fu) {
            fe.producer_uop = fetch_fault(parcel_pc, 2, false);
            fe.producer_uop.debug_inst = parcel;
            fe.producer_fetch_id = fe.resp_fetch_id;
            fe.producer_valid = true;
            fe.pc = parcel_pc + 2;
            fe.response_received = false;
        } else if (!upper) {
            MicroOp uop;
            uop.debug_pc = parcel_pc;
            uop.inst = fe.resp_instruction;
            uop.is_rvc = false;
            fe.producer_uop = uop;
            fe.producer_fetch_id = fe.resp_fetch_id;
            fe.producer_valid = true;
            fe.pc = parcel_pc + 4;
            fe.response_received = false;
        } else {
            fe.halfword_valid = true;
            fe.halfword = parcel;
            fe.halfword_pc = parcel_pc;
            fe.halfword_epoch = fe.epoch;
            fe.response_received = false;
        }
    }

    if (!fe.request_sent && !fe.response_received) {
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

}
