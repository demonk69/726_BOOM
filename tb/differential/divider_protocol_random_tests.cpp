#include "divider.hpp"

#include <climits>
#include <cstdint>
#include <iostream>

using namespace boom;

static uint64_t random64(uint64_t& state) {
    state ^= state << 13; state ^= state >> 7; state ^= state << 17; return state;
}

static uint64_t sext32(uint32_t value) {
    return (value & 0x80000000U) ? 0xffffffff00000000ULL | value : value;
}

static uint64_t reference(DivideOperation op, uint64_t lhs, uint64_t rhs) {
    const bool word = op >= DIVW_OP_SIGNED;
    const bool sign = op == DIV_OP_SIGNED || op == REM_OP_SIGNED || op == DIVW_OP_SIGNED || op == REMW_OP_SIGNED;
    const bool rem = op == REM_OP_SIGNED || op == REM_OP_UNSIGNED || op == REMW_OP_SIGNED || op == REMW_OP_UNSIGNED;
    if (word) {
        uint32_t a = lhs, b = rhs, out;
        if (!b) out = rem ? a : UINT32_MAX;
        else if (sign) {
            int32_t sa = static_cast<int32_t>(a), sb = static_cast<int32_t>(b);
            if (sa == INT32_MIN && sb == -1) out = rem ? 0 : a;
            else out = static_cast<uint32_t>(rem ? sa % sb : sa / sb);
        } else out = rem ? a % b : a / b;
        return sext32(out);
    }
    if (!rhs) return rem ? lhs : UINT64_MAX;
    if (sign) {
        int64_t a = static_cast<int64_t>(lhs), b = static_cast<int64_t>(rhs);
        if (a == INT64_MIN && b == -1) return rem ? 0 : lhs;
        return static_cast<uint64_t>(rem ? a % b : a / b);
    }
    return rem ? lhs % rhs : lhs / rhs;
}

static bool is_fast(DivideOperation op, uint64_t lhs, uint64_t rhs) {
    const bool word = op >= DIVW_OP_SIGNED;
    const bool sign = op == DIV_OP_SIGNED || op == REM_OP_SIGNED || op == DIVW_OP_SIGNED || op == REMW_OP_SIGNED;
    const uint64_t mask = word ? UINT32_MAX : UINT64_MAX;
    const uint64_t sign_bit = word ? 0x80000000ULL : 0x8000000000000000ULL;
    lhs &= mask; rhs &= mask;
    return rhs == 0 || lhs == 0 || rhs == 1 || (sign && rhs == mask) ||
           (sign && lhs == sign_bit && rhs == mask);
}

int main() {
    uint64_t mismatches = 0, accepted_count = 0, consumed_count = 0;
    uint64_t reset_kills = 0, held_checks = 0, rejected_busy = 0;
    for (unsigned seed = 0; seed < 128; seed++) {
        uint64_t rng = 0xa0761d6478bd642fULL ^ (uint64_t(seed) * 0xe7037ed1a0b428dbULL);
        DividerState dut;
        bool model_busy = false, model_pending = false;
        unsigned remaining = 0;
        uint64_t expected = 0;
        for (unsigned cycle = 0; cycle < 512; cycle++) {
            const uint64_t control = random64(rng);
            const bool reset = (control & 0xffU) == 0;
            const bool response_ready = (control & 0x100U) != 0;
            const bool request_valid = (control & 0x600U) != 0;
            DivideOperation op = static_cast<DivideOperation>((control >> 11) & 7U);
            uint64_t lhs = random64(rng), rhs = random64(rng);
            if ((control & 0x78000U) == 0) rhs = 0;

            const DividerResponse before = divider_response(dut);
            if (model_pending && (!before.valid || before.result != expected)) mismatches++;
            if (model_pending && !response_ready) held_checks++;

            if (reset) {
                if (model_busy || model_pending) reset_kills++;
                divider_reset(dut);
                model_busy = false; model_pending = false; remaining = 0; expected = 0;
            } else {
                if (response_ready && model_pending) {
                    divider_consume_response(dut);
                    model_pending = false; consumed_count++;
                }

                DividerRequest request;
                request.valid = request_valid; request.operation = op;
                request.dividend = lhs; request.divisor = rhs;
                const bool model_ready = !model_busy && !model_pending;
                const bool should_accept = request_valid && model_ready;
                const bool accepted = divider_accept(dut, request);
                if (accepted != should_accept) mismatches++;
                if (request_valid && !model_ready) rejected_busy++;
                if (accepted) {
                    accepted_count++;
                    expected = reference(op, lhs, rhs);
                    if (is_fast(op, lhs, rhs)) model_pending = true;
                    else { model_busy = true; remaining = op >= DIVW_OP_SIGNED ? 32 : 64; }
                } else if (model_busy) {
                    divider_step(dut);
                    if (--remaining == 0) { model_busy = false; model_pending = true; }
                }
            }

            const DividerResponse after = divider_response(dut);
            if (dut.busy != model_busy || dut.result_pending != model_pending ||
                divider_request_ready(dut) != (!model_busy && !model_pending)) mismatches++;
            if (model_pending && (!after.valid || after.result != expected)) mismatches++;
            if (!model_pending && after.valid) mismatches++;
        }
    }
    std::cout << "seeds=128\ncycles_per_seed=512\ntotal_cycles=65536\n"
              << "accepted_requests=" << accepted_count << "\nconsumed_responses=" << consumed_count
              << "\nreset_kills=" << reset_kills << "\nheld_response_checks=" << held_checks
              << "\nrejected_while_not_ready=" << rejected_busy << "\nmismatches=" << mismatches << "\n";
    return mismatches == 0 ? 0 : 1;
}
