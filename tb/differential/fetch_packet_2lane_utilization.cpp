#include "boom_interfaces.hpp"
#include "boom_state.hpp"

#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <vector>

namespace boom {
void frontend_module(BoomCoreState&, PipeSignals&);
void decode_module(BoomCoreState&);
}

namespace {

struct Instruction {
    bool compressed;
    uint32_t bits;
    Instruction(bool c, uint32_t b) : compressed(c), bits(b) {}
};

struct Result {
    std::string scenario;
    uint64_t instructions;
    uint64_t rvc;
    uint64_t native_calls;
    uint64_t requests;
    uint64_t responses;
    uint64_t packets;
    uint64_t mask00;
    uint64_t mask01;
    uint64_t mask11;
    uint64_t valid_slots;
    uint64_t fetched_bytes;
    uint64_t useful_bytes;
    uint64_t b2_calls;
};

uint64_t failures = 0;

void check(bool condition, const std::string& message) {
    if (!condition) {
        ++failures;
        if (failures <= 20) std::cerr << "FAIL: " << message << '\n';
    }
}

uint32_t word_at(const std::map<uint64_t, uint16_t>& memory, uint64_t address) {
    std::map<uint64_t, uint16_t>::const_iterator lo = memory.find(address);
    std::map<uint64_t, uint16_t>::const_iterator hi = memory.find(address + 2);
    const uint16_t low = lo == memory.end() ? 0x0001u : lo->second;
    const uint16_t high = hi == memory.end() ? 0x0001u : hi->second;
    return static_cast<uint32_t>(low) | (static_cast<uint32_t>(high) << 16);
}

uint64_t run_b2_single_producer(unsigned instructions) {
    bool producer = false;
    bool buffer = false;
    unsigned generated = 0;
    unsigned retired = 0;
    uint64_t calls = 0;
    while (retired < instructions) {
        ++calls;
        if (buffer) {
            ++retired;
            buffer = false;
        }
        if (producer && !buffer) {
            buffer = true;
            producer = false;
        }
        if (!producer && generated < instructions) {
            producer = true;
            ++generated;
        }
    }
    return calls;
}

Result run(const std::string& name, uint64_t start,
           const std::vector<Instruction>& program) {
    std::map<uint64_t, uint16_t> memory;
    std::vector<uint64_t> expected_pc;
    uint64_t address = start;
    uint64_t rvc = 0;
    for (std::size_t i = 0; i < program.size(); ++i) {
        expected_pc.push_back(address);
        memory[address] = static_cast<uint16_t>(program[i].bits);
        if (program[i].compressed) {
            address += 2;
            ++rvc;
        } else {
            memory[address + 2] = static_cast<uint16_t>(program[i].bits >> 16);
            address += 4;
        }
    }

    BoomCoreState state;
    PipeSignals pipe;
    state.frontend.reset_done = true;
    state.frontend.pc = start;
    bool response_pending = false;
    ImemResponse pending;
    uint64_t decoded = 0;
    Result result = {name, program.size(), rvc, 0, 0, 0, 0, 0, 0, 0, 0,
                     0, address - start, 0};

    const uint64_t timeout = program.size() * 6 + 64;
    while (decoded < program.size() && result.native_calls < timeout) {
        const bool delivered = response_pending;
        if (delivered) {
            pipe.imem_resp.write(pending);
            response_pending = false;
            ++result.responses;
        }
        const bool response_was_held = state.frontend.response_received;
        const uint64_t parse_anchor = state.frontend.halfword_valid ?
            state.frontend.halfword_pc : state.frontend.pc;
        state.decode.dec_valids[0] = false;
        state.rename.dispatch_packets[0] = RenameDispatchPacket();
        boom::frontend_module(state, pipe);
        const bool publication = state.frontend.fetch_packet_valid;
        const uint64_t publication_pc = publication ? state.frontend.fetch_uop.debug_pc : 0;
        boom::decode_module(state);
        ++result.native_calls;

        if (publication) {
            check(decoded < expected_pc.size(), name + ": excess publication");
            if (decoded < expected_pc.size())
                check(publication_pc == expected_pc[decoded], name + ": publication order");
            ++decoded;
        }

        if ((delivered || response_was_held) && !state.frontend.response_received) {
            const boom::FetchPacket& packet = state.frontend.pending_packet;
            const uint8_t raw_mask = packet.valid ? packet.valid_mask : 0;
            check(raw_mask == 0 || raw_mask == 1 || raw_mask == 3,
                  name + ": illegal packet mask");
            unsigned in_range = 0;
            for (unsigned lane = 0; lane < 2; ++lane)
                if (((raw_mask >> lane) & 1u) != 0 &&
                    packet.slots[lane].pc >= start && packet.slots[lane].pc < address)
                    ++in_range;
            if (raw_mask == 0 && parse_anchor >= start && parse_anchor < address)
                ++result.mask00;
            if (in_range == 1) {
                ++result.mask01;
                ++result.valid_slots;
            }
            if (in_range == 2) {
                ++result.mask11;
                result.valid_slots += 2;
            }
            if (in_range != 0) ++result.packets;
        }

        while (!pipe.imem_req.empty()) {
            const ImemRequest request = pipe.imem_req.read();
            check(!response_pending, name + ": multiple requests outstanding");
            pending.address = request.address;
            pending.fetch_id = request.fetch_id;
            pending.epoch = request.epoch;
            pending.instruction = word_at(memory, request.address);
            pending.exception = false;
            pending.exc_cause = 0;
            response_pending = true;
            ++result.requests;
        }
    }
    check(decoded == program.size(), name + ": native-call timeout");
    result.fetched_bytes = result.requests * 4;
    // Execute the accepted B2 one-token producer/buffer/decode schedule as the
    // scalar baseline; this is a call-level model, not the removed B2 source.
    result.b2_calls = run_b2_single_producer(result.instructions);
    return result;
}

std::vector<Instruction> repeated(bool compressed, unsigned count,
                                  uint32_t bits = 0x0001u) {
    std::vector<Instruction> result;
    for (unsigned i = 0; i < count; ++i) {
        const uint32_t value = compressed ? bits :
            (bits == 0x0001u ? (0x00000013u | (((i + 1) & 0x7ffu) << 20)) : bits);
        result.push_back(Instruction(compressed, value));
    }
    return result;
}

void write_packet_csv(const std::string& path, const std::vector<Result>& results) {
    std::ofstream out(path.c_str());
    check(out.good(), "open packet_utilization.csv");
    out << "scenario,instructions,rvc_instructions,native_calls,requests,responses,packets,"
           "mask_00,mask_01,mask_11,valid_slots,packet_capacity_slots,packet_slot_utilization,"
           "useful_bytes,fetched_bytes,fetch_byte_utilization,native_call_qualified\n";
    out << std::fixed << std::setprecision(6);
    for (std::size_t i = 0; i < results.size(); ++i) {
        const Result& r = results[i];
        const double slots = r.packets ? static_cast<double>(r.valid_slots) / (2 * r.packets) : 0;
        const double bytes = r.fetched_bytes ?
            static_cast<double>(r.useful_bytes) / r.fetched_bytes : 0;
        out << r.scenario << ',' << r.instructions << ',' << r.rvc << ',' << r.native_calls
            << ',' << r.requests << ',' << r.responses << ',' << r.packets << ',' << r.mask00
            << ',' << r.mask01 << ',' << r.mask11 << ',' << r.valid_slots << ','
            << 2 * r.packets << ',' << slots << ',' << r.useful_bytes << ','
            << r.fetched_bytes << ',' << bytes << ",true\n";
    }
}

void write_comparison_csv(const std::string& path, const std::vector<Result>& results) {
    std::ofstream out(path.c_str());
    check(out.good(), "open throughput_comparison.csv");
    out << "scenario,instructions,b2_single_producer_native_calls,b3i_native_calls,"
           "b2_instructions_per_native_call,b3i_instructions_per_native_call,"
           "measured_native_call_rate_ratio,end_to_end_speedup_claim,"
           "b3i_valid_slots_per_packet,b3i_packets_per_response,physical_clock_claim\n";
    out << std::fixed << std::setprecision(6);
    for (std::size_t i = 0; i < results.size(); ++i) {
        const Result& r = results[i];
        const double b2_rate = static_cast<double>(r.instructions) / r.b2_calls;
        const double b3_rate = static_cast<double>(r.instructions) / r.native_calls;
        const double slots_per_packet = r.packets ?
            static_cast<double>(r.valid_slots) / r.packets : 0;
        const double packets_per_response = r.responses ?
            static_cast<double>(r.packets) / r.responses : 0;
        out << r.scenario << ',' << r.instructions << ',' << r.b2_calls << ','
            << r.native_calls << ',' << b2_rate << ',' << b3_rate << ','
            << b3_rate / b2_rate << ",none_one_wide_decode," << slots_per_packet
            << ',' << packets_per_response << ",none\n";
    }
}

void write_analysis(const std::string& path, const std::vector<Result>& results) {
    std::ofstream out(path.c_str());
    check(out.good(), "open throughput_analysis.md");
    out << "# Gate 5.3 B3I Native Packet Utilization\n\n"
           "This audit calls the canonical `boom::frontend_module` and then the canonical "
           "`boom::decode_module` once per counted native call. IMEM responses are returned on "
           "the native call after their request. A native call is a C++ state transition, not an "
           "HLS transaction clock or physical `ap_clk`; this report makes no RTL throughput claim.\n\n"
           "The B2 comparison is the accepted one-token scalar-producer schedule under the same "
           "zero-downstream-stall assumption. B3I still has one-wide Decode, so two packet lanes "
           "improve response unpacking and buffer admission rather than backend width. Therefore "
           "B3I claims no end-to-end native-call speedup; startup effects make the measured rate "
           "ratio slightly below one in these finite campaigns.\n\n"
           "| Scenario | Instructions | RVC | Native calls | Masks 00/01/11 | Packet slot util. | "
           "Fetch byte util. | B2 calls | B3I inst/call |\n"
           "|---|---:|---:|---:|---:|---:|---:|---:|---:|\n";
    out << std::fixed << std::setprecision(3);
    for (std::size_t i = 0; i < results.size(); ++i) {
        const Result& r = results[i];
        out << "| " << r.scenario << " | " << r.instructions << " | " << r.rvc << " | "
            << r.native_calls << " | " << r.mask00 << '/' << r.mask01 << '/' << r.mask11
            << " | " << (r.packets ? static_cast<double>(r.valid_slots) / (2 * r.packets) : 0)
            << " | " << (r.fetched_bytes ? static_cast<double>(r.useful_bytes) /
                           r.fetched_bytes : 0)
            << " | " << r.b2_calls << " | "
            << static_cast<double>(r.instructions) / r.native_calls << " |\n";
    }
    out << "\nPacket masks and valid slots include only lanes whose PCs are within the intended "
           "program range. Trailing frontend prefetch remains included in request, response, and "
           "fetched-byte totals but cannot inflate packet production. `mask_00` records in-range "
           "carry-only responses, `mask_01` one complete instruction, and "
           "`mask_11` two complete instructions. No `mask_10` is legal. `useful_bytes` is the "
           "architectural stream size; fetched bytes include aligned and trailing native requests.\n";
}

}  // namespace

