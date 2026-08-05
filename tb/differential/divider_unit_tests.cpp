#include "divider.hpp"

#include <climits>
#include <cstdint>
#include <iostream>
#include <vector>

using namespace boom;

struct Vector {
    const char* name;
    DivideOperation operation;
    uint64_t dividend;
    uint64_t divisor;
};

static uint64_t sext32(uint32_t value) {
    return (value & 0x80000000U) != 0
        ? 0xffffffff00000000ULL | value : value;
}

static uint64_t reference(DivideOperation operation, uint64_t lhs, uint64_t rhs) {
    const bool word = operation >= DIVW_OP_SIGNED;
    const bool signed_op = operation == DIV_OP_SIGNED || operation == REM_OP_SIGNED ||
                           operation == DIVW_OP_SIGNED || operation == REMW_OP_SIGNED;
    const bool remainder = operation == REM_OP_SIGNED || operation == REM_OP_UNSIGNED ||
                           operation == REMW_OP_SIGNED || operation == REMW_OP_UNSIGNED;
    if (word) {
        const uint32_t a = static_cast<uint32_t>(lhs);
        const uint32_t b = static_cast<uint32_t>(rhs);
        uint32_t result;
        if (b == 0) result = remainder ? a : UINT32_MAX;
        else if (signed_op) {
            const int32_t sa = static_cast<int32_t>(a);
            const int32_t sb = static_cast<int32_t>(b);
            if (sa == INT32_MIN && sb == -1) result = remainder ? 0 : a;
            else result = static_cast<uint32_t>(remainder ? sa % sb : sa / sb);
        } else {
            result = remainder ? a % b : a / b;
        }
        return sext32(result);
    }
    if (rhs == 0) return remainder ? lhs : UINT64_MAX;
    if (signed_op) {
        const int64_t a = static_cast<int64_t>(lhs);
        const int64_t b = static_cast<int64_t>(rhs);
        if (a == INT64_MIN && b == -1) return remainder ? 0 : lhs;
        return static_cast<uint64_t>(remainder ? a % b : a / b);
    }
    return remainder ? lhs % rhs : lhs / rhs;
}

static uint64_t run(const Vector& vector, unsigned& iterations) {
    DividerState state;
    DividerRequest request;
    request.valid = true;
    request.operation = vector.operation;
    request.dividend = vector.dividend;
    request.divisor = vector.divisor;
    if (!divider_accept(state, request)) return 0xbadbadbadbadbadULL;
    iterations = 0;
    while (state.busy) {
        divider_step(state);
        iterations++;
        if (iterations > 64) break;
    }
    return divider_response(state).result;
}

