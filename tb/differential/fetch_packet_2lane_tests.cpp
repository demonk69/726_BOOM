#include "boom_interfaces.hpp"
#include "boom_state.hpp"
#include "fetch_packet.hpp"
#include "rvc.hpp"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>

namespace boom {
void frontend_module(BoomCoreState&, PipeSignals&);
void decode_module(BoomCoreState&);
}

namespace {

uint64_t checks = 0;
uint64_t failures = 0;

void check(bool condition, const std::string& message) {
    ++checks;
    if (!condition) {
        ++failures;
        if (failures <= 30) std::cerr << "FAIL[" << checks << "]: " << message << '\n';
    }
}

boom::FetchPacketResult build(uint64_t pc, uint32_t word,
                              bool carry_valid = false, uint16_t carry = 0,
                              uint64_t carry_pc = 0, bool fault = false,
                              uint64_t cause = 0) {
    boom::FetchPacketInput input;
    input.pc = pc;
    input.instruction = word;
    input.fetch_id = 17;
    input.exception = fault;
    input.exception_cause = cause;
    input.carry_valid = carry_valid;
    input.carry = carry;
    input.carry_pc = carry_pc;
    return boom::build_fetch_packet(input);
}

ImemResponse response(uint64_t address, uint32_t id, uint32_t epoch,
                      uint32_t word, bool fault = false, uint64_t cause = 0) {
    ImemResponse r;
    r.address = address;
    r.fetch_id = id;
    r.epoch = epoch;
    r.instruction = word;
    r.exception = fault;
    r.exc_cause = cause;
    return r;
}

void native_call(BoomCoreState& state, PipeSignals& pipe, bool decode_ready) {
    if (decode_ready) {
        state.decode.dec_valids[0] = false;
        state.rename.dispatch_packets[0] = RenameDispatchPacket();
    } else {
        state.rename.dispatch_packets[0].valid = true;
    }
    boom::frontend_module(state, pipe);
    boom::decode_module(state);
}

boom::FetchInstruction token(uint64_t pc, uint32_t value) {
    boom::FetchInstruction entry;
    entry.pc = pc;
    entry.instruction = value;
    entry.original_instruction = value;
    entry.fetch_id = static_cast<uint32_t>(pc >> 2);
    return entry;
}

boom::FetchPacket two_lane_packet(uint64_t pc) {
    boom::FetchPacket packet;
    packet.valid = true;
    packet.valid_mask = 3;
    packet.slots[0] = token(pc, 0x00100093u);
    packet.slots[1] = token(pc + 2, 0x00000013u);
    packet.slots[1].is_rvc = true;
    packet.slots[1].original_instruction = 0x0001;
    return packet;
}

bool same_entry(const boom::FetchInstruction& a, const boom::FetchInstruction& b) {
    return a.pc == b.pc && a.instruction == b.instruction &&
        a.original_instruction == b.original_instruction &&
        a.fetch_id == b.fetch_id && a.exception_cause == b.exception_cause &&
        a.is_rvc == b.is_rvc && a.exception == b.exception &&
        a.exception_access_fault == b.exception_access_fault &&
        a.exception_misaligned == b.exception_misaligned;
}

void check_masks_faults_and_capacity() {
    const uint16_t c_nop = 0x0001;
    const uint16_t c_addi = 0x0085;
    boom::FetchPacketResult result = build(0x1000, c_nop | (uint32_t(c_addi) << 16));
    check(result.packet.valid && result.packet.valid_mask == 3, "C+C mask 11");
    check(result.packet.slots[0].pc == 0x1000, "C+C lane0 PC");
    check(result.packet.slots[1].pc == 0x1002, "C+C lane1 PC");
    check(result.next_pc == 0x1004 && !result.carry_valid, "C+C continuation");

    result = build(0x1100, 0x00108093u);
    check(result.packet.valid_mask == 1, "aligned 32-bit mask 01");
    check(result.packet.slots[0].instruction == 0x00108093u, "aligned 32-bit data");
    check(result.next_pc == 0x1104 && !result.carry_valid, "aligned continuation");

    result = build(0x1200, c_nop | (uint32_t(0x0093) << 16));
    check(result.packet.valid_mask == 1, "C+32 mask 01");
    check(result.carry_valid && result.carry == 0x0093 && result.carry_pc == 0x1202,
          "C+32 carry");
    result = build(0x1302, uint32_t(0x0093) << 16);
    check(!result.packet.valid && result.packet.valid_mask == 0, "upper 32 mask 00");
    check(result.carry_valid && result.carry_pc == 0x1302, "upper carry");

    result = build(0x1404, uint32_t(c_addi) << 16 | 0x0010u,
                   true, 0x0093, 0x1402);
    check(result.packet.valid_mask == 3, "carry+C mask 11");
    check(result.packet.slots[0].pc == 0x1402 &&
          result.packet.slots[0].instruction == 0x00100093u, "carry completion lane0");
    check(result.packet.slots[1].pc == 0x1406 && result.packet.slots[1].is_rvc,
          "carry+C lane1");
    result = build(0x1504, uint32_t(0x0093) << 16 | 0x0010u,
                   true, 0x0093, 0x1502);
    check(result.packet.valid_mask == 1 && result.carry_valid, "carry+32 replacement");

    result = build(0x1600, 0, false, 0, 0, true, 12);
    check(result.packet.valid_mask == 1 && result.packet.slots[0].exception,
          "aligned access fault");
    check(result.packet.slots[0].pc == 0x1600 &&
          result.packet.slots[0].exception_cause == 12 &&
          result.packet.slots[0].exception_access_fault, "access fault metadata");
    result = build(0x1704, 0, true, 0x0093, 0x1702, true, 13);
    check(result.packet.valid_mask == 1 && result.packet.slots[0].pc == 0x1702,
          "faulted carry attribution");
    check(!result.carry_valid, "faulted carry termination");

    result = build(0x1800, c_nop | (uint32_t(0x0000) << 16));
    check(result.packet.valid_mask == 3 && result.packet.slots[1].exception,
          "illegal compressed upper lane");
    check(result.packet.slots[1].original_instruction == 0x0000, "illegal upper bits");
    result = build(0x1900, 0x00010000u);
    check(result.packet.valid_mask == 1 && result.packet.slots[0].exception,
          "illegal compressed lower terminal");
    result = build(0x1a00, c_nop | (uint32_t(0x001f) << 16));
    check(result.packet.valid_mask == 3 && result.packet.slots[1].exception,
          "long upper terminal");
    result = build(0x1b00, 0x0001001fu);
    check(result.packet.valid_mask == 1 && result.packet.slots[0].exception &&
          !result.carry_valid, "long lower terminal");

    for (uint32_t i = 0; i < 96; ++i) {
        const uint32_t word = (i % 3 == 0) ? 0x00010001u :
            ((i % 3 == 1) ? 0x00108093u : 0x00930001u);
        result = build(0x2000 + i * 4, word);
        check(result.packet.valid_mask == 0 || result.packet.valid_mask == 1 ||
              result.packet.valid_mask == 3, "mask is only 00/01/11");
        check(result.packet.valid_mask != 2, "mask 10 prohibited");
    }
}

void check_atomic_backpressure_and_stability() {
    BoomCoreState state;
    PipeSignals pipe;
    state.frontend.reset_done = true;
    state.frontend.request_sent = true;
    state.frontend.fetch_buffer.count = FETCH_BUFFER_DEPTH - 1;
    state.frontend.fetch_buffer.head = 0;
    state.frontend.fetch_buffer.tail = FETCH_BUFFER_DEPTH - 1;
    for (uint8_t i = 0; i < FETCH_BUFFER_DEPTH - 1; ++i)
        state.frontend.fetch_buffer.entries[i] = token(0x3000 + i * 4, 0x00000013u);
    state.frontend.pending_packet = two_lane_packet(0x4000);
    state.frontend.producer_valid = true;
    const boom::FetchPacket held = state.frontend.pending_packet;

    for (unsigned i = 0; i < 5; ++i) {
        native_call(state, pipe, false);
        check(state.frontend.fetch_buffer.count == FETCH_BUFFER_DEPTH - 1,
              "two-lane packet partially admitted with one free slot");
        check(state.frontend.pending_packet.valid && state.frontend.stalled,
              "packet/mirror stall validity");
        check(state.frontend.pending_packet.valid_mask == 3 &&
              same_entry(state.frontend.pending_packet.slots[0], held.slots[0]) &&
              same_entry(state.frontend.pending_packet.slots[1], held.slots[1]),
              "stalled packet stability");
    }

    native_call(state, pipe, true);
    check(state.decode.dec_valids[0] && state.decode.dec_uops[0].debug_pc == 0x3000,
          "same-cycle dequeue missing");
    check(state.frontend.fetch_buffer.count == FETCH_BUFFER_DEPTH,
          "dequeue capacity did not admit atomic pair");
    check(!state.frontend.pending_packet.valid && !state.frontend.producer_valid &&
          !state.frontend.stalled, "accepted packet mirrors not cleared");
    const uint8_t p0 = static_cast<uint8_t>((state.frontend.fetch_buffer.tail +
                                             FETCH_BUFFER_DEPTH - 2) &
                                            (FETCH_BUFFER_DEPTH - 1));
    const uint8_t p1 = static_cast<uint8_t>((p0 + 1) & (FETCH_BUFFER_DEPTH - 1));
    check(state.frontend.fetch_buffer.entries[p0].pc == 0x4000 &&
          state.frontend.fetch_buffer.entries[p1].pc == 0x4002,
          "atomic packet lane order");
}

void setup_owned(BoomCoreState& state, uint64_t pc, uint32_t id = 7,
                 uint32_t epoch = 9) {
    state.frontend.reset_done = true;
    state.frontend.pc = pc;
    state.frontend.request_sent = true;
    state.frontend.pending_address = pc & ~3ULL;
    state.frontend.pending_fetch_id = id;
    state.frontend.pending_epoch = epoch;
    state.frontend.epoch = epoch;
}

void integrated_terminal(uint32_t word, uint8_t mask, unsigned lane,
                         const std::string& label) {
    BoomCoreState state;
    PipeSignals pipe;
    setup_owned(state, 0x5a00);
    pipe.imem_resp.write(response(0x5a00, 7, 9, word));
    native_call(state, pipe, false);
    check(state.frontend.pending_packet.valid_mask == mask, label + " mask");
    check(state.frontend.pending_packet.slots[lane].exception &&
          state.frontend.pending_packet.slots[lane].exception_cause == 2,
          label + " exception");
    check(!state.frontend.halfword_valid, label + " carry termination");
}

void check_response_ownership_faults_and_no_aggregation() {
    BoomCoreState state;
    PipeSignals pipe;
    setup_owned(state, 0x5000);
    pipe.imem_resp.write(response(0x5000, 8, 9, 0x00010001u));
    native_call(state, pipe, true);
    check(!state.frontend.pending_packet.valid && state.frontend.request_sent,
          "stale id response had side effect");
    pipe.imem_resp.write(response(0x5004, 7, 9, 0x00010001u));
    native_call(state, pipe, true);
    check(!state.frontend.pending_packet.valid, "stale address response had side effect");
    pipe.imem_resp.write(response(0x5000, 7, 10, 0x00010001u));
    native_call(state, pipe, true);
    check(!state.frontend.pending_packet.valid, "stale epoch response had side effect");
    pipe.imem_resp.write(response(0x5000, 7, 9, 0x00010001u));
    native_call(state, pipe, true);
    check(state.frontend.pending_packet.valid &&
          state.frontend.pending_packet.valid_mask == 3, "owned C+C response packet");
    check(state.frontend.fetch_buffer.count == 0,
          "response packet bypassed canonical buffer");
    native_call(state, pipe, false);
    check(state.frontend.fetch_buffer.count == 2,
          "single response did not enqueue exactly two entries");
    check(!state.frontend.pending_packet.valid,
          "multi-response aggregation retained accepted packet");

    integrated_terminal(0x00010000u, 1, 0, "integrated illegal compressed lower");
    integrated_terminal(0x00000001u, 3, 1, "integrated illegal compressed upper");
    integrated_terminal(0x0001001fu, 1, 0, "integrated long lower");
    integrated_terminal(0x001f0001u, 3, 1, "integrated long upper");

    BoomCoreState fault;
    PipeSignals fault_pipe;
    setup_owned(fault, 0x6000);
    fault_pipe.imem_resp.write(response(0x6000, 7, 9, 0, true, 12));
    native_call(fault, fault_pipe, false);
    check(fault.frontend.pending_packet.valid_mask == 1 &&
          fault.frontend.pending_packet.slots[0].exception &&
          fault.frontend.pending_packet.slots[0].exception_access_fault,
          "integrated access fault packet");

    BoomCoreState carry_fault;
    PipeSignals carry_pipe;
    setup_owned(carry_fault, 0x6104);
    carry_fault.frontend.halfword_valid = true;
    carry_fault.frontend.halfword = 0x0093;
    carry_fault.frontend.halfword_pc = 0x6102;
    carry_fault.frontend.halfword_epoch = 9;
    carry_pipe.imem_resp.write(response(0x6104, 7, 9, 0, true, 13));
    native_call(carry_fault, carry_pipe, false);
    check(carry_fault.frontend.pending_packet.valid_mask == 1 &&
          carry_fault.frontend.pending_packet.slots[0].pc == 0x6102 &&
          !carry_fault.frontend.halfword_valid, "integrated faulted carry");

    BoomCoreState odd;
    PipeSignals odd_pipe;
    odd.frontend.reset_done = true;
    odd.frontend_redirect.valid = true;
    odd.frontend_redirect.cause = FRONTEND_REDIRECT_DEBUG;
    odd.frontend_redirect.target_pc = 0x6201;
    native_call(odd, odd_pipe, false);
    check(odd.frontend.pending_packet.valid_mask == 1 &&
          odd.frontend.pending_packet.slots[0].exception &&
          odd.frontend.pending_packet.slots[0].exception_misaligned,
          "misaligned redirect fault packet");
}

void seed_kill_state(BoomCoreState& state) {
    state.frontend.reset_done = true;
    state.frontend.pc = 0x7000;
    state.frontend.request_sent = true;
    state.frontend.response_received = true;
    state.frontend.halfword_valid = true;
    state.frontend.pending_packet = two_lane_packet(0x7100);
    state.frontend.producer_valid = true;
    state.frontend.stalled = true;
    state.frontend.fetch_buffer.count = 2;
}

void check_killed(const BoomCoreState& state, const std::string& kind) {
    check(state.frontend.fetch_buffer.count == 0, kind + " buffer kill");
    check(!state.frontend.pending_packet.valid && !state.frontend.producer_valid,
          kind + " packet kill");
    check(!state.frontend.halfword_valid && !state.frontend.response_received,
          kind + " carry/response kill");
}

void check_all_kills() {
    BoomCoreState reset;
    PipeSignals reset_pipe;
    seed_kill_state(reset);
    reset.frontend.reset_done = false;
    native_call(reset, reset_pipe, true);
    check_killed(reset, "reset");

    BoomCoreState branch;
    PipeSignals branch_pipe;
    seed_kill_state(branch);
    branch.brupdate.valid = true;
    branch.brupdate.mispredict = true;
    branch.brupdate.jalr_target = 0x7200;
    native_call(branch, branch_pipe, true);
    check_killed(branch, "branch redirect");

    BoomCoreState redirect;
    PipeSignals redirect_pipe;
    seed_kill_state(redirect);
    redirect.frontend_redirect.valid = true;
    redirect.frontend_redirect.cause = FRONTEND_REDIRECT_DEBUG;
    redirect.frontend_redirect.target_pc = 0x7300;
    native_call(redirect, redirect_pipe, true);
    check_killed(redirect, "architectural redirect");

    BoomCoreState flush;
    PipeSignals flush_pipe;
    seed_kill_state(flush);
    flush.global_flush = true;
    native_call(flush, flush_pipe, true);
    check_killed(flush, "global flush");

    BoomCoreState exception;
    PipeSignals exception_pipe;
    seed_kill_state(exception);
    exception.rob.state = ROB_EXCEPTION;
    native_call(exception, exception_pipe, true);
    check_killed(exception, "ROB_EXCEPTION");
    check(!exception.frontend.request_sent && !exception.frontend.stalled,
          "ROB_EXCEPTION request/stall kill");
}

void check_all_legal_rvc_in_both_lanes() {
    uint32_t legal = 0;
    for (uint32_t raw = 0; raw <= 0xffffu; ++raw) {
        const boom::RvcDecodeResult decoded = boom::decompress_rvc(uint16_t(raw));
        if (!decoded.valid || !decoded.legal) continue;
        ++legal;
        const boom::FetchPacketResult result = build(0x80000000ull, raw | (raw << 16));
        check(result.packet.valid && result.packet.valid_mask == 3,
              "legal RVC two-lane packet");
        check(result.packet.slots[0].pc == 0x80000000ull &&
              result.packet.slots[1].pc == 0x80000002ull, "legal RVC lane PCs");
        check(result.packet.slots[0].instruction == decoded.instruction &&
              result.packet.slots[1].instruction == decoded.instruction,
              "legal RVC lane expansion");
        check(result.packet.slots[0].original_instruction == raw &&
              result.packet.slots[1].original_instruction == raw,
              "legal RVC original parcels");
        check(result.packet.slots[0].is_rvc && result.packet.slots[1].is_rvc &&
              !result.packet.slots[0].exception && !result.packet.slots[1].exception,
              "legal RVC attribution");
    }
    check(legal == 38551, "legal RVC inventory");
}

}  // namespace

int main() {
    check_masks_faults_and_capacity();
    check_atomic_backpressure_and_stability();
    check_response_ownership_faults_and_no_aggregation();
    check_all_kills();
    check_all_legal_rvc_in_both_lanes();
    check(checks >= 200, "directed suite check floor");
    std::cout << "GATE5_3_B3I_FETCH_PACKET_2LANE_"
              << (failures ? "FAIL" : "PASS") << " checks=" << checks
              << " legal_rvc_each_lane=38551 failures=" << failures << '\n';
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
