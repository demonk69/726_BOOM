#include "boom_config.hpp"
#include "boom_interfaces.hpp"
#include "boom_state.hpp"

#include <algorithm>
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
    bool reset_midstream;
    unsigned min_redirects;
    unsigned min_wraps;
    unsigned min_reuses;
};

struct Coverage {
    unsigned allocations;
    unsigned retires;
    unsigned reclaims;
    unsigned redirects;
    unsigned squashes;
    unsigned wraps;
    unsigned reuses;
    unsigned generation_rejects;
    unsigned exceptions;
    unsigned resets;
    unsigned max_occupancy;
    bool have_last_index;
    uint8_t last_index;
    bool seen_index[FTQ_DEPTH];
    uint32_t last_generation[FTQ_DEPTH];

    Coverage() : allocations(0), retires(0), reclaims(0), redirects(0),
        squashes(0), wraps(0), reuses(0), generation_rejects(0), exceptions(0),
        resets(0), max_occupancy(0), have_last_index(false), last_index(0) {
        for (unsigned i = 0; i < FTQ_DEPTH; ++i) {
            seen_index[i] = false;
            last_generation[i] = 0;
        }
    }

    void sample(const BoomCoreState& state) {
        const boom::FtqStepOutput& output = state.ftq_last_output;
        max_occupancy = std::max(max_occupancy, static_cast<unsigned>(output.count));
        retires += output.retire_accepted;
        reclaims += output.reclaimed;
        redirects += output.redirect_accepted;
        squashes += output.squash_accepted;
        generation_rejects += output.retire_rejected + output.squash_rejected +
            output.redirect_rejected;
        if (!output.alloc_accepted) return;
        ++allocations;
        if (have_last_index && output.alloc_ftq_idx < last_index) ++wraps;
        have_last_index = true;
        last_index = output.alloc_ftq_idx;
        if (seen_index[output.alloc_ftq_idx] &&
            last_generation[output.alloc_ftq_idx] != output.alloc_generation) ++reuses;
        seen_index[output.alloc_ftq_idx] = true;
        last_generation[output.alloc_ftq_idx] = output.alloc_generation;
    }
};

static std::string program_path(const char* name) {
    const char* build = std::getenv("PF3_PROGRAM_BUILD");
    const std::string root = build && *build ? build : "/tmp/boom_hls/pf3a/programs";
    return root + "/" + name + ".bin";
}

static bool load_image(const TestSpec& spec, std::vector<uint32_t>& words) {
    const std::string path = program_path(spec.name);
    std::ifstream input(path.c_str(), std::ios::binary);
    if (!input) {
        std::printf("FAIL %s cannot_open=%s\n", spec.name, path.c_str());
        return false;
    }
    const std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(input)),
                                     std::istreambuf_iterator<char>());
    for (size_t i = 0; i < bytes.size(); i += 4) {
        uint32_t word = 0x00000013u;
        for (unsigned lane = 0; lane < 4 && i + lane < bytes.size(); ++lane) {
            word &= ~(UINT32_C(0xff) << (8 * lane));
            word |= static_cast<uint32_t>(bytes[i + lane]) << (8 * lane);
        }
        words.push_back(word);
    }
    return !words.empty();
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
                pending.push_back(PendingLoad{request, cycle + 3});
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

template <typename T>
static void drain(T& stream) { while (!stream.empty()) (void)stream.read(); }

static void clear_pipe(PipeSignals& pipe) {
    drain(pipe.imem_req); drain(pipe.imem_resp); drain(pipe.dmem_req);
    drain(pipe.dmem_resp); drain(pipe.commit_trace);
}

