#include "boom_config.hpp"
#include "boom_state.hpp"
#include "boom_interfaces.hpp"
#include <cstdint>
#include <cstdio>
#include <vector>

extern void boom_core_step(BoomCoreState& state, PipeSignals& pipe);

static uint32_t addi(int32_t imm, uint8_t rs1, uint8_t rd) {
    return ((uint32_t)imm & 0xfff) << 20 | rs1 << 15 | rd << 7 | 0x13;
}

static uint32_t mul(uint8_t funct3, uint8_t rs2, uint8_t rs1, uint8_t rd,
                    bool word = false) {
    return 1u << 25 | rs2 << 20 | rs1 << 15 | funct3 << 12 | rd << 7 |
        (word ? 0x3b : 0x33);
}

struct CommitValue { uint8_t rd; uint64_t value; };

int main() {
    const uint32_t program[] = {
        addi(-3, 0, 1), addi(7, 0, 2),
        mul(0, 2, 1, 3), mul(1, 2, 1, 4), mul(2, 2, 1, 5),
        mul(3, 2, 1, 6), mul(0, 2, 1, 7, true),
        addi(1, 3, 8), addi(1, 4, 9), addi(1, 5, 10),
        addi(1, 6, 11), addi(1, 7, 12), 0x00000073,
    };
    BoomCoreState state;
    PipeSignals pipe;
    std::vector<CommitValue> commits;
    for (unsigned cycle = 0; cycle < 400 && !state.io_success && !state.io_trap; ++cycle) {
        if (!pipe.imem_req.empty()) {
            const ImemRequest request = pipe.imem_req.read();
            const uint64_t index = (request.address - RESET_VECTOR) >> 2;
            ImemResponse response;
            response.address = request.address;
            response.fetch_id = request.fetch_id;
            response.instruction = index < sizeof(program) / sizeof(program[0]) ?
                program[index] : 0x00000073;
            pipe.imem_resp.write(response);
        }
        boom_core_step(state, pipe);
        while (!pipe.commit_trace.empty()) {
            const CommitEntry entry = pipe.commit_trace.read();
            if (entry.rd_valid) commits.push_back({entry.rd, entry.rd_value});
        }
    }
    if (!state.io_success || state.io_trap) return 1;
    const CommitValue expected[] = {
        {3, (uint64_t)-21}, {4, UINT64_MAX}, {5, UINT64_MAX}, {6, 6},
        {7, (uint64_t)-21}, {8, (uint64_t)-20}, {9, 0}, {10, 0},
        {11, 7}, {12, (uint64_t)-20},
    };
    for (unsigned i = 0; i < sizeof(expected) / sizeof(expected[0]); ++i) {
        bool found = false;
        for (unsigned j = 0; j < commits.size(); ++j)
            if (commits[j].rd == expected[i].rd && commits[j].value == expected[i].value)
                found = true;
        if (!found) {
            std::printf("FAIL rd=%u expected=%016llx\n", expected[i].rd,
                        (unsigned long long)expected[i].value);
            return 1;
        }
    }
    std::printf("M2B full-core multiply program: 10/10 architectural checks PASS\n");
    return 0;
}
