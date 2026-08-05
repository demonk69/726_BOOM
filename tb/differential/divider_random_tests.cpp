#include "divider.hpp"

#include <climits>
#include <cstdint>
#include <iomanip>
#include <iostream>

using namespace boom;

static uint64_t next_random(uint64_t& state) {
    state ^= state << 13; state ^= state >> 7; state ^= state << 17; return state;
}

static uint64_t sext32(uint32_t value) {
    return (value & 0x80000000U) ? 0xffffffff00000000ULL | value : value;
}

static uint64_t reference(DivideOperation op, uint64_t lhs, uint64_t rhs) {
    const bool word = op >= DIVW_OP_SIGNED;
    const bool sign = op == DIV_OP_SIGNED || op == REM_OP_SIGNED ||
                      op == DIVW_OP_SIGNED || op == REMW_OP_SIGNED;
    const bool rem = op == REM_OP_SIGNED || op == REM_OP_UNSIGNED ||
                     op == REMW_OP_SIGNED || op == REMW_OP_UNSIGNED;
    if (word) {
        uint32_t a = lhs, b = rhs, out;
        if (b == 0) out = rem ? a : UINT32_MAX;
        else if (sign) {
            int32_t sa = static_cast<int32_t>(a), sb = static_cast<int32_t>(b);
            if (sa == INT32_MIN && sb == -1) out = rem ? 0 : a;
            else out = static_cast<uint32_t>(rem ? sa % sb : sa / sb);
        } else out = rem ? a % b : a / b;
        return sext32(out);
    }
    if (rhs == 0) return rem ? lhs : UINT64_MAX;
    if (sign) {
        int64_t a = static_cast<int64_t>(lhs), b = static_cast<int64_t>(rhs);
        if (a == INT64_MIN && b == -1) return rem ? 0 : lhs;
        return static_cast<uint64_t>(rem ? a % b : a / b);
    }
    return rem ? lhs % rhs : lhs / rhs;
}

static bool special(DivideOperation op, uint64_t lhs, uint64_t rhs) {
    const bool word = op >= DIVW_OP_SIGNED;
    const bool sign = op == DIV_OP_SIGNED || op == REM_OP_SIGNED ||
                      op == DIVW_OP_SIGNED || op == REMW_OP_SIGNED;
    const uint64_t mask = word ? UINT32_MAX : UINT64_MAX;
    const uint64_t sign_bit = word ? 0x80000000ULL : 0x8000000000000000ULL;
    lhs &= mask; rhs &= mask;
    return rhs == 0 || lhs == 0 || rhs == 1 ||
           (sign && rhs == mask) || (sign && lhs == sign_bit && rhs == mask);
}

static uint64_t injected(unsigned index, uint64_t random) {
    static const uint64_t values[] = {
        0, 1, UINT64_MAX, 1ULL << 63, INT64_MAX, 0x80000000ULL,
        INT32_MAX, 1ULL << 1, 1ULL << 17, 1ULL << 32, 1ULL << 62,
        0x0000000100000001ULL, 0xaaaaaaaaaaaaaaaaULL, 0x5555555555555555ULL,
        0xfffffffffffffffeULL, 0x8000000000000001ULL
    };
    return (index & 3U) == 0 ? values[(index >> 2) & 15U] : random;
}

int main() {
    uint64_t per_operation[8] = {};
    uint64_t specials = 0, normal = 0, mismatches = 0, latency_sum = 0;
    unsigned min_latency = 65, max_latency = 0;
    for (unsigned seed = 0; seed < 256; seed++) {
        uint64_t rng = 0x9e3779b97f4a7c15ULL ^ (uint64_t(seed) * 0xd1b54a32d192ed03ULL);
        for (unsigned index = 0; index < 256; index++) {
            DivideOperation op = static_cast<DivideOperation>((seed * 256 + index) & 7U);
            uint64_t lhs = injected(index, next_random(rng));
            uint64_t rhs = injected(index + seed + 7, next_random(rng));
            DividerState state;
            DividerRequest request;
            request.valid = true; request.operation = op; request.dividend = lhs; request.divisor = rhs;
            if (!divider_accept(state, request)) { mismatches++; continue; }
            unsigned latency = 0;
            while (state.busy) { divider_step(state); latency++; }
            const uint64_t actual = divider_response(state).result;
            const uint64_t expected = reference(op, lhs, rhs);
            if (actual != expected) {
                if (mismatches < 8)
                    std::cerr << "mismatch op=" << unsigned(op) << " lhs=0x" << std::hex << lhs
                              << " rhs=0x" << rhs << " actual=0x" << actual
                              << " expected=0x" << expected << std::dec << "\n";
                mismatches++;
            }
            per_operation[op]++;
            if (special(op, lhs, rhs)) specials++; else normal++;
            if (latency < min_latency) min_latency = latency;
            if (latency > max_latency) max_latency = latency;
            latency_sum += latency;
        }
    }
    std::cout << "total_operations=65536\n";
    for (unsigned op = 0; op < 8; op++) std::cout << "operation_" << op << "=" << per_operation[op] << "\n";
    std::cout << "special_cases=" << specials << "\nnormal_cases=" << normal
              << "\nmismatches=" << mismatches << "\nmin_latency=" << min_latency
              << "\nmax_latency=" << max_latency << "\naverage_latency="
              << std::fixed << std::setprecision(6) << double(latency_sum) / 65536.0 << "\n";
    return mismatches == 0 ? 0 : 1;
}
