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
};

static std::string program_path(const char* name, const char* extension) {
    const char* environment_root = std::getenv("HLS_PROJECT_ROOT");
    if (environment_root && *environment_root)
        return std::string(environment_root) + "/tb/programs/rvc_fetch/build/" + name + extension;
    std::string source = __FILE__;
    const std::string marker = "/tb/differential/";
    const std::string::size_type at = source.rfind(marker);
    const std::string root = at == std::string::npos ? "." : source.substr(0, at);
    return root + "/tb/programs/rvc_fetch/build/" + name + extension;
}

static bool load_image(const TestSpec& spec, std::vector<uint8_t>& bytes,
                       std::vector<uint32_t>& words) {
    const std::string path = program_path(spec.name, ".bin");
    std::ifstream input(path.c_str(), std::ios::binary);
    if (!input) {
        std::printf("FAIL %-24s cannot open %s\n", spec.name, path.c_str());
        return false;
    }
    bytes.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
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
    for (size_t pc = 0; pc + 1 < bytes.size();) {
        const uint16_t parcel = static_cast<uint16_t>(bytes[pc]) |
                                (static_cast<uint16_t>(bytes[pc + 1]) << 8);
        if ((parcel & 3u) != 3u) {
            const boom::RvcDecodeResult decoded = boom::decompress_rvc(parcel);
            if (!decoded.legal) {
                std::printf("FAIL %-24s illegal compressed parcel %04x at +0x%zx\n",
                            spec.name, parcel, pc);
                return false;
            }
            ++compressed;
            pc += 2;
        } else {
            if (pc + 3 >= bytes.size()) {
                std::printf("FAIL %-24s truncated 32-bit instruction at +0x%zx\n", spec.name, pc);
                return false;
            }
            ++uncompressed;
            pc += 4;
        }
    }
    if (compressed == 0 || uncompressed == 0) {
        std::printf("FAIL %-24s mixed-width coverage C=%u I32=%u\n",
                    spec.name, compressed, uncompressed);
        return false;
    }
    std::printf("IMAGE %-23s bytes=%u C=%u I32=%u\n", spec.name,
                static_cast<unsigned>(bytes.size()), compressed, uncompressed);
    return true;
}

struct PendingImem { ImemRequest request; unsigned ready_cycle; };
struct InstructionMemory {
    const std::vector<uint32_t>& words;
    std::vector<PendingImem> pending;
    explicit InstructionMemory(const std::vector<uint32_t>& image) : words(image) {}
    void step(PipeSignals& pipe, unsigned cycle) {
        while (!pipe.imem_req.empty()) {
            PendingImem item = {pipe.imem_req.read(), cycle + 2};
            pending.push_back(item);
        }
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
    DataMemory() : saw_tohost(false), tohost_value(0) {}
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
                PendingLoad load = {request, cycle + 3};
                pending.push_back(load);
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

static bool run_test(const TestSpec& spec) {
    std::vector<uint8_t> bytes;
    std::vector<uint32_t> words;
    if (!load_image(spec, bytes, words)) return false;
    BoomCoreState state;
    PipeSignals pipe;
    InstructionMemory imem(words);
    DataMemory dmem;
    std::vector<CommitEntry> commits;
    uint64_t final_registers[32] = {};
    bool register_written[32] = {};
    bool committed_tohost = false;
    unsigned cycles = 0;
    for (; cycles < 5000 && !dmem.saw_tohost && !state.io_trap; ++cycles) {
        imem.step(pipe, cycles);
        dmem.step(pipe, cycles);
        boom_core_step(state, pipe);
        while (!pipe.commit_trace.empty()) {
            const CommitEntry entry = pipe.commit_trace.read();
            commits.push_back(entry);
            if (entry.rd_valid && entry.rd < 32) {
                final_registers[entry.rd] = entry.rd_value;
                register_written[entry.rd] = true;
            }
            if (entry.is_store && entry.memory_address == TOHOST && entry.memory_data == 1)
                committed_tohost = true;
        }
        dmem.step(pipe, cycles);
    }
    if (state.io_trap || !dmem.saw_tohost || dmem.tohost_value != 1 ||
        !committed_tohost || state.tohost != 1) {
        std::printf("FAIL %-24s tohost/trap memory=%d/%llu commit=%d state=%llu cycles=%u\n",
                    spec.name, dmem.saw_tohost, (unsigned long long)dmem.tohost_value,
                    committed_tohost, (unsigned long long)state.tohost, cycles);
        return false;
    }
    for (size_t e = 0; e < spec.signature.size(); ++e) {
        const uint8_t rd = spec.signature[e].rd;
        if (!register_written[rd] || final_registers[rd] != spec.signature[e].value) {
            std::printf("FAIL %-24s final x%u=%016llx expected=%016llx\n", spec.name,
                        rd, (unsigned long long)final_registers[rd],
                        (unsigned long long)spec.signature[e].value);
            return false;
        }
    }
    std::printf("PASS %-24s cycles=%u commits=%u signature=%u tohost=1\n",
                spec.name, cycles, static_cast<unsigned>(commits.size()),
                static_cast<unsigned>(spec.signature.size()));
    return true;
}

int main() {
    const std::vector<TestSpec> tests = {
        {"rvc_addi", {{8, 12}, {9, 15}}},
        {"rvc_load_store", {{9, 46}, {10, 46}, {11, 47}}},
        {"rvc_branch", {{9, 9}, {10, 13}}},
        {"rvc_jump", {{8, 7}, {9, 18}}},
        {"rvc_word_ops", {{8, 12}, {10, UINT64_C(0xfffffffffffffff8)}}},
        {"rvc_mixed_16_32", {{8, 19}, {9, 13}, {10, 30}}},
        {"rvc_cross_boundary", {{8, 10}, {9, 21}, {10, 31}}},
        {"rvc_rv64m_mix", {{10, 42}, {11, 8}}},
        {"rvc_redirect_halfword", {{8, 23}, {9, 27}}},
        {"rvc_tohost", {{8, 15}, {9, 31}}},
        {"rvc_decode_gaps", {{8, UINT64_C(0xffffffff)}, {9, 2}}}
    };
    unsigned passed = 0;
    for (size_t i = 0; i < tests.size(); ++i) if (run_test(tests[i])) ++passed;
    std::printf("GATE5_2_R2_FULL_CORE_RVC %u/%u %s\n", passed,
                static_cast<unsigned>(tests.size()), passed == tests.size() ? "PASS" : "FAIL");
    return passed == tests.size() ? 0 : 1;
}
