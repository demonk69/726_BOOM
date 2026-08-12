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
        if (!pipe.imem_req.empty()) {
            const ImemRequest request = pipe.imem_req.read();
            ++metrics.requests;
            check((request.address & 3u) == 0, name + ": request lost alignment");
            check(memory.count(request.address) != 0,
                  name + ": request outside constructed image at " + hex64(request.address));
            const uint32_t word = memory.count(request.address) ? memory[request.address] : 0;
            pipe.imem_resp.write(response_for(request, word));
        }
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
    check(state.frontend.pc == base + halfwords.size() * 2 ||
          state.frontend.pc == expected.back().pc +
              (expected.back().compressed ? 2u : 4u),
          name + ": final sequential PC is not exact");
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
        pipe.imem_resp.write(response_for(request, 0x00010013u));
        boom::frontend_module(state, pipe);
        check(state.frontend.fetch_packet_valid, "PC%4=2 redirect parcel not delivered");
        check(state.frontend.fetch_uop.debug_pc == 0x30002,
              "PC%4=2 redirect target was masked architecturally");
        check(state.frontend.fetch_uop.is_rvc, "upper compressed redirect parcel not marked RVC");
        check(state.frontend.pc == 0x30004, "upper compressed redirect did not advance PC by two");
        count_case();
    }
    {
        BoomCoreState state;
        PipeSignals pipe;
        initial_request(state, pipe);
        branch_redirect(state, 0x31003);
        boom::frontend_module(state, pipe);
        clear_branch_redirect(state);
        check(state.frontend.fetch_packet_valid, "odd redirect did not produce a fault");
        check(state.frontend.fetch_uop.exception, "odd redirect fault bit missing");
        check(state.frontend.fetch_uop.exc_cause == 0, "odd redirect fault cause changed");
        check(state.frontend.fetch_uop.exc.xcpt_ma_if, "odd redirect missing MA-IF detail");
        check(state.frontend.fetch_uop.debug_pc == 0x31003, "odd redirect target was masked");
        check(pipe.imem_req.empty(), "odd redirect issued an IMEM request");
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

    pipe.imem_resp.write(response_for(upper, instruction >> 16));
    boom::frontend_module(state, pipe);
    check(state.frontend.fetch_packet_valid, "matching delayed upper did not emit packet");
    check(!state.frontend.fetch_uop.is_rvc, "assembled cross-word instruction marked RVC");
    check(state.frontend.fetch_uop.inst == instruction, "cross-word assembly mismatch");
    check(state.frontend.fetch_uop.debug_pc == pc, "cross-word packet PC mismatch");
    check(state.frontend.pc == pc + 4, "cross-word instruction did not advance PC by four");
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
        check(state.frontend.response_received, "decode stall did not retain response");
        check(!state.frontend.fetch_packet_valid, "decode stall exposed response early");
        state.decode.dec_valids[0] = false;
        boom::frontend_module(state, pipe);
        check(state.frontend.fetch_packet_valid, "retained stalled response was not emitted");
        const MicroOp held = state.frontend.fetch_uop;
        state.decode.dec_valids[0] = true;
        boom::frontend_module(state, pipe);
        check(state.frontend.fetch_packet_valid, "decode stall dropped held packet");
        check(state.frontend.fetch_uop.inst == held.inst, "held packet instruction changed");
        check(state.frontend.fetch_uop.debug_pc == held.debug_pc, "held packet PC changed");
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
        boom::frontend_module(state, pipe);
        check(state.frontend.fetch_packet_valid, "compressed-position fault missing");
        check_fault(state.frontend.fetch_uop, RESET_VECTOR, 7, true, "compressed fault");
        count_case();
    }
    {
        BoomCoreState state;
        PipeSignals pipe;
        const ImemRequest request = initial_request(state, pipe);
        pipe.imem_resp.write(response_for(request, 0x00108093u, true, 9));
        boom::frontend_module(state, pipe);
        check_fault(state.frontend.fetch_uop, RESET_VECTOR, 9, true, "aligned-32 fault");
        count_case();
    }
    {
        BoomCoreState state;
        PipeSignals pipe;
        const uint64_t pc = 0x33002;
        const ImemRequest upper = establish_cross_carry(state, pipe, pc, 0x0093);
        pipe.imem_resp.write(response_for(upper, 0, true, 11));
        boom::frontend_module(state, pipe);
        check(state.frontend.fetch_packet_valid, "delayed upper fault missing");
        check_fault(state.frontend.fetch_uop, pc, 11, true, "cross-upper fault");
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
        boom::frontend_module(state, pipe);
        check(!state.frontend.fetch_uop.exception, "stale fault contaminated fresh packet");
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
    check(!state.frontend.halfword_valid, "production branch recovery retained carry");
    check(!state.frontend.request_sent, "production branch recovery retained request");
    check(state.frontend.pc == target, "production branch recovery target mismatch");
    check(state.frontend.epoch != stale_upper.epoch,
          "production branch recovery did not change epoch");

    pipe.imem_resp.write(response_for(stale_upper, 0x1230));
    state.brupdate = BranchUpdate();
    boom::frontend_module(state, pipe);
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
        pipe.imem_resp.write(response_for(request, illegal[i]));
        boom::frontend_module(state, pipe);
        check(state.frontend.fetch_packet_valid, "illegal RVC packet missing");
        check(state.frontend.fetch_uop.is_rvc, "illegal RVC lost compressed attribution");
        check(state.frontend.fetch_uop.exception, "illegal RVC did not fault");
        check(state.frontend.fetch_uop.exc_cause == 2, "illegal RVC cause is not illegal instruction");
        check(state.frontend.fetch_uop.debug_inst == illegal[i], "illegal RVC debug parcel mismatch");
        count_case();
    }

    BoomCoreState state;
    PipeSignals pipe;
    const ImemRequest request = initial_request(state, pipe);
    pipe.imem_resp.write(response_for(request, 0x0000001fu));
    boom::frontend_module(state, pipe);
    check(state.frontend.fetch_packet_valid, "reserved long encoding packet missing");
    check(state.frontend.fetch_uop.exception, "reserved long encoding did not fault");
    check(state.frontend.fetch_uop.exc_cause == 2, "reserved long encoding cause mismatch");
    check(!state.frontend.fetch_uop.is_rvc, "reserved long encoding marked compressed");
    check(state.frontend.pc == RESET_VECTOR + 2, "reserved long encoding PC increment mismatch");
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
        pipe.imem_resp.write(response_for(request, parcel));
        boom::frontend_module(state, pipe);
        check(state.frontend.fetch_packet_valid, "protected C.EBREAK packet missing");
        check(state.frontend.fetch_uop.exception && state.frontend.fetch_uop.exc_cause == 2,
              "protected C.EBREAK was allowed through frontend");
        check(state.frontend.fetch_uop.debug_inst == parcel, "C.EBREAK debug parcel mismatch");
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
            pipe.imem_resp.write(response_for(request, parcel));
            boom::frontend_module(state, pipe);
            check(state.frontend.fetch_packet_valid, "protected C.SRLI packet missing");
            check(state.frontend.fetch_uop.is_rvc, "protected C.SRLI lost RVC attribution");
            check(state.frontend.fetch_uop.exception, "protected C.SRLI did not fault");
            check(state.frontend.fetch_uop.exc_cause == 2,
                  "protected C.SRLI fault cause mismatch");
            check(state.frontend.fetch_uop.debug_inst == parcel,
                  "protected C.SRLI debug parcel mismatch");
            ++metrics.rvc_protected;
            count_case();
        }
    }

    // C.JALR requires a PC+2 link value, while the frozen backend writes PC+4.
    for (unsigned rd = 1; rd < 32; ++rd) {
        const uint16_t parcel = static_cast<uint16_t>(0x9002u | (rd << 7));
        const boom::RvcDecodeResult decoded = boom::decompress_rvc(parcel);
        check(decoded.legal && (decoded.instruction & 0x7fu) == 0x67u,
              "protected C.JALR reference changed");
        BoomCoreState state;
        PipeSignals pipe;
        const ImemRequest request = initial_request(state, pipe);
        pipe.imem_resp.write(response_for(request, parcel));
        boom::frontend_module(state, pipe);
        check(state.frontend.fetch_packet_valid, "protected C.JALR packet missing");
        check(state.frontend.fetch_uop.exception && state.frontend.fetch_uop.exc_cause == 2,
              "protected C.JALR was allowed into the frozen PC+4 backend");
        check(state.frontend.fetch_uop.debug_inst == parcel,
              "protected C.JALR debug parcel mismatch");
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
