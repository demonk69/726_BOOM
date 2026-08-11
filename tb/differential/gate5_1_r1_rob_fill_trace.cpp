#include "boom_config.hpp"
#include "boom_types.hpp"
#include "boom_state.hpp"
#include "boom_interfaces.hpp"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

extern void boom_core_step(BoomCoreState&, PipeSignals&);

static unsigned rob_count(const BoomCoreState& state) {
    unsigned count = 0;
    for (unsigned i = 0; i < ROB_DEPTH; ++i) count += state.rob.entries[i].valid ? 1u : 0u;
    return count;
}

static uint32_t frontend_epoch(const BoomCoreState& state) {
#ifdef GATE5_1_R1_LEGACY_FRONTEND
    (void)state;
    return 0;
#else
    return state.frontend.epoch;
#endif
}

static uint32_t request_epoch(const ImemRequest& request) {
#ifdef GATE5_1_R1_LEGACY_FRONTEND
    (void)request;
    return 0;
#else
    return request.epoch;
#endif
}

static uint32_t response_epoch(const ImemResponse& response) {
#ifdef GATE5_1_R1_LEGACY_FRONTEND
    (void)response;
    return 0;
#else
    return response.epoch;
#endif
}

static void set_response_epoch(ImemResponse& response, uint32_t epoch) {
#ifdef GATE5_1_R1_LEGACY_FRONTEND
    (void)response;
    (void)epoch;
#else
    response.epoch = epoch;
#endif
}

static ImemResponse response_for(const ImemRequest& request) {
    ImemResponse response;
    response.address = request.address;
    response.fetch_id = request.fetch_id;
    set_response_epoch(response, request_epoch(request));
    response.instruction = 0x00000013u;
    return response;
}

static const char* no_request_reason(const BoomCoreState& state, bool request_fire) {
    if (request_fire) return "none";
    if (state.rob.state == ROB_EXCEPTION) return "rob_exception";
    if (state.rename.dispatch_packets[0].valid) return "dispatch_hold";
    if (state.decode.dec_valids[0]) return "decode_hold";
    if (state.frontend.request_sent) return "outstanding";
    if (state.frontend.fetch_packet_valid) return "held_entry";
    return "no_eligible_request";
}

static void write_header(std::ofstream& out) {
    out << "cycle,frontend_pc,imem_req_valid,imem_req_ready,imem_req_fire,request_addr,"
           "request_fetch_id,request_epoch,outstanding_valid,response_valid,response_ready,"
           "response_fire,response_match_id,response_match_epoch,response_match_addr,"
           "decode_hold_valid,decode_ready,decode_fire,rename_fire,dispatch_fire,rob_allocate,"
           "rob_count,rob_head,rob_tail,commit_valid,redirect_valid,flush_valid,stale_response,"
           "reason_no_request\n";
}

static void run_rob_full_fixture(const std::string& path) {
    BoomCoreState state;
    PipeSignals pipe;
    state.frontend.reset_done = true;
    state.rob.state = ROB_NORMAL;
    state.rob.head = 0;
    state.rob.tail = 0;
    state.rob.maybe_full = true;
    for (unsigned i = 0; i < ROB_DEPTH; ++i) {
        state.rob.entries[i].valid = true;
        state.rob.entries[i].busy = true;
    }

    ImemResponse injected;
    injected.address = RESET_VECTOR;
    injected.fetch_id = 0;
    set_response_epoch(injected, 0);
    injected.instruction = 0x00100093u;
    pipe.imem_resp.write(injected);

    std::ofstream out(path.c_str());
    write_header(out);
    for (unsigned cycle = 0; cycle < 6; ++cycle) {
        const bool response_valid = !pipe.imem_resp.empty();
        const bool pre_decode_hold = state.decode.dec_valids[0];
        const bool pre_dispatch_hold = state.rename.dispatch_packets[0].valid;
        const unsigned pre_rob_count = rob_count(state);
        const uint8_t pre_tail = state.rob.tail;
        const bool pre_commit = state.rob.commit_valid;
        boom_core_step(state, pipe);

        bool request_fire = !pipe.imem_req.empty();
        ImemRequest request;
        if (request_fire) request = pipe.imem_req.read();
        const bool response_fire = response_valid && pipe.imem_resp.empty();
        const bool accepted_response = response_fire &&
            (state.frontend.fetch_packet_valid || state.frontend.response_received ||
             state.rename.dispatch_packets[0].valid || state.decode.dec_valids[0]);
        const bool rename_fire = !pre_dispatch_hold && state.rename.dispatch_packets[0].valid;
        const bool decode_fire = rename_fire || (!pre_decode_hold && state.decode.dec_valids[0]);
        const bool allocation = state.rob.tail != pre_tail || rob_count(state) > pre_rob_count;
        const bool stale = response_fire && !accepted_response;
        const bool id_match = request_fire && injected.fetch_id == request.fetch_id;
        const bool epoch_match = request_fire && response_epoch(injected) == request_epoch(request);
        const bool addr_match = request_fire && injected.address == request.address;

        out << cycle << ',' << state.frontend.pc << ',' << request_fire << ",1," << request_fire
            << ',' << (request_fire ? request.address : 0) << ','
            << (request_fire ? request.fetch_id : 0) << ','
            << (request_fire ? request_epoch(request) : frontend_epoch(state)) << ','
            << state.frontend.request_sent << ',' << response_valid << ",1," << response_fire
            << ',' << id_match << ',' << epoch_match << ',' << addr_match << ','
            << state.frontend.fetch_packet_valid << ','
            << (!state.rename.dispatch_packets[0].valid && !state.decode.dec_valids[0]) << ','
            << decode_fire << ',' << rename_fire << ",0," << allocation << ','
            << rob_count(state) << ',' << unsigned(state.rob.head) << ',' << unsigned(state.rob.tail)
            << ',' << (pre_commit || state.rob.commit_valid) << ",0," << state.global_flush << ','
            << stale << ',' << no_request_reason(state, request_fire) << '\n';
    }
}

