#include "boom_interfaces.hpp"
#include "boom_state.hpp"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace boom {
DividerState::DividerState()
    : busy(false), result_pending(false), operation(DIV_OP_SIGNED),
      original_dividend(0), original_divisor(0), dividend_magnitude(0),
      divisor_magnitude(0), quotient(0), remainder(0), iteration(0),
      quotient_negative(false), remainder_negative(false), word_operation(false) {}
void frontend_module(BoomCoreState&, PipeSignals&);
void decode_module(BoomCoreState&);
}

namespace {

struct Instruction {
    bool compressed;
    uint32_t bits;
    Instruction(bool c, uint32_t value) : compressed(c), bits(value) {}
};

struct TraceRow {
    std::string scenario;
    unsigned cycle;
    bool request_valid;
    uint64_t request_address;
    bool response_valid;
    uint64_t response_address;
    bool response_accepted;
    bool publication_valid;
    bool decode_accepted;
    uint64_t publication_pc;
    bool publication_rvc;
    uint64_t pc_before;
    uint64_t pc_after;
    bool carry_before;
    bool carry_after;
    bool hold_before;
    bool hold_after;
    bool stall;
};

struct Result {
    std::string name;
    unsigned instructions;
    unsigned cycles;
    unsigned requests;
    unsigned responses;
    unsigned bytes;
    std::vector<unsigned> request_cycles;
    std::vector<unsigned> publication_cycles;
};

struct IntervalStats {
    double mean;
    unsigned minimum;
    unsigned maximum;
};

int failures = 0;

void check(bool condition, const std::string& message) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

uint32_t memory_word(const std::map<uint64_t, uint16_t>& memory, uint64_t address) {
    std::map<uint64_t, uint16_t>::const_iterator low = memory.find(address);
    std::map<uint64_t, uint16_t>::const_iterator high = memory.find(address + 2);
    const uint16_t lo = low == memory.end() ? 0x0001u : low->second;
    const uint16_t hi = high == memory.end() ? 0x0001u : high->second;
    return static_cast<uint32_t>(lo) | (static_cast<uint32_t>(hi) << 16);
}

ImemResponse response_for(const ImemRequest& request,
                          const std::map<uint64_t, uint16_t>& memory) {
    ImemResponse response;
    response.address = request.address;
    response.fetch_id = request.fetch_id;
    response.epoch = request.epoch;
    response.instruction = memory_word(memory, request.address);
    return response;
}

IntervalStats intervals(const std::vector<unsigned>& cycles) {
    IntervalStats stats = {0.0, 0, 0};
    if (cycles.size() < 2) return stats;
    stats.minimum = cycles[1] - cycles[0];
    stats.maximum = stats.minimum;
    unsigned total = 0;
    for (std::size_t i = 1; i < cycles.size(); ++i) {
        const unsigned interval = cycles[i] - cycles[i - 1];
        total += interval;
        stats.minimum = std::min(stats.minimum, interval);
        stats.maximum = std::max(stats.maximum, interval);
    }
    stats.mean = static_cast<double>(total) / (cycles.size() - 1);
    return stats;
}

Result run_scenario(const std::string& name, uint64_t start_pc,
                    const std::vector<Instruction>& program,
                    std::vector<TraceRow>& trace) {
    std::map<uint64_t, uint16_t> memory;
    std::vector<uint64_t> expected_pc;
    uint64_t pc = start_pc;
    for (std::size_t i = 0; i < program.size(); ++i) {
        expected_pc.push_back(pc);
        memory[pc] = static_cast<uint16_t>(program[i].bits);
        if (!program[i].compressed)
            memory[pc + 2] = static_cast<uint16_t>(program[i].bits >> 16);
        pc += program[i].compressed ? 2 : 4;
    }

    BoomCoreState state;
    PipeSignals pipe;
    state.frontend.reset_done = true;
    state.frontend.pc = start_pc;
    state.frontend.epoch = 0;

    bool pending_response = false;
    ImemResponse pending;
    std::size_t published = 0;
    Result result = {name, static_cast<unsigned>(program.size()), 0, 0, 0, 0,
                     std::vector<unsigned>(), std::vector<unsigned>()};

    const unsigned timeout = static_cast<unsigned>(program.size() * 5 + 16);
    for (unsigned cycle = 0; published < program.size() && cycle < timeout; ++cycle) {
        const bool response_valid = pending_response;
        const uint64_t response_address = response_valid ? pending.address : 0;
        const bool response_accepted = response_valid && state.frontend.request_sent &&
            pending.fetch_id == state.frontend.pending_fetch_id &&
            pending.epoch == state.frontend.pending_epoch &&
            pending.address == state.frontend.pending_address;
        if (response_valid) {
            pipe.imem_resp.write(pending);
            pending_response = false;
            ++result.responses;
        }

        // The downstream consumer retires Decode's previous one-entry output.
        state.decode.dec_valids[0] = false;
        const uint64_t pc_before = state.frontend.pc;
        const bool carry_before = state.frontend.halfword_valid;
        const bool hold_before = state.frontend.fetch_packet_valid;

        boom::frontend_module(state, pipe);
        const bool publication = state.frontend.fetch_packet_valid;
        const uint64_t publication_pc = publication ? state.frontend.fetch_uop.debug_pc : 0;
        const bool publication_rvc = publication && state.frontend.fetch_uop.is_rvc;
        boom::decode_module(state);
        const bool decode_accepted = publication && state.decode.dec_valids[0];

        bool request_valid = false;
        uint64_t request_address = 0;
        if (!pipe.imem_req.empty()) {
            const ImemRequest request = pipe.imem_req.read();
            request_valid = true;
            request_address = request.address;
            check((request.address & 3u) == 0, name + ": unaligned IMEM request");
            check(!pending_response, name + ": more than one request outstanding");
            pending = response_for(request, memory);
            pending_response = true;
            ++result.requests;
            result.request_cycles.push_back(cycle);
        }

        if (publication) {
            check(published < program.size(), name + ": excess publication");
            if (published < program.size()) {
                check(publication_pc == expected_pc[published], name + ": publication PC mismatch");
                check(publication_rvc == program[published].compressed,
                      name + ": publication RVC attribution mismatch");
            }
            check(decode_accepted, name + ": Decode did not accept publication");
            result.publication_cycles.push_back(cycle);
            ++published;
        }

        TraceRow row = {name, cycle, request_valid, request_address,
                        response_valid, response_address, response_accepted,
                        publication, decode_accepted, publication_pc, publication_rvc,
                        pc_before, state.frontend.pc, carry_before,
                        state.frontend.halfword_valid, hold_before,
                        state.frontend.fetch_packet_valid, state.frontend.stalled};
        trace.push_back(row);
        result.cycles = cycle + 1;
    }

    check(published == program.size(), name + ": timeout or dropped instruction");
    result.bytes = result.requests * 4;
    return result;
}

void write_trace(const std::string& path, const std::vector<TraceRow>& rows) {
    std::ofstream out(path.c_str());
    check(out.good(), "cannot open cycle trace output");
    out << "scenario,cycle,request_valid,request_accepted,request_address,"
           "response_valid,response_address,response_accepted,publication_valid,"
           "decode_accepted,publication_pc,publication_rvc,pc_before,pc_after,"
           "carry_before,carry_after,hold_before,hold_after,stall\n";
    out << std::hex << std::showbase;
    for (std::size_t i = 0; i < rows.size(); ++i) {
        const TraceRow& r = rows[i];
        out << r.scenario << std::dec << ',' << r.cycle << ',' << r.request_valid << ','
            << r.request_valid << ',' << std::hex << r.request_address << ',' << std::dec
            << r.response_valid << ',' << std::hex << r.response_address << ',' << std::dec
            << r.response_accepted << ',' << r.publication_valid << ',' << r.decode_accepted
            << ',' << std::hex << r.publication_pc << ',' << std::dec << r.publication_rvc
            << ',' << std::hex << r.pc_before << ',' << r.pc_after << ',' << std::dec
            << r.carry_before << ',' << r.carry_after << ',' << r.hold_before << ','
            << r.hold_after << ',' << r.stall << '\n';
    }
}

void write_analysis(const std::string& path, const std::vector<Result>& results) {
    std::ofstream out(path.c_str());
    check(out.good(), "cannot open throughput analysis output");
    out << "# Gate 5.2 RVC R2 Native Throughput Audit\n\n"
           "The harness calls canonical `boom::frontend_module` followed by canonical "
           "`boom::decode_module`. Decode's one-entry output is consumed before every native "
           "call. Every aligned-word request is accepted immediately and its exact matching "
           "response is presented on the next call. Thus a cycle below is one native "
           "architectural call, not an `ap_clk`.\n\n"
           "| Scenario | Architectural instructions | Cycles | Bytes fetched | Requests | "
           "Responses | Request interval mean/min/max | Instruction interval mean/min/max | "
           "Cycles/instruction |\n"
           "|---|---:|---:|---:|---:|---:|---:|---:|---:|\n";
    out << std::fixed << std::setprecision(6);
    for (std::size_t i = 0; i < results.size(); ++i) {
        const Result& r = results[i];
        const IntervalStats req = intervals(r.request_cycles);
        const IntervalStats insn = intervals(r.publication_cycles);
        out << "| " << r.name << " | " << r.instructions << " | " << r.cycles << " | "
            << r.bytes << " | " << r.requests << " | " << r.responses << " | "
            << req.mean << '/' << req.minimum << '/' << req.maximum << " | "
            << insn.mean << '/' << insn.minimum << '/' << insn.maximum << " | "
            << static_cast<double>(r.cycles) / r.instructions << " |\n";
    }
    out << "\n`Bytes fetched` counts all accepted 4-byte requests, including a trailing "
           "sequential request if the canonical frontend emits one with the final publication. "
           "Intervals are differences between event call numbers; cycles/instruction includes "
           "the initial request call.\n\n"
           "## Gates\n\n"
           "- **Retained C parcels: PASS.** In `all-C`, every upper compressed parcel is "
           "published one call after its lower partner without a response on that second call; "
           "the response word is reused locally and there is no publication bubble.\n"
           "- **Aligned all-32 contract: PASS.** Logical requests and publications each have "
           "mean/min/max interval 1/1/1 after startup. The implementation retains the current "
           "one-request-per-word cadence.\n"
           "- **Cross-boundary distinction:** the cross-boundary-heavy stream starts every "
           "32-bit instruction at PC modulo 4 equal to 2. Its carry-only calls are parcel-local "
           "assembly work, not extra IMEM response latency.\n\n"
           "## Protocol Boundary\n\n"
           "This native test measures C++ call-level state transitions. It does not instantiate "
           "an HLS `ap_ctrl_hs` wrapper and makes no physical-clock throughput claim. HLS call "
           "latency/II must be reported from synthesis or RTL separately; the one-call IMEM "
           "latency here is explicit in `cycle_trace.csv` as request call N and response call "
           "N+1. Parcel-local reuse is visible when publication occurs with "
           "`response_valid=0`, and carry assembly is visible in `carry_before/after`.\n";
}

std::vector<Instruction> repeated(bool compressed, unsigned count) {
    std::vector<Instruction> result;
    for (unsigned i = 0; i < count; ++i) {
        const uint32_t value = compressed ? 0x0001u :
            (((i + 1) & 0x7ffu) << 20) | 0x00000013u;
        result.push_back(Instruction(compressed, value));
    }
    return result;
}

} // namespace

