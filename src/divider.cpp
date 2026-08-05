#include "divider.hpp"

namespace boom {

static bool divider_is_word(DivideOperation operation) {
    return operation == DIVW_OP_SIGNED || operation == DIVW_OP_UNSIGNED ||
           operation == REMW_OP_SIGNED || operation == REMW_OP_UNSIGNED;
}

static bool divider_is_signed(DivideOperation operation) {
    return operation == DIV_OP_SIGNED || operation == REM_OP_SIGNED ||
           operation == DIVW_OP_SIGNED || operation == REMW_OP_SIGNED;
}

static bool divider_is_remainder(DivideOperation operation) {
    return operation == REM_OP_SIGNED || operation == REM_OP_UNSIGNED ||
           operation == REMW_OP_SIGNED || operation == REMW_OP_UNSIGNED;
}

static uint64_t divider_negate(uint64_t value) {
    return (~value) + 1ULL;
}

static uint64_t divider_sign_extend_word(uint64_t value) {
    const uint64_t low = value & 0xffffffffULL;
    return (low & 0x80000000ULL) != 0 ? low | 0xffffffff00000000ULL : low;
}

static void divider_finish(DividerState& state, uint64_t result) {
    state.original_dividend = state.word_operation
        ? divider_sign_extend_word(result) : result;
    state.busy = false;
    state.result_pending = true;
}

DividerState::DividerState() {
    divider_reset(*this);
}

void divider_reset(DividerState& state) {
    state.busy = false;
    state.result_pending = false;
    state.operation = DIV_OP_SIGNED;
    state.original_dividend = 0;
    state.original_divisor = 0;
    state.dividend_magnitude = 0;
    state.divisor_magnitude = 0;
    state.quotient = 0;
    state.remainder = 0;
    state.iteration = 0;
    state.quotient_negative = false;
    state.remainder_negative = false;
    state.word_operation = false;
}

bool divider_request_ready(const DividerState& state) {
    return !state.busy && !state.result_pending;
}

bool divider_accept(DividerState& state, const DividerRequest& request) {
    if (!request.valid || !divider_request_ready(state) ||
        static_cast<uint8_t>(request.operation) >
            static_cast<uint8_t>(REMW_OP_UNSIGNED)) {
        return false;
    }

    state.operation = request.operation;
    state.original_dividend = request.dividend;
    state.original_divisor = request.divisor;
    state.word_operation = divider_is_word(request.operation);
    state.iteration = 0;
    state.quotient = 0;
    state.remainder = 0;
    state.result_pending = false;

    const bool signed_operation = divider_is_signed(request.operation);
    const uint64_t operand_mask = state.word_operation
        ? 0xffffffffULL : 0xffffffffffffffffULL;
    const uint64_t sign_mask = state.word_operation
        ? 0x80000000ULL : 0x8000000000000000ULL;
    const uint64_t dividend = request.dividend & operand_mask;
    const uint64_t divisor = request.divisor & operand_mask;
    const bool dividend_negative = signed_operation && (dividend & sign_mask) != 0;
    const bool divisor_negative = signed_operation && (divisor & sign_mask) != 0;

    state.dividend_magnitude = dividend_negative
        ? divider_negate(dividend) & operand_mask : dividend;
    state.divisor_magnitude = divisor_negative
        ? divider_negate(divisor) & operand_mask : divisor;
    state.quotient_negative = dividend_negative != divisor_negative;
    state.remainder_negative = dividend_negative;

    if (divisor == 0) {
        const uint64_t result = divider_is_remainder(request.operation)
            ? dividend : operand_mask;
        divider_finish(state, result);
        return true;
    }

    const bool overflow = signed_operation && dividend == sign_mask &&
                          divisor == operand_mask;
    if (overflow) {
        divider_finish(state, divider_is_remainder(request.operation) ? 0 : sign_mask);
        return true;
    }

    if (dividend == 0) {
        divider_finish(state, 0);
        return true;
    }

    if (state.divisor_magnitude == 1) {
        uint64_t result = 0;
        if (!divider_is_remainder(request.operation)) {
            result = state.quotient_negative
                ? divider_negate(state.dividend_magnitude) & operand_mask
                : state.dividend_magnitude;
        }
        divider_finish(state, result);
        return true;
    }

    state.busy = true;
    return true;
}

void divider_step(DividerState& state) {
    if (!state.busy || state.result_pending) return;

    const uint8_t total_iterations = state.word_operation ? 32 : 64;
    const uint64_t input_bit = state.word_operation
        ? (state.dividend_magnitude >> 31) & 1ULL
        : (state.dividend_magnitude >> 63) & 1ULL;
    state.dividend_magnitude <<= 1;

    const bool shift_overflow = (state.remainder >> 63) != 0;
    const uint64_t shifted_remainder = (state.remainder << 1) | input_bit;
    const bool subtract = shift_overflow ||
                          shifted_remainder >= state.divisor_magnitude;
    state.remainder = subtract
        ? shifted_remainder - state.divisor_magnitude : shifted_remainder;
    state.quotient = (state.quotient << 1) | (subtract ? 1ULL : 0ULL);
    state.iteration++;

    if (state.iteration != total_iterations) return;

    const uint64_t operand_mask = state.word_operation
        ? 0xffffffffULL : 0xffffffffffffffffULL;
    uint64_t result;
    if (divider_is_remainder(state.operation)) {
        result = state.remainder_negative
            ? divider_negate(state.remainder) & operand_mask : state.remainder;
    } else {
        result = state.quotient_negative
            ? divider_negate(state.quotient) & operand_mask : state.quotient;
    }
    divider_finish(state, result);
}

DividerResponse divider_response(const DividerState& state) {
    DividerResponse response;
    response.valid = state.result_pending;
    response.result = state.result_pending ? state.original_dividend : 0;
    return response;
}

void divider_consume_response(DividerState& state) {
    if (!state.result_pending) return;
    state.result_pending = false;
    state.original_dividend = 0;
}

}
