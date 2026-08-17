#include "boom_interfaces.hpp"
#include "boom_state.hpp"
#include "rvc.hpp"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace boom {
void frontend_module(BoomCoreState&, PipeSignals&);
void decode_module(BoomCoreState&);
}

namespace {

unsigned checks = 0;
unsigned failures = 0;

void check(bool condition, const std::string& message) {
    ++checks;
    if (!condition) {
        ++failures;
        std::cerr << "FAIL[" << checks << "]: " << message << '\n';
    }
}

void semantic_check(bool condition, const char* semantic, const std::string& message) {
    check(condition, message);
    if (condition) std::cout << "SEMANTIC_PASS," << semantic << '\n';
}

ImemResponse response(const ImemRequest& request, uint32_t instruction,
                      bool fault = false, uint64_t cause = 0) {
    ImemResponse value;
    value.address = request.address;
    value.fetch_id = request.fetch_id;
    value.epoch = request.epoch;
    value.instruction = instruction;
    value.exception = fault;
    value.exc_cause = cause;
    return value;
}

void consume_decode(BoomCoreState& state) {
    state.decode.dec_valids[0] = false;
    state.rename.dispatch_packets[0] = RenameDispatchPacket();
}

void cycle(BoomCoreState& state, PipeSignals& pipe, bool ready) {
    if (ready) {
        consume_decode(state);
    } else {
        state.rename.dispatch_packets[0].valid = true;
    }
    boom::frontend_module(state, pipe);
    boom::decode_module(state);
}

void redirect(BoomCoreState& state, uint64_t pc) {
    state.brupdate.valid = true;
    state.brupdate.mispredict = true;
    state.brupdate.jalr_target = pc;
}

void clear_redirect(BoomCoreState& state) {
    state.brupdate = BranchUpdate();
}

void test_single_entry_fifo_and_stalls() {
    BoomCoreState state;
    PipeSignals pipe;
    cycle(state, pipe, true);
    check(!pipe.imem_req.empty(), "initial request missing");
    ImemRequest stale_request = pipe.imem_req.read();

    redirect(state, 0x10000);
    cycle(state, pipe, true);
    clear_redirect(state);
    check(state.frontend.fetch_buffer.count == 0, "redirect did not flush buffer");
    check(!pipe.imem_req.empty(), "redirect request missing");
    ImemRequest request0 = pipe.imem_req.read();
    pipe.imem_resp.write(response(request0, 0x00108093u));
    cycle(state, pipe, false);
    check(state.frontend.producer_valid, "complete base instruction not held");
    const MicroOp held = state.frontend.producer_uop;
    check(held.debug_pc == 0x10000 && held.inst == 0x00108093u,
          "held base metadata mismatch");

    cycle(state, pipe, false);
    check(!state.frontend.producer_valid, "accepted producer was not released");
    check(state.frontend.fetch_buffer.count == 1, "lane0 enqueue missing");
    check(!state.frontend.fetch_packet_valid, "empty-FIFO bypass was introduced");
    cycle(state, pipe, false);
    check(state.frontend.fetch_packet_valid, "FIFO head not visible during Decode stall");
    check(state.frontend.fetch_uop.debug_pc == held.debug_pc, "FIFO head PC mismatch");
    for (unsigned i = 0; i < 24; ++i) {
        cycle(state, pipe, false);
        check(state.frontend.fetch_packet_valid, "repeated stall lost FIFO head");
        check(state.frontend.fetch_uop.inst == held.inst, "stalled instruction changed");
        check(state.frontend.fetch_buffer.count == 1, "stall changed occupancy");
    }
    cycle(state, pipe, true);
    check(state.decode.dec_valids[0], "ready Decode did not consume FIFO head");
    check(state.decode.dec_uops[0].debug_pc == 0x10000, "Decode PC mismatch");
    check(state.frontend.fetch_buffer.count == 0, "pop did not remove one entry");

    pipe.imem_resp.write(response(stale_request, 0x00210113u));
    cycle(state, pipe, true);
    check(!state.frontend.producer_valid, "stale response produced an entry");
}

void seed_producer(BoomCoreState& state, uint64_t pc, uint32_t id,
                   bool rvc = false, bool fault = false, uint64_t cause = 0) {
    state.frontend.producer_valid = true;
    state.frontend.producer_fetch_id = id;
    state.frontend.producer_uop = MicroOp();
    state.frontend.producer_uop.debug_pc = pc;
    state.frontend.producer_uop.inst = 0x00000013u | (id << 20);
    state.frontend.producer_uop.debug_inst = rvc ? 0x0001u : state.frontend.producer_uop.inst;
    state.frontend.producer_uop.is_rvc = rvc;
    state.frontend.producer_uop.exception = fault;
    state.frontend.producer_uop.exc_cause = cause;
    state.frontend.producer_uop.exc.exception = fault;
    state.frontend.producer_uop.exc.exc_cause = cause;
}

void test_full_wrap_order_and_flush() {
    BoomCoreState state;
    PipeSignals pipe;
    state.frontend.reset_done = true;
    state.frontend.pc = 0x20000;
    state.frontend.request_sent = true;

    for (uint32_t id = 0; id < FETCH_BUFFER_DEPTH; ++id) {
        seed_producer(state, 0x20000 + id * 4, id, (id & 1u) != 0);
        cycle(state, pipe, false);
        check(!state.frontend.producer_valid, "fill enqueue backpressured early");
        check(state.frontend.fetch_buffer.count == id + 1, "fill occupancy mismatch");
    }
    check(state.frontend.fetch_buffer.count == FETCH_BUFFER_DEPTH, "buffer not full");
    seed_producer(state, 0x30000, 99);
    const MicroOp full_held = state.frontend.producer_uop;
    for (unsigned i = 0; i < 5; ++i) {
        cycle(state, pipe, false);
        check(state.frontend.producer_valid, "full buffer consumed held entry");
        check(state.frontend.producer_uop.debug_pc == full_held.debug_pc,
              "full backpressure changed held PC");
        check(state.frontend.fetch_buffer.count == FETCH_BUFFER_DEPTH,
              "full backpressure changed occupancy");
    }

    std::set<uint64_t> seen;
    for (uint32_t id = 0; id <= FETCH_BUFFER_DEPTH; ++id) {
        cycle(state, pipe, true);
        check(state.decode.dec_valids[0], "drain Decode valid missing");
        const uint64_t pc = state.decode.dec_uops[0].debug_pc;
        check(seen.insert(pc).second, "duplicate Decode token");
        const uint64_t expected = id < FETCH_BUFFER_DEPTH ? 0x20000 + id * 4 : 0x30000;
        check(pc == expected, "FIFO order error");
    }
    check(state.frontend.fetch_buffer.count == 0, "drain did not empty buffer");

    for (uint32_t round = 0; round < 3; ++round) {
        for (uint32_t id = 0; id < FETCH_BUFFER_DEPTH; ++id) {
            seed_producer(state, 0x40000 + (round * FETCH_BUFFER_DEPTH + id) * 4,
                          200 + round * FETCH_BUFFER_DEPTH + id);
            cycle(state, pipe, false);
        }
        for (uint32_t id = 0; id < FETCH_BUFFER_DEPTH; ++id) cycle(state, pipe, true);
        check(state.frontend.fetch_buffer.count == 0, "wrap round failed to drain");
    }

    seed_producer(state, 0x50000, 500, false, true, 1);
    cycle(state, pipe, false);
    cycle(state, pipe, false);
    check(state.frontend.fetch_buffer.count == 1, "fault entry not enqueued");
    redirect(state, 0x60000);
    cycle(state, pipe, true);
    clear_redirect(state);
    check(state.frontend.fetch_buffer.count == 0, "redirect did not kill fault entry");
    check(!state.decode.dec_valids[0], "same-cycle pop escaped redirect flush");

    seed_producer(state, 0x70000, 700);
    cycle(state, pipe, false);
    state.frontend.reset_done = false;
    cycle(state, pipe, true);
    check(state.frontend.fetch_buffer.count == 0, "runtime reset did not flush buffer");
    check(!state.frontend.producer_valid, "runtime reset did not kill producer");
    check(!state.decode.dec_valids[0], "runtime reset allowed old dequeue");
}

void test_rvc_cross_word_and_fault() {
    BoomCoreState state;
    PipeSignals pipe;
    cycle(state, pipe, true);
    pipe.imem_req.read();
    redirect(state, 0x80002);
    cycle(state, pipe, true);
    clear_redirect(state);
    ImemRequest first = pipe.imem_req.read();
    pipe.imem_resp.write(response(first, 0x00010013u));
    cycle(state, pipe, false);
    semantic_check(state.frontend.producer_valid && state.frontend.producer_uop.is_rvc,
                   "upper_half_rvc_production", "upper-half RVC not produced");
    semantic_check(state.frontend.producer_uop.debug_pc == 0x80002,
                   "rvc_pc_plus_2_start", "RVC PC+2 start mismatch");
    check(state.frontend.producer_uop.debug_inst == 0x0001,
          "upper-half RVC parcel mismatch");
    check(!pipe.imem_req.empty(), "post-RVC cross-word request missing");
    ImemRequest second = pipe.imem_req.read();
    pipe.imem_resp.write(response(second, 0x00930001u));
    cycle(state, pipe, false);
    check(state.frontend.producer_valid && state.frontend.producer_uop.is_rvc,
          "lower-half setup RVC not produced");
    check(state.frontend.producer_uop.debug_pc == 0x80004,
          "lower-half setup RVC PC mismatch");
    const uint8_t completed_count = state.frontend.fetch_buffer.count;
    cycle(state, pipe, false);
    semantic_check(state.frontend.halfword_valid, "cross_word_carry_creation",
                   "cross-word carry missing");
    semantic_check(!state.frontend.producer_valid,
                   "partial_cross_word_excluded_from_fetch_buffer",
                   "partial cross-word instruction entered buffer");
    check(state.frontend.fetch_buffer.count == completed_count + 1,
          "partial cross-word instruction changed completed-entry occupancy");
    check(!pipe.imem_req.empty(), "cross-word completion request missing");
    ImemRequest third = pipe.imem_req.read();
    pipe.imem_resp.write(response(third, 0, true, 12));
    cycle(state, pipe, false);
    check(state.frontend.producer_valid, "faulted cross-word entry missing");
    semantic_check(state.frontend.producer_uop.debug_pc == 0x80006,
                   "faulted_cross_word_lower_half_start_pc",
                   "faulted cross-word instruction did not use lower-half start PC");
    check(state.frontend.producer_uop.exception &&
          state.frontend.producer_uop.exc_cause == 12,
          "faulted cross-word cause mismatch");
}

}  // namespace

int main() {
    test_single_entry_fifo_and_stalls();
    test_full_wrap_order_and_flush();
    test_rvc_cross_word_and_fault();
    check(checks + 1 >= 169, "focused suite has fewer than 169 checks");
    std::cout << "GATE5_3_B2_FETCH_BUFFER_INTEGRATION_FOCUSED_"
              << (failures ? "FAIL" : "PASS") << " checks=" << checks
              << " failures=" << failures << '\n';
    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