int main(int argc, char** argv) {
    const std::string packet_path = argc > 1 ? argv[1] : "packet_utilization.csv";
    const std::string comparison_path = argc > 2 ? argv[2] : "throughput_comparison.csv";
    const std::string analysis_path = argc > 3 ? argv[3] : "throughput_analysis.md";
    std::vector<Result> results;
    results.push_back(run("all-C", 0x40000, repeated(true, 512)));
    results.push_back(run("all-32", 0x42000, repeated(false, 256)));

    std::vector<Instruction> alternating;
    for (unsigned i = 0; i < 384; ++i)
        alternating.push_back((i & 1u) ? Instruction(false, 0x00108093u) :
                                        Instruction(true, 0x0001u));
    results.push_back(run("alternating", 0x44000, alternating));

    std::vector<Instruction> mostly_rvc;
    for (unsigned i = 0; i < 512; ++i)
        mostly_rvc.push_back((i & 3u) == 3u ? Instruction(false, 0x00108093u) :
                                             Instruction(true, 0x0085u));
    results.push_back(run("75-percent-RVC", 0x48000, mostly_rvc));
    results.push_back(run("cross-word-heavy", 0x4c002, repeated(false, 256)));

    std::vector<Instruction> branches;
    for (unsigned i = 0; i < 384; ++i)
        branches.push_back((i & 3u) ? Instruction(true, 0xc001u) :
                                     Instruction(false, 0x00000063u));
    results.push_back(run("branch-heavy", 0x50000, branches));

    write_packet_csv(packet_path, results);
    write_comparison_csv(comparison_path, results);
    write_analysis(analysis_path, results);
    if (failures) {
        std::cerr << "GATE5_3_B3I_UTILIZATION_FAIL failures=" << failures << '\n';
        return EXIT_FAILURE;
    }
    for (std::size_t i = 0; i < results.size(); ++i)
        std::cout << results[i].scenario << " instructions=" << results[i].instructions
                  << " native_calls=" << results[i].native_calls
                  << " masks=" << results[i].mask00 << '/' << results[i].mask01 << '/'
                  << results[i].mask11 << " valid_slots=" << results[i].valid_slots
                  << " fetched_bytes=" << results[i].fetched_bytes << '\n';
    std::cout << "GATE5_3_B3I_UTILIZATION_PASS scenarios=6 native_call_qualified=true\n";
    return EXIT_SUCCESS;
}
