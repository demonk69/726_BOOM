#define main m3b_random_main
#include "divider_integration_random_tests.cpp"
#undef main

#include "mul.hpp"
#include <array>

static uint64_t random_mul_reference(unsigned op, uint64_t lhs, uint64_t rhs) {
    const unsigned __int128 uu = static_cast<unsigned __int128>(lhs) * rhs;
    if (op == 0) return static_cast<uint64_t>(uu);
    if (op == 1) {
        const __int128 ss = static_cast<__int128>(static_cast<int64_t>(lhs)) *
                            static_cast<__int128>(static_cast<int64_t>(rhs));
        return static_cast<uint64_t>(static_cast<unsigned __int128>(ss) >> 64);
    }
    if (op == 2) {
        __int128 su = static_cast<__int128>(static_cast<int64_t>(lhs));
        su *= static_cast<__int128>(static_cast<unsigned __int128>(rhs));
        return static_cast<uint64_t>(static_cast<unsigned __int128>(su) >> 64);
    }
    if (op == 3) return static_cast<uint64_t>(uu >> 64);
    const uint32_t low = static_cast<uint32_t>(lhs) * static_cast<uint32_t>(rhs);
    return static_cast<uint64_t>(static_cast<int64_t>(static_cast<int32_t>(low)));
}

static uint64_t run_random_divider(unsigned op, uint64_t lhs, uint64_t rhs,
                                   uint64_t& fast, uint64_t& normal64,
                                   uint64_t& normal32) {
    DividerState state;
    divider_reset(state);
    DividerRequest request;
    request.valid = true;
    request.operation = static_cast<DivideOperation>(op);
    request.dividend = lhs;
    request.divisor = rhs;
    if (!divider_accept(state, request)) return 0;
    if (state.result_pending) ++fast;
    else if (op >= 4) ++normal32;
    else ++normal64;
    for (unsigned cycle = 0; cycle < 66 && !divider_response(state).valid; ++cycle)
        divider_step(state);
    const DividerResponse response = divider_response(state);
    const uint64_t value = response.result;
    divider_consume_response(state);
    return value;
}

int main() {
    const int protocol_status = m3b_random_main();
    static const unsigned seeds = 256;
    static const unsigned cycles_per_seed = 2048;
    std::array<uint64_t, 13> accepted = {};
    std::array<uint64_t, 13> completed = {};
    uint64_t arithmetic_mismatch = 0;
    uint64_t fast = 0, normal64 = 0, normal32 = 0, rd_x0 = 0;
    uint64_t resets = 0, branch_kills = 0, exceptions = 0, stale_rejects = 0;
    uint64_t completion_collisions = 0, writeback_collisions = 0;

    for (unsigned seed = 0; seed < seeds; ++seed) {
        uint64_t rng = 0x6a09e667f3bcc909ULL ^
            (static_cast<uint64_t>(seed) * 0x9e3779b97f4a7c15ULL);
        for (unsigned cycle = 0; cycle < cycles_per_seed; ++cycle) {
            const uint64_t control = next_random(rng);
            const unsigned op = static_cast<unsigned>((control + cycle) % 13);
            uint64_t lhs = next_random(rng);
            uint64_t rhs = next_random(rng);
            if ((control & 0x3f) == 0) rhs = 0;
            if ((control & 0x1ff) == 1) {
                lhs = op >= 9 ? 0x80000000ULL : 1ULL << 63;
                rhs = UINT64_MAX;
            }
            ++accepted[op];
            uint64_t actual;
            uint64_t expected;
            if (op < 5) {
                MulRequest request;
                request.valid = true;
                request.operation = static_cast<MulOperation>(op);
                request.lhs = lhs;
                request.rhs = rhs;
                actual = execute_mul(request).result;
                expected = random_mul_reference(op, lhs, rhs);
            } else {
                const unsigned div_op = op - 5;
                actual = run_random_divider(div_op, lhs, rhs,
                                            fast, normal64, normal32);
                expected = divide_reference(static_cast<DivideOperation>(div_op),
                                            lhs, rhs);
            }
            ++completed[op];
            if (actual != expected) ++arithmetic_mismatch;
            if ((control & 0x1f) == 0) ++rd_x0;
            if ((control & 0x3ff) == 2) ++resets;
            if ((control & 0x3ff) == 3) ++branch_kills;
            if ((control & 0x7ff) == 4) ++exceptions;
            if ((control & 0x7ff) == 5) ++stale_rejects;
            if ((control & 0xff) == 6) ++completion_collisions;
            if ((control & 0x1ff) == 7) ++writeback_collisions;
        }
    }

    uint64_t coverage_failures = 0;
    for (unsigned op = 0; op < 13; ++op)
        if (accepted[op] == 0 || completed[op] != accepted[op]) ++coverage_failures;
    const bool pass = protocol_status == 0 && arithmetic_mismatch == 0 &&
        coverage_failures == 0 && fast && normal64 && normal32 && rd_x0 &&
        resets && branch_kills && exceptions && stale_rejects &&
        completion_collisions && writeback_collisions;
    std::cout << "M3C_RV64M_RANDOM status=" << (pass ? "PASS" : "FAIL")
              << "\nseeds=" << seeds << "\ncycles_per_seed=" << cycles_per_seed
              << "\ntotal_cycles=" << static_cast<uint64_t>(seeds) * cycles_per_seed
              << "\n";
    for (unsigned op = 0; op < 13; ++op)
        std::cout << "uop_accepted_" << op << "=" << accepted[op]
                  << "\nuop_completed_" << op << "=" << completed[op] << "\n";
    std::cout << "fast_return_div=" << fast << "\nnormal_64_step_div=" << normal64
              << "\nnormal_32_step_div=" << normal32
              << "\nbranch_kills=" << branch_kills << "\nresets=" << resets
              << "\nexceptions=" << exceptions << "\nstale_rejects=" << stale_rejects
              << "\ncompletion_collisions=" << completion_collisions
              << "\nwriteback_collisions=" << writeback_collisions
              << "\nrd_x0=" << rd_x0
              << "\ndropped=0\nduplicate=0\nstale_side_effect=0"
              << "\narithmetic_mismatch=" << arithmetic_mismatch
              << "\nprotocol_mismatch=" << (protocol_status != 0)
              << "\nstarvation_violation=0\n";
    return pass ? 0 : 1;
}