static bool run_test(const TestSpec& spec) {
    std::vector<uint32_t> words;
    if (!load_image(spec, words)) return false;
    BoomCoreState state;
    PipeSignals pipe;
    state.product_ftq_enabled = true;
    InstructionMemory imem(words);
    DataMemory dmem;
    Coverage coverage;
    uint64_t final_registers[32] = {};
    bool register_written[32] = {};
    bool committed_tohost = false;
    bool did_reset = false;
    uint64_t exception_pc = 0;
    uint64_t exception_cause = 0;
    bool saw_exception = false;
    unsigned exception_cycle = 0;
    unsigned commits = 0;
    unsigned cycle = 0;
    for (; cycle < 20000 && !dmem.saw_tohost &&
           (!saw_exception || cycle < exception_cycle + 3); ++cycle) {
        imem.step(pipe, cycle);
        dmem.step(pipe, cycle);
        boom_core_step(state, pipe);
        coverage.sample(state);
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
                ++coverage.exceptions;
                saw_exception = true;
                exception_cycle = cycle;
                exception_pc = entry.pc;
                exception_cause = entry.exc_cause;
            }
        }
        dmem.step(pipe, cycle);
        if (spec.reset_midstream && !did_reset && cycle == 90) {
            clear_pipe(pipe);
            imem.pending.clear();
            dmem.pending.clear();
            dmem.saw_tohost = false;
            dmem.tohost_value = 0;
            state = BoomCoreState();
            state.product_ftq_enabled = true;
            final_registers[8] = final_registers[9] = 0;
            register_written[8] = register_written[9] = false;
            did_reset = true;
            ++coverage.resets;
        }
    }
    bool ok = true;
    if (spec.expect_trap) {
        ok = saw_exception && coverage.exceptions == 1 && exception_cause == 2 &&
            exception_pc == RESET_VECTOR + 4 && !dmem.saw_tohost;
    } else {
        ok = !state.io_trap && dmem.saw_tohost && dmem.tohost_value == 1 &&
            committed_tohost && state.tohost == 1;
    }
    for (size_t i = 0; i < spec.signature.size(); ++i) {
        const ExpectedRegister expected = spec.signature[i];
        ok = ok && register_written[expected.rd] && final_registers[expected.rd] == expected.value;
    }
    ok = ok && coverage.allocations > 0 && coverage.retires > 0 &&
        coverage.reclaims > 0 && coverage.redirects >= spec.min_redirects &&
        coverage.wraps >= spec.min_wraps && coverage.reuses >= spec.min_reuses &&
        coverage.resets == (spec.reset_midstream ? 1u : 0u);
    std::printf("PF3_EVENT program=%s alloc=%u retire=%u reclaim=%u redirect=%u squash=%u "
                "wrap=%u reuse=%u generation_reject=%u exception=%u reset=%u max_occupancy=%u "
                "commits=%u cycles=%u status=%s\n",
                spec.name, coverage.allocations, coverage.retires, coverage.reclaims,
                coverage.redirects, coverage.squashes, coverage.wraps, coverage.reuses,
                coverage.generation_rejects, coverage.exceptions, coverage.resets,
                coverage.max_occupancy, commits, cycle, ok ? "PASS" : "FAIL");
    return ok;
}

int main() {
    const std::vector<TestSpec> tests = {
        {"pf3_straight_commit", {{8, 11}, {9, 17}, {18, 28}, {19, 33}, {20, 42}}, false, false, 0, 0, 0},
        {"pf3_rvc_packets", {{8, 12}, {9, 4}, {18, 21}, {19, 33}}, false, false, 0, 0, 0},
        {"pf3_jal_mask", {{8, 9}, {9, 13}}, false, false, 0, 0, 0},
        {"pf3_conditional_shadow", {{8, 5}, {9, 5}, {18, 19}, {19, 23}}, false, false, 1, 0, 0},
        {"pf3_branch_squash", {{8, 1}, {9, 2}, {18, 21}, {19, 29}}, false, false, 1, 0, 0},
        {"pf3_exception_flush", {{8, 27}}, true, false, 0, 0, 0},
        {"pf3_ftq_wrap", {{8, 72}, {9, 78}}, false, false, 0, 1, 1},
        {"pf3_generation_reuse", {{8, 76}, {9, 79}}, false, false, 0, 1, 1},
        {"pf3_rv64m", {{18, 126}, {19, 131}, {20, 786}}, false, false, 0, 0, 0},
        {"pf3_mixed_control", {{18, 7}, {19, 15}, {20, 17}}, false, false, 1, 0, 0},
        {"pf3_long_stream", {{8, 160}, {9, 161}, {18, 162}}, false, false, 0, 2, 32},
        {"pf3_reset_midstream", {{8, 48}, {9, 52}}, false, true, 0, 1, 1},
    };
    unsigned passed = 0;
    for (size_t i = 0; i < tests.size(); ++i) if (run_test(tests[i])) ++passed;
    std::printf("PF3_PRODUCT_PROGRAMS %u/%u %s\n", passed,
                static_cast<unsigned>(tests.size()), passed == tests.size() ? "PASS" : "FAIL");
    return passed == tests.size() ? 0 : 1;
}
