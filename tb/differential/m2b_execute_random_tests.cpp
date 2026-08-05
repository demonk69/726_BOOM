#include "boom_config.hpp"
#include "boom_state.hpp"
#include <cstdint>
#include <cstdio>

namespace boom { void execute_module(BoomCoreState& state); }

static uint64_t next(uint64_t& state) {
    state ^= state >> 12;
    state ^= state << 25;
    state ^= state >> 27;
    return state * 0x2545f4914f6cdd1dULL;
}

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

int main() {
    const unsigned seeds = 128;
    const unsigned vectors_per_seed = 1024;
    uint64_t counts[5] = {};
    uint64_t held_checks = 0;
    for (unsigned seed_index = 0; seed_index < seeds; ++seed_index) {
        uint64_t rng = 0x9e3779b97f4a7c15ULL * (seed_index + 1);
        BoomCoreState state;
        for (unsigned index = 0; index < vectors_per_seed; ++index) {
            const uint8_t operation = (uint8_t)((seed_index + index) % 5);
            const uint8_t uopc = (uint8_t)(16 + operation);
            const uint64_t lhs = next(rng);
            const uint64_t rhs = next(rng);
            MicroOp& uop = state.issue.issued_uops[INT_ISSUE_LANE];
            state.issue.issued_valids[INT_ISSUE_LANE] = true;
            uop = MicroOp();
            uop.uopc = uopc;
            uop.iq_type = IQ_ALU;
            uop.fu_code = FU_MUL;
            uop.rename.prs1 = 1;
            uop.rename.prs2 = 2;
            uop.rename.pdst = 3;
            uop.rename.dst_rtype = DST_INT;
            uop.queue.rob_idx = (uint8_t)(index % ROB_DEPTH);
            uop.queue.rob_allocation_id = seed_index * vectors_per_seed + index + 1;
            boom::prf_seed(state, 1, lhs);
            boom::prf_seed(state, 2, rhs);
            state.execute.alu_results[INT_ISSUE_LANE] = ExecuteState::AluResult();
            boom::execute_module(state);
            const ExecuteState::AluResult first =
                state.execute.alu_results[INT_ISSUE_LANE];
            const uint64_t expected = reference(uopc, lhs, rhs);
            if (!first.valid || first.result != expected) {
                std::printf("FAIL seed=%u index=%u uopc=%u expected=%016llx actual=%016llx\n",
                            seed_index, index, uopc,
                            (unsigned long long)expected,
                            (unsigned long long)first.result);
                return 1;
            }
            if ((index & 7u) == 0) {
                boom::prf_seed(state, 1, next(rng));
                boom::prf_seed(state, 2, next(rng));
                boom::execute_module(state);
                const ExecuteState::AluResult& held =
                    state.execute.alu_results[INT_ISSUE_LANE];
                if (held.result != expected || held.uop.uopc != uopc) return 1;
                ++held_checks;
            }
            ++counts[operation];
        }
    }
    std::printf("METRIC,random_seeds,%u\n", seeds);
    std::printf("METRIC,vectors_per_seed,%u\n", vectors_per_seed);
    std::printf("METRIC,total_vectors,%u\n", seeds * vectors_per_seed);
    for (unsigned i = 0; i < 5; ++i)
        std::printf("METRIC,uopc_%u_vectors,%llu\n", 16 + i,
                    (unsigned long long)counts[i]);
    std::printf("METRIC,held_result_checks,%llu\n",
                (unsigned long long)held_checks);
    std::printf("METRIC,mismatches,0\n");
    std::printf("M2B persistent randomized execute integration: PASS\n");
    return 0;
}
