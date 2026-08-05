#ifndef DIVIDER_HPP
#define DIVIDER_HPP

#include <cstdint>

namespace boom {

enum DivideOperation : uint8_t {
    DIV_OP_SIGNED = 0,
    DIV_OP_UNSIGNED = 1,
    REM_OP_SIGNED = 2,
    REM_OP_UNSIGNED = 3,
    DIVW_OP_SIGNED = 4,
    DIVW_OP_UNSIGNED = 5,
    REMW_OP_SIGNED = 6,
    REMW_OP_UNSIGNED = 7
};

struct DividerRequest {
    bool valid;
    DivideOperation operation;
    uint64_t dividend;
    uint64_t divisor;
    DividerRequest()
        : valid(false), operation(DIV_OP_SIGNED), dividend(0), divisor(0) {}
};

struct DividerResponse {
    bool valid;
    uint64_t result;
    DividerResponse() : valid(false), result(0) {}
};

struct DividerState {
    bool busy;
    bool result_pending;
    DivideOperation operation;

    uint64_t original_dividend;
    uint64_t original_divisor;

    uint64_t dividend_magnitude;
    uint64_t divisor_magnitude;
    uint64_t quotient;
    uint64_t remainder;

    uint8_t iteration;

    bool quotient_negative;
    bool remainder_negative;
    bool word_operation;

    DividerState();
};

void divider_reset(DividerState& state);
bool divider_request_ready(const DividerState& state);
bool divider_accept(DividerState& state, const DividerRequest& request);
void divider_step(DividerState& state);
DividerResponse divider_response(const DividerState& state);
void divider_consume_response(DividerState& state);

}

#endif
