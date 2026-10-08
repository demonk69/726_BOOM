#include "boom_interfaces.hpp"
#include "boom_state.hpp"
#include "rvc.hpp"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace boom {
void branch_complete_event(BoomCoreState& state, const MicroOp& uop,
                           bool mispredict, uint64_t redirect_pc);
void decode_module(BoomCoreState& state);
}

namespace boom {
void frontend_module(BoomCoreState&, PipeSignals&);
}

namespace {

struct Metrics {
    unsigned assertions;
    unsigned failures;
    unsigned cases;
    unsigned packets;
    unsigned requests;
    unsigned rvc_protected;
    Metrics() : assertions(0), failures(0), cases(0), packets(0), requests(0),
                rvc_protected(0) {}
} metrics;

void check(bool condition, const std::string& message) {
    ++metrics.assertions;
    if (!condition) {
        ++metrics.failures;
        std::cerr << "FAIL[" << metrics.assertions << "]: " << message << '\n';
    }
}

void count_case() { ++metrics.cases; }

std::string hex64(uint64_t value) {
    std::ostringstream out;
    out << "0x" << std::hex << value;
    return out.str();
}

ImemResponse response_for(const ImemRequest& request, uint32_t word,
                          bool exception = false, uint64_t cause = 0) {
    ImemResponse response;
    response.address = request.address;
    response.fetch_id = request.fetch_id;
    response.epoch = request.epoch;
    response.instruction = word;
    response.exception = exception;
    response.exc_cause = cause;
    return response;
}

ImemRequest initial_request(BoomCoreState& state, PipeSignals& pipe) {
    boom::frontend_module(state, pipe);
    check(!pipe.imem_req.empty(), "initial request missing");
    ImemRequest request = pipe.imem_req.read();
    ++metrics.requests;
    check(request.address == RESET_VECTOR, "initial request is not at reset vector");
    check((request.address & 3u) == 0, "initial request is not word aligned");
    return request;
}

void branch_redirect(BoomCoreState& state, uint64_t target) {
    state.brupdate.valid = true;
    state.brupdate.mispredict = true;
    state.brupdate.jalr_target = target;
}

void clear_branch_redirect(BoomCoreState& state) {
    state.brupdate.valid = false;
    state.brupdate.mispredict = false;
}

struct Parcel {
    bool compressed;
    uint16_t raw;
    uint32_t instruction;
    Parcel(uint16_t c) : compressed(true), raw(c), instruction(0) {
        const boom::RvcDecodeResult decoded = boom::decompress_rvc(c);
        if (!decoded.legal) {
            std::cerr << "test construction used illegal RVC parcel " << hex64(c) << '\n';
            std::exit(2);
        }
        instruction = decoded.instruction;
    }
    Parcel(uint32_t i, bool) : compressed(false), raw(static_cast<uint16_t>(i)),
                               instruction(i) {}
};

struct ExpectedPacket {
    uint64_t pc;
    bool compressed;
    uint16_t raw;
    uint32_t instruction;
};

// Pack architectural parcels into the 32-bit memory words consumed by the frontend.
void append_parcel(std::vector<uint16_t>& halfwords, const Parcel& parcel) {
    halfwords.push_back(parcel.raw);
    if (!parcel.compressed)
        halfwords.push_back(static_cast<uint16_t>(parcel.instruction >> 16));
}

const uint16_t kLookaheadParcel = 0x0001u;
const uint32_t kLookaheadWord = 0x00010001u;

void add_lookahead_padding(std::map<uint64_t, uint32_t>& memory,
                           uint64_t begin, uint64_t end) {
    uint64_t addr = begin & ~3ull;
    const uint64_t limit = (end + 3ull) & ~3ull;
    while (addr <= limit) {
        if (memory.count(addr) == 0) memory[addr] = kLookaheadWord;
        addr += 4;
    }
}

bool service_imem_request(BoomCoreState& state, PipeSignals& pipe,
                          const std::map<uint64_t, uint32_t>& memory,
                          const std::string& name) {
    (void)state;
    if (pipe.imem_req.empty()) return false;
    const ImemRequest request = pipe.imem_req.read();
    ++metrics.requests;
    check((request.address & 3u) == 0, name + ": request lost alignment");
    const bool known = memory.count(request.address) != 0;
    check(known, name + ": request outside bounded test image at " +
                 hex64(request.address));
    const uint32_t word = known ? memory.find(request.address)->second : kLookaheadWord;
    pipe.imem_resp.write(response_for(request, word));
    return true;
}

MicroOp wait_for_packet_at(BoomCoreState& state, PipeSignals& pipe,
                           const std::map<uint64_t, uint32_t>& memory,
                           uint64_t target_pc, const std::string& name,
                           unsigned max_cycles = 24) {
    for (unsigned cycle = 0; cycle < max_cycles; ++cycle) {
        service_imem_request(state, pipe, memory, name);
        boom::frontend_module(state, pipe);
        if (state.frontend.fetch_packet_valid &&
            state.frontend.fetch_uop.debug_pc == target_pc)
            return state.frontend.fetch_uop;
    }
    check(false, name + ": packet at " + hex64(target_pc) + " not observed");
    return state.frontend.fetch_uop;
}

std::map<uint64_t, uint32_t> single_parcel_image(uint64_t base,
                                                 uint16_t parcel,
                                                 unsigned lookahead_words = 8) {
    std::map<uint64_t, uint32_t> memory;
    memory[base & ~3ull] = ((base & 2ull) != 0)
        ? (static_cast<uint32_t>(parcel) << 16) | kLookaheadParcel
        : static_cast<uint32_t>(parcel) | (static_cast<uint32_t>(kLookaheadParcel) << 16);
    add_lookahead_padding(memory, (base & ~3ull) + 4,
                          (base & ~3ull) + lookahead_words * 4);
    return memory;
}

void run_stream(const std::string& name, uint64_t base,
                const std::vector<Parcel>& parcels) {
    check((base & 3u) == 0, name + ": test base must be aligned");
    std::vector<uint16_t> halfwords;
    std::vector<ExpectedPacket> expected;
    uint64_t pc = base;
    for (std::size_t i = 0; i < parcels.size(); ++i) {
        ExpectedPacket packet = {pc, parcels[i].compressed, parcels[i].raw,
                                 parcels[i].instruction};
        expected.push_back(packet);
        append_parcel(halfwords, parcels[i]);
        pc += parcels[i].compressed ? 2 : 4;
    }
    if (halfwords.size() & 1u) halfwords.push_back(0x0001u);

    std::map<uint64_t, uint32_t> memory;
    for (std::size_t i = 0; i < halfwords.size(); i += 2) {
        memory[base + i * 2] = static_cast<uint32_t>(halfwords[i]) |
                               (static_cast<uint32_t>(halfwords[i + 1]) << 16);
    }
    const uint64_t architectural_end = expected.back().pc +
        (expected.back().compressed ? 2u : 4u);
    const uint64_t lookahead_limit = architectural_end + expected.size() * 4 + 64;
    add_lookahead_padding(memory, architectural_end, lookahead_limit);

    BoomCoreState state;
    PipeSignals pipe;
    ImemRequest discarded = initial_request(state, pipe);
    (void)discarded;
    branch_redirect(state, base);
    boom::frontend_module(state, pipe);
    clear_branch_redirect(state);
    check(!pipe.imem_req.empty(), name + ": redirect request missing");

    std::size_t seen = 0;
    unsigned cycles = 0;
    while (seen < expected.size() && cycles++ < expected.size() * 8 + 20) {
        service_imem_request(state, pipe, memory, name);
        boom::frontend_module(state, pipe);
        if (!state.frontend.fetch_packet_valid) continue;

        check(seen < expected.size(), name + ": duplicate packet");
        if (seen >= expected.size()) break;
        const ExpectedPacket& exp = expected[seen];
        const MicroOp& got = state.frontend.fetch_uop;
        check(got.debug_pc == exp.pc,
              name + ": PC mismatch at packet " + hex64(seen));
        check(got.is_rvc == exp.compressed,
              name + ": compressed flag mismatch at packet " + hex64(seen));
        check(!got.exception, name + ": unexpected fetch exception");
        check(got.inst == exp.instruction,
              name + ": instruction mismatch at PC " + hex64(exp.pc));
        if (exp.compressed)
            check(got.debug_inst == exp.raw, name + ": compressed debug parcel mismatch");
        ++seen;
        ++metrics.packets;
        count_case();
    }
    check(seen == expected.size(), name + ": packet drop or timeout");
    check(state.frontend.pc >= architectural_end && state.frontend.pc <= lookahead_limit + 4,
          name + ": final PC escaped bounded lookahead window");
}

void test_mixed_and_long_streams() {
    const uint32_t addi1 = 0x00108093u;
    const uint32_t addi2 = 0x00210113u;
    run_stream("C/C", 0x20000, std::vector<Parcel>{Parcel(0x0001), Parcel(0x0085)});
    run_stream("C/32", 0x21000, std::vector<Parcel>{Parcel(0x0001), Parcel(addi1, false)});
    run_stream("32/C", 0x22000, std::vector<Parcel>{Parcel(addi1, false), Parcel(0x0085)});
    run_stream("32/32", 0x23000,
               std::vector<Parcel>{Parcel(addi1, false), Parcel(addi2, false)});

    std::vector<Parcel> long_stream;
    for (unsigned i = 0; i < 96; ++i) {
        if (i % 5 == 0 || i % 5 == 3) {
            const uint16_t c_addi = static_cast<uint16_t>(0x0081u | ((i & 0x1fu) << 2));
            long_stream.push_back(Parcel(c_addi));
        } else {
            const uint32_t imm = (i + 1) & 0x7ffu;
            long_stream.push_back(Parcel((imm << 20) | 0x00000013u, false));
        }
    }
    run_stream("long-mixed", 0x24000, long_stream);
}

void test_redirect_alignment_and_odd_fault() {
    {
        BoomCoreState state;
        PipeSignals pipe;
        initial_request(state, pipe);
        branch_redirect(state, 0x30002);
        boom::frontend_module(state, pipe);
        clear_branch_redirect(state);
        check(!state.frontend.fetch_packet_valid, "PC%4=2 redirect incorrectly faulted");
        check(!pipe.imem_req.empty(), "PC%4=2 redirect request missing");
        const ImemRequest request = pipe.imem_req.read();
        ++metrics.requests;
        check(request.address == 0x30000, "PC%4=2 redirect request was not aligned down");
        std::map<uint64_t, uint32_t> memory;
        memory[request.address] = 0x00010013u;
        add_lookahead_padding(memory, 0x30004, 0x30040);
        pipe.imem_resp.write(response_for(request, memory[request.address]));
        const MicroOp got = wait_for_packet_at(state, pipe, memory, 0x30002,
                                               "PC%4=2 redirect");
        check(got.debug_pc == 0x30002,
              "PC%4=2 redirect target was masked architecturally");
        check(got.is_rvc, "upper compressed redirect parcel not marked RVC");
        check(state.frontend.pc >= 0x30004 && state.frontend.pc <= 0x30044,
              "upper compressed redirect PC escaped bounded lookahead");
        count_case();
    }
    {
        BoomCoreState state;
        PipeSignals pipe;
        initial_request(state, pipe);
        branch_redirect(state, 0x31003);
        boom::frontend_module(state, pipe);
        clear_branch_redirect(state);
        MicroOp got;
        bool observed = false;
        for (unsigned cycle = 0; cycle < 8 && !observed; ++cycle) {
            check(pipe.imem_req.empty(), "odd redirect issued an IMEM request");
            if (state.frontend.fetch_packet_valid) {
                got = state.frontend.fetch_uop;
                observed = true;
                break;
            }
            boom::frontend_module(state, pipe);
        }
        check(observed, "odd redirect did not produce a fault");
        check(got.exception, "odd redirect fault bit missing");
        check(got.exc_cause == 0, "odd redirect fault cause changed");
        check(got.exc.xcpt_ma_if, "odd redirect missing MA-IF detail");
        check(got.debug_pc == 0x31003, "odd redirect target was masked");
        state.frontend.fetch_packet_valid = false;
        boom::frontend_module(state, pipe);
        check(pipe.imem_req.empty(), "odd redirect fetched after its fault was consumed");
        check(state.frontend.pc == 0x31003, "odd redirect PC changed after fault consumption");
        count_case();
    }
}

void test_decode_rvc_metadata() {
    BoomCoreState state;
    state.frontend.fetch_packet_valid = true;
    state.frontend.fetch_uop.inst = 0x00108093u;
    state.frontend.fetch_uop.debug_pc = 0x31502;
    state.frontend.fetch_uop.debug_inst = 0x0085;
    state.frontend.fetch_uop.is_rvc = true;
    boom::decode_module(state);
    check(state.decode.dec_valids[0], "RVC metadata decode entry missing");
    check(state.decode.dec_uops[0].is_rvc, "RVC attribution did not reach Decode");
    check(state.decode.dec_uops[0].debug_inst == 0x0085,
          "original compressed bits did not reach Decode");
    check(state.decode.dec_uops[0].debug_pc == 0x31502,
          "compressed instruction PC changed in Decode");
    count_case();
}

ImemRequest establish_cross_carry(BoomCoreState& state, PipeSignals& pipe,
                                  uint64_t target, uint16_t lower) {
    initial_request(state, pipe);
    branch_redirect(state, target);
    boom::frontend_module(state, pipe);
    clear_branch_redirect(state);
    check(!pipe.imem_req.empty(), "cross-word first request missing");
    const ImemRequest first = pipe.imem_req.read();
    ++metrics.requests;
    check(first.address == (target & ~3ull), "cross-word first request address mismatch");
    pipe.imem_resp.write(response_for(first, static_cast<uint32_t>(lower) << 16));
    boom::frontend_module(state, pipe);
    check(state.frontend.halfword_valid, "upper 32-bit parcel was not retained as carry");
    check(state.frontend.halfword_pc == target, "carry PC mismatch");
    check(!pipe.imem_req.empty(), "cross-word upper request missing");
    ImemRequest upper = pipe.imem_req.read();
    ++metrics.requests;
    check(upper.address == ((target + 2) & ~3ull), "cross-word upper request address mismatch");
    return upper;
}

void test_cross_word_identity_and_delayed_upper() {
    BoomCoreState state;
    PipeSignals pipe;
    const uint64_t pc = 0x32002;
    const uint32_t instruction = 0x12300093u;
    const ImemRequest upper = establish_cross_carry(
        state, pipe, pc, static_cast<uint16_t>(instruction));

    ImemResponse bad = response_for(upper, instruction >> 16);
    bad.fetch_id++;
    pipe.imem_resp.write(bad);
    boom::frontend_module(state, pipe);
    check(state.frontend.request_sent, "wrong-ID upper consumed pending request");
    check(state.frontend.halfword_valid, "wrong-ID upper erased carry");
    check(!state.frontend.fetch_packet_valid, "wrong-ID upper emitted packet");

    bad = response_for(upper, instruction >> 16);
    bad.epoch++;
    pipe.imem_resp.write(bad);
    boom::frontend_module(state, pipe);
    check(state.frontend.request_sent, "wrong-epoch upper consumed pending request");
    check(state.frontend.halfword_valid, "wrong-epoch upper erased carry");

    bad = response_for(upper, instruction >> 16);
    bad.address += 4;
    pipe.imem_resp.write(bad);
    boom::frontend_module(state, pipe);
    check(state.frontend.request_sent, "wrong-address upper consumed pending request");
    check(!state.frontend.fetch_packet_valid, "wrong-address upper emitted packet");

    std::map<uint64_t, uint32_t> memory;
    memory[upper.address] = instruction >> 16;
    add_lookahead_padding(memory, pc + 4, pc + 64);
    pipe.imem_resp.write(response_for(upper, instruction >> 16));
    const MicroOp got = wait_for_packet_at(state, pipe, memory, pc,
                                           "matching delayed upper");
    check(!got.is_rvc, "assembled cross-word instruction marked RVC");
    check(got.inst == instruction, "cross-word assembly mismatch");
    check(got.debug_pc == pc, "cross-word packet PC mismatch");
    check(state.frontend.pc >= pc + 4 && state.frontend.pc <= pc + 68,
          "cross-word instruction PC escaped bounded lookahead");
    check(!state.frontend.halfword_valid, "carry survived completed assembly");
    check(!state.frontend.response_received, "assembled response remained replayable");
    count_case();
}

void test_backpressure_and_stall_holds() {
    {
        BoomCoreState state;
        PipeSignals pipe;
        for (unsigned i = 0; i < 1024; ++i) pipe.imem_req.write(ImemRequest());
        boom::frontend_module(state, pipe);
        check(!state.frontend.request_sent, "full request stream accepted request");
        check(state.frontend.fetch_id == 0, "backpressure advanced fetch ID");
        check(state.frontend.pc == RESET_VECTOR, "backpressure changed PC");
        pipe.imem_req.read();
        boom::frontend_module(state, pipe);
        check(state.frontend.request_sent, "request did not retry after backpressure");
        for (unsigned i = 0; i < 1023; ++i) pipe.imem_req.read();
        const ImemRequest retried = pipe.imem_req.read();
        ++metrics.requests;
        check(retried.fetch_id == 0 && retried.address == RESET_VECTOR,
              "retried request payload changed");
        count_case();
    }
    {
        BoomCoreState state;
        PipeSignals pipe;
        const ImemRequest request = initial_request(state, pipe);
        state.decode.dec_valids[0] = true;
        pipe.imem_resp.write(response_for(request, 0x00108093u));
        boom::frontend_module(state, pipe);
        check(pipe.imem_resp.empty(), "decode stall did not drain response");
        check(!state.frontend.fetch_packet_valid, "decode stall exposed response early");
        state.decode.dec_valids[0] = false;
        std::map<uint64_t, uint32_t> memory;
        add_lookahead_padding(memory, RESET_VECTOR + 4, RESET_VECTOR + 64);
        const MicroOp held = wait_for_packet_at(state, pipe, memory, RESET_VECTOR,
                                                "retained stalled response");
        check(held.inst == 0x00108093u, "retained stalled response instruction changed");
        check(held.debug_pc == RESET_VECTOR, "retained stalled response PC changed");
        count_case();
    }
}

void check_fault(const MicroOp& uop, uint64_t pc, uint64_t cause,
                 bool access, const std::string& name) {
    check(uop.exception, name + ": exception bit missing");
    check(uop.exc.exception, name + ": nested exception bit missing");
    check(uop.exc_cause == cause && uop.exc.exc_cause == cause,
          name + ": exception cause mismatch");
    check(uop.debug_pc == pc, name + ": fault PC mismatch");
    check(uop.exc.xcpt_ae_if == access, name + ": access-fault detail mismatch");
}

void test_fault_positions_and_stale_fault() {
    const uint16_t c_nop = 0x0001;
    {
        BoomCoreState state;
        PipeSignals pipe;
        const ImemRequest request = initial_request(state, pipe);
        pipe.imem_resp.write(response_for(request, c_nop, true, 7));
        std::map<uint64_t, uint32_t> memory;
        add_lookahead_padding(memory, RESET_VECTOR + 4, RESET_VECTOR + 64);
        const MicroOp got = wait_for_packet_at(state, pipe, memory, RESET_VECTOR,
                                               "compressed-position fault");
        check_fault(got, RESET_VECTOR, 7, true, "compressed fault");
        count_case();
    }
    {
        BoomCoreState state;
        PipeSignals pipe;
        const ImemRequest request = initial_request(state, pipe);
        pipe.imem_resp.write(response_for(request, 0x00108093u, true, 9));
        std::map<uint64_t, uint32_t> memory;
        add_lookahead_padding(memory, RESET_VECTOR + 4, RESET_VECTOR + 64);
        const MicroOp got = wait_for_packet_at(state, pipe, memory, RESET_VECTOR,
                                               "aligned-32 fault");
        check_fault(got, RESET_VECTOR, 9, true, "aligned-32 fault");
        count_case();
    }
    {
        BoomCoreState state;
        PipeSignals pipe;
        const uint64_t pc = 0x33002;
        const ImemRequest upper = establish_cross_carry(state, pipe, pc, 0x0093);
        pipe.imem_resp.write(response_for(upper, 0, true, 11));
        std::map<uint64_t, uint32_t> memory;
        add_lookahead_padding(memory, pc + 4, pc + 64);
        const MicroOp got = wait_for_packet_at(state, pipe, memory, pc,
                                               "delayed upper fault");
        check_fault(got, pc, 11, true, "cross-upper fault");
        check(!state.frontend.halfword_valid, "upper fault left carry valid");
        count_case();
    }
    {
        BoomCoreState state;
        PipeSignals pipe;
        const ImemRequest old = initial_request(state, pipe);
        branch_redirect(state, 0x34000);
        pipe.imem_resp.write(response_for(old, 0, true, 13));
        boom::frontend_module(state, pipe);
        clear_branch_redirect(state);
        check(!state.frontend.fetch_packet_valid, "stale redirect-cycle fault leaked");
        check(!pipe.imem_req.empty(), "redirect after stale fault did not request target");
        const ImemRequest fresh = pipe.imem_req.read();
        ++metrics.requests;
        check(fresh.address == 0x34000, "stale fault changed redirect address");
        pipe.imem_resp.write(response_for(fresh, 0x00108093u));
        std::map<uint64_t, uint32_t> memory;
        memory[fresh.address] = 0x00108093u;
        add_lookahead_padding(memory, fresh.address + 4, fresh.address + 64);
        const MicroOp got = wait_for_packet_at(state, pipe, memory, fresh.address,
                                               "fresh packet after stale fault");
        check(!got.exception, "stale fault contaminated fresh packet");
        count_case();
    }
}

void test_redirect_and_reset_while_carry() {
    {
        BoomCoreState state;
        PipeSignals pipe;
        const ImemRequest stale_upper = establish_cross_carry(state, pipe, 0x35002, 0x0093);
        branch_redirect(state, 0x36002);
        pipe.imem_resp.write(response_for(stale_upper, 0x1230));
        boom::frontend_module(state, pipe);
        clear_branch_redirect(state);
        check(!state.frontend.halfword_valid, "redirect did not clear carry");
        check(!state.frontend.fetch_packet_valid, "stale upper beat redirect");
        check(!pipe.imem_req.empty(), "redirect while carry did not request target");
        const ImemRequest request = pipe.imem_req.read();
        ++metrics.requests;
        check(request.address == 0x36000, "redirect while carry target address mismatch");
        check(request.epoch != stale_upper.epoch, "redirect while carry did not change epoch");
        count_case();
    }
    {
        BoomCoreState state;
        PipeSignals pipe;
        const ImemRequest stale_upper = establish_cross_carry(state, pipe, 0x37002, 0x0093);
        state.frontend.reset_done = false;
        pipe.imem_resp.write(response_for(stale_upper, 0x1230));
        boom::frontend_module(state, pipe);
        check(!state.frontend.halfword_valid, "reset did not clear carry");
        check(!state.frontend.fetch_packet_valid, "pre-reset upper leaked");
        check(!pipe.imem_req.empty(), "reset while carry did not request reset vector");
        const ImemRequest request = pipe.imem_req.read();
        ++metrics.requests;
        check(request.address == RESET_VECTOR, "reset while carry requested wrong address");
        check(request.fetch_id == 0, "reset while carry did not restart fetch ID");
        check(request.epoch != stale_upper.epoch, "reset while carry did not change epoch");
        count_case();
    }
}

void test_production_branch_recovery_while_carry() {
    BoomCoreState state;
    PipeSignals pipe;
    const ImemRequest stale_upper = establish_cross_carry(state, pipe, 0x38002, 0x0093);
    const uint64_t target = 0x39002;
    MicroOp branch;
    branch.queue.rob_idx = 1;
    branch.queue.rob_allocation_id = 7;
    branch.branch.br_tag = 0;
    state.rob.entries[1].valid = true;
    state.rob.entries[1].uop.queue.rob_allocation_id = branch.queue.rob_allocation_id;
    state.branch_state.tag_valid[0] = true;
    state.branch_state.snapshot_valid[0] = true;

    boom::branch_complete_event(state, branch, true, target);
    pipe.imem_resp.write(response_for(stale_upper, 0x1230));
    boom::frontend_module(state, pipe);
    clear_branch_redirect(state);
    check(!state.frontend.halfword_valid, "production branch recovery retained carry");
    check(state.frontend.pc == target, "production branch recovery target mismatch");
    check(state.frontend.epoch != stale_upper.epoch,
          "production branch recovery did not change epoch");
    check(!state.frontend.fetch_packet_valid, "stale carried response survived branch recovery");
    check(!pipe.imem_req.empty(), "branch recovery did not request redirect target");
    const ImemRequest request = pipe.imem_req.read();
    ++metrics.requests;
    check(request.address == (target & ~3ull), "branch recovery requested stale carry address");
    check(request.epoch == state.frontend.epoch, "branch recovery request used stale epoch");
    count_case();
}

void test_reserved_and_unsupported_rvc() {
    const uint16_t illegal[] = {
        0x0000, // reserved C.ADDI4SPN imm=0
        0x2000, // unsupported C.FLD
        0xa000, // unsupported C.FSD
        0x2002, // unsupported C.FLDSP
        0xa002, // unsupported C.FSDSP
        0x8002, // reserved C.JR x0
        0x6001, // reserved C.LUI x0, 0
        0x6101  // reserved C.ADDI16SP zero immediate
    };
    for (std::size_t i = 0; i < sizeof(illegal) / sizeof(illegal[0]); ++i) {
        const boom::RvcDecodeResult decoded = boom::decompress_rvc(illegal[i]);
        check(!decoded.legal, "reserved/unsupported RVC unexpectedly decompressed: " +
                              hex64(illegal[i]));
        BoomCoreState state;
        PipeSignals pipe;
        const ImemRequest request = initial_request(state, pipe);
        std::map<uint64_t, uint32_t> memory = single_parcel_image(request.address,
                                                                  illegal[i]);
        pipe.imem_resp.write(response_for(request, illegal[i]));
        const MicroOp got = wait_for_packet_at(state, pipe, memory, request.address,
                                               "illegal RVC");
        check(got.is_rvc, "illegal RVC lost compressed attribution");
        check(got.exception, "illegal RVC did not fault");
        check(got.exc_cause == 2, "illegal RVC cause is not illegal instruction");
        check(got.debug_inst == illegal[i], "illegal RVC debug parcel mismatch");
        count_case();
    }

    BoomCoreState state;
    PipeSignals pipe;
    const ImemRequest request = initial_request(state, pipe);
    std::map<uint64_t, uint32_t> memory;
    memory[request.address] = 0x0000001fu;
    add_lookahead_padding(memory, request.address + 4, request.address + 64);
    pipe.imem_resp.write(response_for(request, 0x0000001fu));
    const MicroOp got = wait_for_packet_at(state, pipe, memory, request.address,
                                           "reserved long encoding");
    check(got.exception, "reserved long encoding did not fault");
    check(got.exc_cause == 2, "reserved long encoding cause mismatch");
    check(!got.is_rvc, "reserved long encoding marked compressed");
    check(state.frontend.pc >= RESET_VECTOR + 2 &&
          state.frontend.pc <= RESET_VECTOR + 68,
          "reserved long encoding PC escaped bounded lookahead");
    count_case();
}

void test_protected_decode_gaps() {
    {
        const uint16_t parcel = 0x9002;
        const boom::RvcDecodeResult decoded = boom::decompress_rvc(parcel);
        check(decoded.legal && decoded.instruction == 0x00100073u,
              "C.EBREAK decompressor reference changed");
        BoomCoreState state;
        PipeSignals pipe;
        const ImemRequest request = initial_request(state, pipe);
        std::map<uint64_t, uint32_t> memory = single_parcel_image(request.address,
                                                                  parcel);
        pipe.imem_resp.write(response_for(request, parcel));
        const MicroOp got = wait_for_packet_at(state, pipe, memory, request.address,
                                               "protected C.EBREAK");
        check(!got.exception,
              "C.EBREAK was rejected before base Decode");
        check(got.debug_inst == parcel, "C.EBREAK debug parcel mismatch");
        boom::decode_module(state);
        check(state.decode.dec_valids[0] && state.decode.dec_uops[0].uopc == 66,
              "C.EBREAK Decode uopc mismatch");
        check(state.decode.dec_uops[0].exception && state.decode.dec_uops[0].exc_cause == 3,
              "C.EBREAK did not raise breakpoint cause 3");
        ++metrics.rvc_protected;
        count_case();
    }

    // Every C.SRLI encoding with shamt[5]=1: all 8 compact registers and 32 low shamts.
    for (unsigned reg = 0; reg < 8; ++reg) {
        for (unsigned low_shamt = 0; low_shamt < 32; ++low_shamt) {
            const uint16_t parcel = static_cast<uint16_t>(
                0x9001u | (reg << 7) | (low_shamt << 2));
            const boom::RvcDecodeResult decoded = boom::decompress_rvc(parcel);
            check(decoded.legal, "protected C.SRLI reference is not legal RVC");
            check((decoded.instruction & 0x707fu) == 0x5013u,
                  "protected C.SRLI expansion opcode/funct3 mismatch");
            check(((decoded.instruction >> 25) & 0x7fu) == 1u,
                  "protected C.SRLI expansion lost shamt[5]");

            BoomCoreState state;
            PipeSignals pipe;
            const ImemRequest request = initial_request(state, pipe);
            std::map<uint64_t, uint32_t> memory = single_parcel_image(request.address,
                                                                      parcel);
            pipe.imem_resp.write(response_for(request, parcel));
            const MicroOp got = wait_for_packet_at(state, pipe, memory, request.address,
                                                   "protected C.SRLI");
            check(got.is_rvc, "protected C.SRLI lost RVC attribution");
            check(!got.exception,
                  "RV64 C.SRLI shamt[5] was rejected before Decode");
            check(got.debug_inst == parcel,
                  "C.SRLI debug parcel mismatch");
            check(got.inst == decoded.instruction,
                  "C.SRLI final instruction mismatch");
            check(((got.inst >> 25) & 0x7fu) == 1u,
                  "C.SRLI final instruction lost shamt[5]");
            boom::decode_module(state);
            check(state.decode.dec_valids[0] && state.decode.dec_uops[0].uopc == 55,
                  "RV64 C.SRLI shamt[5] Decode mismatch");
            check(!state.decode.dec_uops[0].exception,
                  "RV64 C.SRLI shamt[5] Decode faulted");
            ++metrics.rvc_protected;
            count_case();
        }
    }

    // Every legal C.JALR form must retain compressed attribution for PC+2 link.
    for (unsigned rd = 1; rd < 32; ++rd) {
        const uint16_t parcel = static_cast<uint16_t>(0x9002u | (rd << 7));
        const boom::RvcDecodeResult decoded = boom::decompress_rvc(parcel);
        check(decoded.legal && (decoded.instruction & 0x7fu) == 0x67u,
              "protected C.JALR reference changed");
        BoomCoreState state;
        PipeSignals pipe;
        const ImemRequest request = initial_request(state, pipe);
        std::map<uint64_t, uint32_t> memory = single_parcel_image(request.address,
                                                                  parcel);
        pipe.imem_resp.write(response_for(request, parcel));
        const MicroOp got = wait_for_packet_at(state, pipe, memory, request.address,
                                               "protected C.JALR");
        check(!got.exception,
              "C.JALR was rejected before Decode");
        check(got.debug_inst == parcel,
              "C.JALR debug parcel mismatch");
        check(got.inst == decoded.instruction,
              "C.JALR final instruction mismatch");
        boom::decode_module(state);
        check(state.decode.dec_valids[0] && state.decode.dec_uops[0].uopc == 30,
              "C.JALR Decode uopc mismatch");
        check(state.decode.dec_uops[0].is_rvc && !state.decode.dec_uops[0].exception,
              "C.JALR Decode metadata mismatch");
        ++metrics.rvc_protected;
        count_case();
    }
}

} // namespace

int main() {
    test_mixed_and_long_streams();
    test_redirect_alignment_and_odd_fault();
    test_decode_rvc_metadata();
    test_cross_word_identity_and_delayed_upper();
    test_backpressure_and_stall_holds();
    test_fault_positions_and_stale_fault();
    test_redirect_and_reset_while_carry();
    test_production_branch_recovery_while_carry();
    test_reserved_and_unsupported_rvc();
    test_protected_decode_gaps();

    std::cout << "RVC_FETCH_METRICS assertions=" << metrics.assertions
              << " cases=" << metrics.cases
              << " packets=" << metrics.packets
              << " requests=" << metrics.requests
              << " protected=" << metrics.rvc_protected
              << " failures=" << metrics.failures << '\n';
    if (metrics.assertions < 150 || metrics.cases < 150) {
        std::cerr << "FAIL: required assertion/case floor was not reached\n";
        return 1;
    }
    if (metrics.failures != 0) return 1;
    std::cout << "RVC_FETCH_CANONICAL_CPP11_UNIQUE_PASS_726\n";
    return 0;
}