int main() {
    const uint64_t neg10 = static_cast<uint64_t>(-10LL);
    const uint64_t neg3 = static_cast<uint64_t>(-3LL);
    std::vector<Vector> vectors = {
        {"div_10_3", DIV_OP_SIGNED, 10, 3}, {"div_n10_3", DIV_OP_SIGNED, neg10, 3},
        {"div_10_n3", DIV_OP_SIGNED, 10, neg3}, {"div_n10_n3", DIV_OP_SIGNED, neg10, neg3},
        {"div_max_1", DIV_OP_SIGNED, INT64_MAX, 1}, {"div_min_1", DIV_OP_SIGNED, 1ULL << 63, 1},
        {"div_overflow", DIV_OP_SIGNED, 1ULL << 63, UINT64_MAX},
        {"div_less", DIV_OP_SIGNED, 3, 10}, {"div_equal", DIV_OP_SIGNED, 77, 77},
        {"div_zero", DIV_OP_SIGNED, 0x123456789abcdef0ULL, 0},
        {"divu_max_3", DIV_OP_UNSIGNED, UINT64_MAX, 3},
        {"divu_equal_max", DIV_OP_UNSIGNED, UINT64_MAX, UINT64_MAX},
        {"divu_bit63_2", DIV_OP_UNSIGNED, 1ULL << 63, 2},
        {"divu_zero_divisor", DIV_OP_UNSIGNED, 99, 0}, {"divu_zero", DIV_OP_UNSIGNED, 0, 99},
        {"rem_pp", REM_OP_SIGNED, 10, 3}, {"rem_np", REM_OP_SIGNED, neg10, 3},
        {"rem_pn", REM_OP_SIGNED, 10, neg3}, {"rem_nn", REM_OP_SIGNED, neg10, neg3},
        {"rem_overflow", REM_OP_SIGNED, 1ULL << 63, UINT64_MAX},
        {"rem_zero", REM_OP_SIGNED, neg10, 0}, {"rem_sign", REM_OP_SIGNED, static_cast<uint64_t>(-23LL), 7},
        {"remu_max_3", REM_OP_UNSIGNED, UINT64_MAX, 3}, {"remu_less", REM_OP_UNSIGNED, 3, 10},
        {"remu_equal", REM_OP_UNSIGNED, 77, 77}, {"remu_zero", REM_OP_UNSIGNED, UINT64_MAX, 0},
        {"divw_positive", DIVW_OP_SIGNED, 100, 7},
        {"divw_negative", DIVW_OP_SIGNED, 0x00000000fffffff6ULL, 3},
        {"divw_overflow", DIVW_OP_SIGNED, 0x80000000ULL, UINT32_MAX},
        {"divw_zero", DIVW_OP_SIGNED, 0x80000001ULL, 0},
        {"divuw_max", DIVW_OP_UNSIGNED, UINT32_MAX, 3}, {"divuw_zero", DIVW_OP_UNSIGNED, 0, 9},
        {"remw_positive", REMW_OP_SIGNED, 23, 7},
        {"remw_negative", REMW_OP_SIGNED, 0xfffffff9ULL, 3},
        {"remw_overflow", REMW_OP_SIGNED, 0x80000000ULL, UINT32_MAX},
        {"remw_zero", REMW_OP_SIGNED, 0x80000001ULL, 0},
        {"remuw_max", REMW_OP_UNSIGNED, UINT32_MAX, 7},
        {"remuw_zero", REMW_OP_UNSIGNED, 0x80000001ULL, 0},
        {"word_high_ignored", DIVW_OP_SIGNED, 0xabcdef010000000aULL, 0x1234567800000003ULL},
        {"word_sign_extend", DIVW_OP_UNSIGNED, UINT32_MAX, 1}
    };
    for (unsigned op = 0; op < 8; op++) {
        for (unsigned i = 0; i < 8; i++) {
            vectors.push_back({"matrix", static_cast<DivideOperation>(op),
                               0xfedcba9876543210ULL ^ (uint64_t(i) << (i + 1)),
                               2 + i * 0x10101ULL});
        }
    }

    unsigned failures = 0;
    unsigned normal64 = 0;
    unsigned normal32 = 0;
    for (const Vector& vector : vectors) {
        unsigned iterations;
        const uint64_t actual = run(vector, iterations);
        const uint64_t expected = reference(vector.operation, vector.dividend, vector.divisor);
        const bool word = vector.operation >= DIVW_OP_SIGNED;
        if (actual != expected || iterations > (word ? 32U : 64U)) {
            std::cerr << "FAIL " << vector.name << " op=" << unsigned(vector.operation)
                      << " actual=0x" << std::hex << actual << " expected=0x" << expected
                      << std::dec << " iterations=" << iterations << "\n";
            failures++;
        }
        if (iterations == 64) normal64++;
        if (iterations == 32) normal32++;
    }

    DividerState state;
    DividerRequest invalid;
    if (divider_accept(state, invalid)) failures++;
    DividerRequest first;
    first.valid = true; first.operation = DIV_OP_UNSIGNED; first.dividend = 100; first.divisor = 7;
    if (!divider_accept(state, first) || !state.busy || divider_request_ready(state)) failures++;
    DividerRequest second = first; second.dividend = 200;
    if (divider_accept(state, second)) failures++;
    while (state.busy) divider_step(state);
    const DividerResponse held = divider_response(state);
    for (int i = 0; i < 5; i++) {
        divider_step(state);
        const DividerResponse again = divider_response(state);
        if (!again.valid || again.result != held.result || divider_request_ready(state)) failures++;
    }
    divider_consume_response(state);
    if (!divider_request_ready(state) || divider_response(state).valid) failures++;
    divider_consume_response(state);
    if (!divider_accept(state, first)) failures++;
    divider_step(state);
    divider_reset(state);
    if (!divider_request_ready(state) || divider_response(state).valid) failures++;
    DividerRequest fast = first; fast.divisor = 0;
    divider_accept(state, fast);
    divider_reset(state);
    if (!divider_request_ready(state) || divider_response(state).valid) failures++;

    std::cout << "directed_cases=" << vectors.size() << "\n"
              << "normal_64_iteration_cases=" << normal64 << "\n"
              << "normal_32_iteration_cases=" << normal32 << "\n"
              << "mismatches=" << failures << "\n";
    return failures == 0 ? 0 : 1;
}