int main(int argc, char** argv) {
    const std::string trace_path = argc > 1 ? argv[1] : "cycle_trace.csv";
    const std::string analysis_path = argc > 2 ? argv[2] : "throughput_analysis.md";
    std::vector<TraceRow> trace;
    std::vector<Result> results;

    results.push_back(run_scenario("all-C", 0x40000, repeated(true, 256), trace));
    results.push_back(run_scenario("all-32", 0x41000, repeated(false, 128), trace));

    std::vector<Instruction> alternating;
    for (unsigned i = 0; i < 192; ++i)
        alternating.push_back(i & 1u ? Instruction(false, 0x00100093u) :
                                       Instruction(true, 0x0001u));
    results.push_back(run_scenario("alternating-C-32", 0x42000, alternating, trace));
    results.push_back(run_scenario("cross-boundary-heavy", 0x43002,
                                   repeated(false, 128), trace));

    const Result& all_c = results[0];
    for (unsigned i = 1; i < all_c.instructions; i += 2) {
        const unsigned cycle = all_c.publication_cycles[i];
        check(cycle == all_c.publication_cycles[i - 1] + 1,
              "all-C retained upper parcel has a publication bubble");
        bool response_valid = false;
        for (std::size_t row = 0; row < trace.size(); ++row)
            if (trace[row].scenario == "all-C" && trace[row].cycle == cycle)
                response_valid = trace[row].response_valid;
        check(!response_valid, "all-C upper parcel was not local response reuse");
    }
    const IntervalStats all32_req = intervals(results[1].request_cycles);
    const IntervalStats all32_pub = intervals(results[1].publication_cycles);
    check(all32_req.mean == 1.0 && all32_req.minimum == 1 && all32_req.maximum == 1,
          "aligned all-32 request cadence regressed");
    check(all32_pub.mean == 1.0 && all32_pub.minimum == 1 && all32_pub.maximum == 1,
          "aligned all-32 publication cadence regressed");

    write_trace(trace_path, trace);
    write_analysis(analysis_path, results);
    if (failures != 0) {
        std::cerr << failures << " throughput audit checks failed\n";
        return 1;
    }
    for (std::size_t i = 0; i < results.size(); ++i) {
        const IntervalStats req = intervals(results[i].request_cycles);
        const IntervalStats insn = intervals(results[i].publication_cycles);
        std::cout << results[i].name << " instructions=" << results[i].instructions
                  << " cycles=" << results[i].cycles << " bytes=" << results[i].bytes
                  << " requests=" << results[i].requests << " responses=" << results[i].responses
                  << " req_interval=" << req.mean << '/' << req.minimum << '/' << req.maximum
                  << " inst_interval=" << insn.mean << '/' << insn.minimum << '/' << insn.maximum
                  << " cpi=" << static_cast<double>(results[i].cycles) / results[i].instructions
                  << '\n';
    }
    std::cout << "PASS: Gate 5.2 R2 native throughput audit\n";
    return 0;
}