static void run_frontend_stream(const std::string& path) {
    BoomCoreState state;
    PipeSignals pipe;
    std::ofstream out(path.c_str());
    write_header(out);
    for (unsigned cycle = 0; cycle < 128; ++cycle) {
        bool response_valid = false;
        ImemResponse response;
        if (!pipe.imem_req.empty()) {
            ImemRequest prior = pipe.imem_req.read();
            response = response_for(prior);
            pipe.imem_resp.write(response);
            response_valid = true;
        }
        const bool pre_decode = state.decode.dec_valids[0];
        const bool pre_dispatch = state.rename.dispatch_packets[0].valid;
        const unsigned pre_rob_count = rob_count(state);
        const uint8_t pre_tail = state.rob.tail;
        const bool pre_commit = state.rob.commit_valid;
        boom_core_step(state, pipe);

        bool request_fire = !pipe.imem_req.empty();
        ImemRequest request;
        if (request_fire) request = pipe.imem_req.read();
        if (request_fire) pipe.imem_req.write(request);
        const bool response_fire = response_valid && pipe.imem_resp.empty();
        const bool rename_fire = !pre_dispatch && state.rename.dispatch_packets[0].valid;
        const bool decode_fire = rename_fire || (!pre_decode && state.decode.dec_valids[0]);
        const bool allocation = state.rob.tail != pre_tail || rob_count(state) > pre_rob_count;
        const bool accepted = response_fire && !state.frontend.request_sent;
        const bool id_match = response_valid && response.fetch_id == state.frontend.pending_fetch_id;
#ifdef GATE5_1_R1_LEGACY_FRONTEND
        const bool epoch_match = response_valid;
        const bool addr_match = response_valid;
#else
        const bool epoch_match = response_valid && response.epoch == state.frontend.pending_epoch;
        const bool addr_match = response_valid && response.address == state.frontend.pending_address;
#endif

        out << cycle << ',' << state.frontend.pc << ',' << request_fire << ",1," << request_fire
            << ',' << (request_fire ? request.address : 0) << ','
            << (request_fire ? request.fetch_id : 0) << ','
            << (request_fire ? request_epoch(request) : frontend_epoch(state)) << ','
            << state.frontend.request_sent << ',' << response_valid << ",1," << response_fire
            << ',' << id_match << ',' << epoch_match << ',' << addr_match << ','
            << state.frontend.fetch_packet_valid << ','
            << (!state.rename.dispatch_packets[0].valid && !state.decode.dec_valids[0]) << ','
            << decode_fire << ',' << rename_fire << ',' << allocation << ',' << allocation << ','
            << rob_count(state) << ',' << unsigned(state.rob.head) << ',' << unsigned(state.rob.tail)
            << ',' << (pre_commit || state.rob.commit_valid) << ",0," << state.global_flush << ','
            << (response_fire && !accepted) << ',' << no_request_reason(state, request_fire) << '\n';
    }
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: gate5_1_r1_rob_fill_trace ROB_TRACE FRONTEND_TRACE\n";
        return 2;
    }
    run_rob_full_fixture(argv[1]);
    run_frontend_stream(argv[2]);
    return 0;
}
