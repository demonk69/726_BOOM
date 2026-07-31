#include "boom_config.hpp"
#include "boom_types.hpp"
#include "boom_state.hpp"
#include "boom_interfaces.hpp"

namespace boom {

void frontend_module(BoomCoreState& state, PipeSignals& pipe) {
    FrontendState& fe = state.frontend;

    if (state.global_flush) {
        fe.flush = true;
        fe.request_sent = false;
        fe.fetch_packet_valid = false;
    }

    if (!fe.reset_done) {
        fe.pc = RESET_VECTOR;
        fe.fetch_id = 0;
        fe.reset_done = true;
        fe.request_sent = false;
    }

    if (state.brupdate.valid && state.brupdate.mispredict) {
        fe.pc = state.brupdate.jalr_target & ~0x3ULL;
        fe.request_sent = false;
        fe.response_received = false;
        fe.fetch_packet_valid = false;
        fe.flush = false;
        state.global_flush = false;
    }

    if (state.rob.state == ROB_EXCEPTION) {
        fe.pc = fe.pc;
        fe.fetch_packet_valid = false;
        return;
    }

    if (fe.flush) {
        fe.pc = state.brupdate.valid ? (state.brupdate.jalr_target & ~0x3ULL) : fe.pc;
        fe.request_sent = false;
        fe.response_received = false;
        fe.fetch_packet_valid = false;
        fe.flush = false;
        state.global_flush = false;
    }

    fe.stalled = state.rename.dispatch_packets[0].valid || state.decode.dec_valids[0];
    if (fe.stalled) return;

    if (!fe.request_sent && fe.fetch_packet_valid) {
        fe.response_received = false;
        fe.fetch_packet_valid = false;
    }

    if (!fe.request_sent) {
        ImemRequest req;
        req.address = fe.pc;
        req.fetch_id = fe.fetch_id;
        req.kill = false;
        if (!pipe.imem_req.full()) {
            pipe.imem_req.write(req);
            fe.pending_fetch_id = fe.fetch_id;
            fe.fetch_id++;
            fe.request_sent = true;
        }
    }

    if (!pipe.imem_resp.empty()) {
        ImemResponse resp = pipe.imem_resp.read();
        if (fe.request_sent && resp.fetch_id == fe.pending_fetch_id) {
            fe.resp_address = resp.address;
            fe.resp_instruction = resp.instruction;
            fe.resp_exception = resp.exception;
            fe.resp_exc_cause = resp.exc_cause;
            fe.response_received = true;
            fe.request_sent = false;
        }
    }

    fe.fetch_packet_valid = false;
    if (fe.response_received && !fe.request_sent) {
        MicroOp uop;
        uop.debug_pc = fe.resp_address;
        uop.inst = fe.resp_instruction;
        uop.is_rvc = false;
        fe.fetch_uop = uop;
        fe.fetch_packet_valid = true;
        fe.pc = fe.resp_address + 4;
        fe.response_received = false;
    }

    state.frontend = fe;
}

}
