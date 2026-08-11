#include "boom_state.hpp"
#include "boom_interfaces.hpp"
#include <cstdint>
#include <iostream>

namespace boom {
void frontend_module(BoomCoreState&, PipeSignals&);
void decode_module(BoomCoreState&);
}

static int failures = 0;

#define CHECK(condition, message) do { \
    if (!(condition)) { \
        std::cerr << "FAIL: " << message << " (line " << __LINE__ << ")\n"; \
        failures++; \
    } \
} while (0)

static ImemRequest issue_initial(BoomCoreState& state, PipeSignals& pipe) {
    boom::frontend_module(state, pipe);
    CHECK(!pipe.imem_req.empty(), "initial request missing");
    return pipe.imem_req.read();
}

static ImemResponse response_for(const ImemRequest& request, uint32_t instruction) {
    ImemResponse response;
    response.address = request.address;
    response.fetch_id = request.fetch_id;
    response.epoch = request.epoch;
    response.instruction = instruction;
    return response;
}

static void test_sequential_and_identity_match() {
    BoomCoreState state;
    PipeSignals pipe;
    const ImemRequest request = issue_initial(state, pipe);
    CHECK(request.address == RESET_VECTOR, "reset vector request address");
    CHECK(request.fetch_id == 0 && request.epoch == 0, "initial request identity");

    ImemResponse wrong_id = response_for(request, 0x00100093u);
    wrong_id.fetch_id++;
    pipe.imem_resp.write(wrong_id);
    boom::frontend_module(state, pipe);
    CHECK(state.frontend.request_sent, "wrong ID consumed pending request");
    CHECK(!state.frontend.fetch_packet_valid, "wrong ID produced packet");

    ImemResponse wrong_epoch = response_for(request, 0x00100093u);
    wrong_epoch.epoch++;
    pipe.imem_resp.write(wrong_epoch);
    boom::frontend_module(state, pipe);
    CHECK(state.frontend.request_sent, "wrong epoch consumed pending request");

    ImemResponse wrong_address = response_for(request, 0x00100093u);
    wrong_address.address += 4;
    pipe.imem_resp.write(wrong_address);
    boom::frontend_module(state, pipe);
    CHECK(state.frontend.request_sent, "wrong address consumed pending request");

    pipe.imem_resp.write(response_for(request, 0x00100093u));
    boom::frontend_module(state, pipe);
    CHECK(state.frontend.fetch_packet_valid, "matching response not delivered");
    CHECK(state.frontend.fetch_uop.debug_pc == RESET_VECTOR, "response PC changed");
    CHECK(state.frontend.pc == RESET_VECTOR + 4, "sequential PC did not advance by four");

    boom::decode_module(state);
    CHECK(state.decode.dec_valids[0], "one-entry handoff did not reach decode");
    CHECK(state.decode.dec_uops[0].inst == 0x00100093u, "decode instruction mismatch");
}

static void test_request_backpressure() {
    BoomCoreState state;
    PipeSignals pipe;
    for (unsigned i = 0; i < 1024; ++i) pipe.imem_req.write(ImemRequest());

    boom::frontend_module(state, pipe);
    CHECK(!state.frontend.request_sent, "full request stream accepted a request");
    CHECK(state.frontend.fetch_id == 0, "backpressure advanced request identity");
    CHECK(state.frontend.pc == RESET_VECTOR, "backpressure changed request address");

    pipe.imem_req.read();
    boom::frontend_module(state, pipe);
    CHECK(state.frontend.request_sent, "request did not retry after backpressure");
    for (unsigned i = 0; i < 1023; ++i) pipe.imem_req.read();
    const ImemRequest request = pipe.imem_req.read();
    CHECK(request.address == RESET_VECTOR && request.fetch_id == 0 && request.epoch == 0,
          "retried request payload changed");
}

static void test_response_drain_while_stalled() {
    BoomCoreState state;
    PipeSignals pipe;
    const ImemRequest request = issue_initial(state, pipe);
    state.decode.dec_valids[0] = true;
    pipe.imem_resp.write(response_for(request, 0x00200113u));
    boom::frontend_module(state, pipe);
    CHECK(pipe.imem_resp.empty(), "stalled frontend did not drain response");
    CHECK(state.frontend.response_received, "stalled frontend did not retain response");
    CHECK(!state.frontend.fetch_packet_valid, "stalled frontend exposed response early");

    state.decode.dec_valids[0] = false;
    boom::frontend_module(state, pipe);
    CHECK(state.frontend.fetch_packet_valid, "retained response not delivered after stall");
    CHECK(state.frontend.fetch_uop.inst == 0x00200113u, "retained instruction mismatch");
}

static void test_redirect_wins_response() {
    BoomCoreState state;
    PipeSignals pipe;
    const ImemRequest old_request = issue_initial(state, pipe);
    pipe.imem_resp.write(response_for(old_request, 0x00300193u));
    state.brupdate.valid = true;
    state.brupdate.mispredict = true;
    state.brupdate.jalr_target = 0x20000;
    boom::frontend_module(state, pipe);

    CHECK(pipe.imem_resp.empty(), "redirect cycle did not drain old response");
    CHECK(!state.frontend.fetch_packet_valid, "old response beat redirect");
    CHECK(!pipe.imem_req.empty(), "redirect target request missing");
    const ImemRequest redirected = pipe.imem_req.read();
    CHECK(redirected.address == 0x20000, "redirect target changed");
    CHECK(redirected.epoch == old_request.epoch + 1, "redirect did not advance epoch");

    state.brupdate.valid = false;
    state.brupdate.mispredict = false;
    ImemResponse stale = response_for(old_request, 0x00400213u);
    stale.address = redirected.address;
    stale.fetch_id = redirected.fetch_id;
    pipe.imem_resp.write(stale);
    boom::frontend_module(state, pipe);
    CHECK(state.frontend.request_sent, "old epoch matched redirected request");

    pipe.imem_resp.write(response_for(redirected, 0x00500293u));
    boom::frontend_module(state, pipe);
    CHECK(state.frontend.fetch_packet_valid, "redirect response not delivered");
    CHECK(state.frontend.fetch_uop.debug_pc == 0x20000, "redirect response PC mismatch");
}

