#include "boom_config.hpp"
#include "boom_interfaces.hpp"
#include "boom_state.hpp"
#include "rvc.hpp"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <string>
#include <vector>

extern void boom_core_step(BoomCoreState& state, PipeSignals& pipe);

static const uint64_t TOHOST = UINT64_C(0x80000080);

struct ExpectedRegister { uint8_t rd; uint64_t value; };
struct TestSpec {
    const char* name;
    std::vector<ExpectedRegister> signature;
    bool expect_trap;
    bool expect_atomic_hold;
};

static std::string program_path(const char* name, const char* extension) {
    const char* environment_root = std::getenv("HLS_PROJECT_ROOT");
    if (environment_root && *environment_root)
        return std::string(environment_root) + "/tb/programs/b3i_packet/build/" + name + extension;
    const std::string source = __FILE__;
    const std::string marker = "/tb/differential/";
    const std::string::size_type at = source.rfind(marker);
    const std::string root = at == std::string::npos ? "." : source.substr(0, at);
    return root + "/tb/programs/b3i_packet/build/" + name + extension;
}

static bool load_image(const TestSpec& spec, std::vector<uint32_t>& words) {
    const std::string path = program_path(spec.name, ".bin");
    std::ifstream input(path.c_str(), std::ios::binary);
    if (!input) {
        std::printf("FAIL %-26s cannot open %s\n", spec.name, path.c_str());
        return false;
    }
    const std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(input)),
                                     std::istreambuf_iterator<char>());
    if (bytes.empty()) return false;
    for (size_t i = 0; i < bytes.size(); i += 4) {
        uint32_t word = 0x00000013u;
        for (unsigned b = 0; b < 4 && i + b < bytes.size(); ++b) {
            word &= ~(UINT32_C(0xff) << (8 * b));
            word |= static_cast<uint32_t>(bytes[i + b]) << (8 * b);
        }
        words.push_back(word);
    }
    unsigned compressed = 0;
    unsigned uncompressed = 0;
    unsigned illegal = 0;
    for (size_t pc = 0; pc + 1 < bytes.size();) {
        const uint16_t parcel = static_cast<uint16_t>(bytes[pc]) |
            (static_cast<uint16_t>(bytes[pc + 1]) << 8);
        if ((parcel & 3u) != 3u) {
            const bool legal = boom::decompress_rvc(parcel).legal;
            if (!legal) {
                if (!spec.expect_trap || pc != 2 || parcel != 0) {
                    std::printf("FAIL %-26s unexpected illegal parcel %04x at +0x%zx\n",
                                spec.name, parcel, pc);
                    return false;
                }
                ++illegal;
            } else {
                ++compressed;
            }
            pc += 2;
        } else {
            if (pc + 3 >= bytes.size()) return false;
            ++uncompressed;
            pc += 4;
        }
    }
    if (compressed == 0 || uncompressed == 0 || illegal != (spec.expect_trap ? 1u : 0u)) {
        std::printf("FAIL %-26s image coverage C=%u I32=%u illegal=%u\n",
                    spec.name, compressed, uncompressed, illegal);
        return false;
    }
    std::printf("IMAGE %-25s bytes=%u legal_C=%u I32=%u illegal_C=%u\n", spec.name,
                static_cast<unsigned>(bytes.size()), compressed, uncompressed, illegal);
    return true;
}

struct PendingImem { ImemRequest request; unsigned ready_cycle; };
struct InstructionMemory {
    const std::vector<uint32_t>& words;
    std::vector<PendingImem> pending;
    explicit InstructionMemory(const std::vector<uint32_t>& image) : words(image) {}
    void step(PipeSignals& pipe, unsigned cycle) {
        while (!pipe.imem_req.empty())
            pending.push_back(PendingImem{pipe.imem_req.read(), cycle + 2});
        if (pending.empty() || pending.front().ready_cycle > cycle || pipe.imem_resp.full()) return;
        const ImemRequest request = pending.front().request;
        pending.erase(pending.begin());
        ImemResponse response;
        response.address = request.address;
        response.fetch_id = request.fetch_id;
        response.epoch = request.epoch;
        const uint64_t index = request.address >= RESET_VECTOR ?
            (request.address - RESET_VECTOR) >> 2 : UINT64_MAX;
        response.instruction = index < words.size() ? words[static_cast<size_t>(index)] : 0x00000013u;
        pipe.imem_resp.write(response);
    }
};

struct PendingLoad { DmemRequest request; unsigned ready_cycle; };
struct DataMemory {
    std::map<uint64_t, uint8_t> bytes;
    std::vector<PendingLoad> pending;
    bool saw_tohost;
    uint64_t tohost_value;
    unsigned load_delay;
    explicit DataMemory(unsigned delay) : saw_tohost(false), tohost_value(0), load_delay(delay) {}
    uint64_t read64(uint64_t address) const {
        uint64_t value = 0;
        for (unsigned i = 0; i < 8; ++i) {
            std::map<uint64_t, uint8_t>::const_iterator it = bytes.find(address + i);
            if (it != bytes.end()) value |= static_cast<uint64_t>(it->second) << (8 * i);
        }
        return value;
    }
    void step(PipeSignals& pipe, unsigned cycle) {
        if (!pending.empty() && pending.front().ready_cycle <= cycle && !pipe.dmem_resp.full()) {
            DmemResponse response;
            response.transaction_id = pending.front().request.transaction_id;
            response.data = response.read_data = read64(pending.front().request.address);
            pipe.dmem_resp.write(response);
            pending.erase(pending.begin());
        }
        while (!pipe.dmem_req.empty()) {
            const DmemRequest request = pipe.dmem_req.read();
            if (!request.is_store) {
                pending.push_back(PendingLoad{request, cycle + load_delay});
                continue;
            }
            const uint8_t mask = request.write_mask ? request.write_mask : request.mask;
            for (unsigned i = 0; i < 8; ++i)
                if (mask & (1u << i)) bytes[request.address + i] =
                    static_cast<uint8_t>(request.write_data >> (8 * i));
            if (request.address == TOHOST) {
                saw_tohost = true;
                tohost_value = request.write_data;
            }
        }
    }
};

