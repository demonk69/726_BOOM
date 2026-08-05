#include "boom_config.hpp"
#include "boom_state.hpp"
#include "completion.hpp"
#include <cstdint>
#include <cstdio>

namespace boom { void execute_module(BoomCoreState& state); }

static uint64_t reference(uint8_t uopc, uint64_t lhs, uint64_t rhs) {
    const unsigned __int128 product =
        (unsigned __int128)lhs * (unsigned __int128)rhs;
    if (uopc == 16) return (uint64_t)product;
    if (uopc == 17) {
        const __int128 signed_product =
            (__int128)(int64_t)lhs * (__int128)(int64_t)rhs;
        return (uint64_t)((unsigned __int128)signed_product >> 64);
    }
    if (uopc == 18) {
        uint64_t high = (uint64_t)(product >> 64);
        return lhs >> 63 ? high - rhs : high;
    }
    if (uopc == 19) return (uint64_t)(product >> 64);
    const uint64_t word = (uint64_t)(uint32_t)lhs * (uint64_t)(uint32_t)rhs;
    return (uint64_t)(int64_t)(int32_t)(uint32_t)word;
}

static bool run_vector(uint8_t uopc, uint64_t lhs, uint64_t rhs,
                       uint8_t rob_idx, uint32_t allocation_id) {
    BoomCoreState state;
    MicroOp& uop = state.issue.issued_uops[INT_ISSUE_LANE];
    state.issue.issued_valids[INT_ISSUE_LANE] = true;
    uop.uopc = uopc;
    uop.iq_type = IQ_ALU;
    uop.fu_code = FU_MUL;
    uop.rename.prs1 = 1;
    uop.rename.prs2 = 2;
    uop.rename.pdst = 3;
    uop.rename.dst_rtype = DST_INT;
    uop.queue.rob_idx = rob_idx;
    uop.queue.rob_allocation_id = allocation_id;
    boom::prf_seed(state, 1, lhs);
    boom::prf_seed(state, 2, rhs);
    boom::execute_module(state);

    const ExecuteState::AluResult first = state.execute.alu_results[INT_ISSUE_LANE];
    const uint64_t expected = reference(uopc, lhs, rhs);
    if (!first.valid || first.result != expected || first.uop.uopc != uopc ||
        first.uop.queue.rob_allocation_id != allocation_id) return false;

    state.issue.issued_uops[INT_ISSUE_LANE].uopc = 16;
    boom::prf_seed(state, 1, 9);
    boom::prf_seed(state, 2, 9);
    boom::execute_module(state);
    const ExecuteState::AluResult& held = state.execute.alu_results[INT_ISSUE_LANE];
    if (!held.valid || held.result != expected || held.uop.uopc != uopc) return false;

    CompletionEvent event;
    boom::completion_from_execute(held, COMPLETION_SOURCE_INT_EXECUTE, event);
    return event.valid && event.kind == COMPLETION_EXECUTE && event.writes_prf &&
        event.source == COMPLETION_SOURCE_INT_EXECUTE && event.value == expected &&
        event.uop.queue.rob_idx == rob_idx &&
        event.uop.queue.rob_allocation_id == allocation_id;
}

int main() {
    static const uint64_t operands[][2] = {
        {0, 0}, {1, 1}, {0xffffffffffffffffULL, 7},
        {0x8000000000000000ULL, 0xffffffffffffffffULL},
        {0x7fffffffffffffffULL, 0x7fffffffffffffffULL},
        {0x123456789abcdef0ULL, 0xfedcba9876543210ULL},
    };
    unsigned passed = 0;
    for (uint8_t uopc = 16; uopc <= 20; ++uopc) {
        for (unsigned i = 0; i < sizeof(operands) / sizeof(operands[0]); ++i) {
            if (!run_vector(uopc, operands[i][0], operands[i][1],
                            (uint8_t)((uopc + i) % ROB_DEPTH),
                            0x2000u + uopc * 16u + i)) {
                std::printf("FAIL uopc=%u vector=%u\n", uopc, i);
                return 1;
            }
            ++passed;
        }
    }
    std::printf("M2B directed execute integration: %u passed, 0 failed\n", passed);
    return 0;
}