static void test_repeated_redirect_latest_wins() {
    BoomCoreState state;
    PipeSignals pipe;
    const ImemRequest initial = issue_initial(state, pipe);
    state.brupdate.valid = state.brupdate.mispredict = true;
    state.brupdate.jalr_target = 0x21000;
    boom::frontend_module(state, pipe);
    const ImemRequest first = pipe.imem_req.read();

    pipe.imem_resp.write(response_for(first, 0x00600313u));
    state.brupdate.jalr_target = 0x22000;
    boom::frontend_module(state, pipe);
    const ImemRequest second = pipe.imem_req.read();
    CHECK(second.address == 0x22000, "latest redirect target lost");
    CHECK(second.epoch == initial.epoch + 2, "repeated redirect epoch count");
    CHECK(!state.frontend.fetch_packet_valid, "first redirect response leaked");
}

static void test_architectural_redirect_priority() {
    BoomCoreState state;
    PipeSignals pipe;
    const ImemRequest old_request = issue_initial(state, pipe);
    pipe.imem_resp.write(response_for(old_request, 0x00800413u));
    state.brupdate.valid = state.brupdate.mispredict = true;
    state.brupdate.jalr_target = 0x24000;
    state.frontend_redirect.valid = true;
    state.frontend_redirect.target_pc = 0x25000;
    state.frontend_redirect.cause = FRONTEND_REDIRECT_EXCEPTION;
    state.frontend_redirect.rob_idx = 3;
    state.frontend_redirect.allocation_id = 77;
    state.rob.entries[3].valid = true;
    state.rob.entries[3].uop.queue.rob_allocation_id = 77;
    boom::frontend_module(state, pipe);

    CHECK(!pipe.imem_req.empty(), "architectural redirect request missing");
    const ImemRequest request = pipe.imem_req.read();
    CHECK(request.address == 0x25000, "branch redirect beat architectural redirect");
    CHECK(!state.frontend_redirect.valid, "architectural redirect was not consumed");
    CHECK(!state.frontend.fetch_packet_valid, "response beat architectural redirect");
}

static void test_reset_rejects_old_id_zero() {
    BoomCoreState state;
    PipeSignals pipe;
    const ImemRequest old_request = issue_initial(state, pipe);
    state.frontend.reset_done = false;
    pipe.imem_resp.write(response_for(old_request, 0x00700393u));
    boom::frontend_module(state, pipe);
    CHECK(!state.frontend.fetch_packet_valid, "pre-reset ID-zero response was accepted");
    CHECK(!pipe.imem_req.empty(), "post-reset request missing");
    const ImemRequest reset_request = pipe.imem_req.read();
    CHECK(reset_request.fetch_id == 0, "reset did not restart fetch ID");
    CHECK(reset_request.epoch == old_request.epoch + 1, "reset did not advance epoch");
}

static void test_fetch_fault_propagation() {
    BoomCoreState state;
    PipeSignals pipe;
    const ImemRequest request = issue_initial(state, pipe);
    ImemResponse fault = response_for(request, 0);
    fault.exception = true;
    fault.exc_cause = 1;
    pipe.imem_resp.write(fault);
    boom::frontend_module(state, pipe);
    CHECK(state.frontend.fetch_uop.exception, "fetch fault missing from holding entry");

    boom::decode_module(state);
    CHECK(state.decode.dec_valids[0], "fetch fault did not reach decode");
    CHECK(state.decode.dec_uops[0].exception, "decode dropped fetch fault");
    CHECK(state.decode.dec_uops[0].exc_cause == 1, "decode changed fetch fault cause");
}

static void test_misaligned_redirect_fault() {
    BoomCoreState state;
    PipeSignals pipe;
    issue_initial(state, pipe);
    state.brupdate.valid = state.brupdate.mispredict = true;
    state.brupdate.jalr_target = 0x23002;
    boom::frontend_module(state, pipe);
    CHECK(pipe.imem_req.empty(), "misaligned target was requested from IMEM");
    CHECK(state.frontend.fetch_packet_valid, "misaligned target did not create fault");
    CHECK(state.frontend.fetch_uop.exception, "misaligned target fault bit missing");
    CHECK(state.frontend.fetch_uop.exc_cause == 0, "misaligned target cause mismatch");
    CHECK(state.frontend.fetch_uop.debug_pc == 0x23002, "misaligned target was masked");
}

int main() {
    test_sequential_and_identity_match();
    test_request_backpressure();
    test_response_drain_while_stalled();
    test_redirect_wins_response();
    test_repeated_redirect_latest_wins();
    test_architectural_redirect_priority();
    test_reset_rejects_old_id_zero();
    test_fetch_fault_propagation();
    test_misaligned_redirect_fault();

    if (failures != 0) {
        std::cerr << failures << " frontend foundation checks failed\n";
        return 1;
    }
    std::cout << "PASS: frontend foundation checks\n";
    return 0;
}