static bool same_packet(const boom::FetchPacket& a, const boom::FetchPacket& b) {
    if (a.valid != b.valid || a.valid_mask != b.valid_mask) return false;
    for (unsigned i = 0; i < FETCH_PACKET_WIDTH; ++i) {
        const boom::FetchInstruction& x = a.slots[i];
        const boom::FetchInstruction& y = b.slots[i];
        if (x.pc != y.pc || x.instruction != y.instruction ||
            x.original_instruction != y.original_instruction || x.fetch_id != y.fetch_id ||
            x.exception != y.exception || x.exception_cause != y.exception_cause ||
            x.is_rvc != y.is_rvc) return false;
    }
    return true;
}

static bool run_test(const TestSpec& spec) {
    std::vector<uint32_t> words;
    if (!load_image(spec, words)) return false;
    BoomCoreState state;
    PipeSignals pipe;
    InstructionMemory imem(words);
    DataMemory dmem(spec.expect_atomic_hold ? 180 : 3);
    uint64_t final_registers[32] = {};
    bool register_written[32] = {};
    bool committed_tohost = false;
    bool saw_exception = false;
    uint64_t exception_pc = 0;
    uint64_t exception_cause = 0;
    unsigned commits = 0;
    unsigned held_cycles = 0;
    unsigned max_held_cycles = 0;
    boom::FetchPacket previous;
    bool previous_valid = false;
    unsigned cycles = 0;
    for (; cycles < 8000 && !dmem.saw_tohost && !state.io_trap; ++cycles) {
        imem.step(pipe, cycles);
        dmem.step(pipe, cycles);
        boom_core_step(state, pipe);
        const boom::FetchPacket& packet = state.frontend.pending_packet;
        if (packet.valid && packet.valid_mask == 3 && previous_valid && same_packet(packet, previous)) {
            ++held_cycles;
            if (held_cycles > max_held_cycles) max_held_cycles = held_cycles;
        } else {
            held_cycles = 0;
        }
        previous = packet;
        previous_valid = packet.valid && packet.valid_mask == 3;
        while (!pipe.commit_trace.empty()) {
            const CommitEntry entry = pipe.commit_trace.read();
            ++commits;
            if (entry.rd_valid && entry.rd < 32) {
                final_registers[entry.rd] = entry.rd_value;
                register_written[entry.rd] = true;
            }
            if (entry.is_store && entry.memory_address == TOHOST && entry.memory_data == 1)
                committed_tohost = true;
            if (entry.exception) {
                saw_exception = true;
                exception_pc = entry.pc;
                exception_cause = entry.exc_cause;
            }
        }
        dmem.step(pipe, cycles);
    }
    bool ok = true;
    if (spec.expect_trap) {
        ok = state.io_trap && saw_exception && exception_cause == 2 &&
            exception_pc == RESET_VECTOR + 2 && !dmem.saw_tohost && !committed_tohost &&
            !register_written[9];
    } else {
        ok = !state.io_trap && dmem.saw_tohost && dmem.tohost_value == 1 &&
            committed_tohost && state.tohost == 1;
    }
    for (size_t i = 0; i < spec.signature.size(); ++i) {
        const ExpectedRegister expected = spec.signature[i];
        ok = ok && register_written[expected.rd] && final_registers[expected.rd] == expected.value;
    }
    if (spec.expect_atomic_hold) ok = ok && max_held_cycles >= 2;
    if (!ok) {
        std::printf("FAIL %-26s cycles=%u commits=%u trap=%d cause=%llu pc=%llx "
                    "tohost=%d/%llu atomic_hold=%u\n", spec.name, cycles, commits, state.io_trap,
                    (unsigned long long)exception_cause, (unsigned long long)exception_pc,
                    dmem.saw_tohost, (unsigned long long)dmem.tohost_value, max_held_cycles);
        return false;
    }
    std::printf("PASS %-26s cycles=%u commits=%u signature=%u trap=%d atomic_hold=%u\n",
                spec.name, cycles, commits, static_cast<unsigned>(spec.signature.size()),
                spec.expect_trap, max_held_cycles);
    return true;
}

int main() {
    const std::vector<TestSpec> tests = {
        {"packet_two_rvc", {{8, 12}, {9, 21}}, false, false},
        {"packet_rvc_rvc_long", {{8, 3}, {9, 4}, {10, 7}}, false, false},
        {"packet_carry_plus_rvc", {{8, 6}, {9, 17}, {10, 18}}, false, false},
        {"packet_atomic_backpressure", {{8, 40}, {9, 19}, {10, 59}}, false, true},
        {"packet_redirect", {{8, 3}, {9, 7}, {10, 10}}, false, false},
        {"packet_fault", {{8, 13}}, true, false},
    };
    unsigned passed = 0;
    for (size_t i = 0; i < tests.size(); ++i) if (run_test(tests[i])) ++passed;
    std::printf("GATE5_3_B3I_FULL_CORE %u/%u %s\n", passed,
                static_cast<unsigned>(tests.size()), passed == tests.size() ? "PASS" : "FAIL");
    return passed == tests.size() ? 0 : 1;
}
