#ifndef MUL_HPP
#define MUL_HPP

#include <cstdint>

namespace boom {

enum MulOperation : uint8_t {
    MUL_OP_LOW = 0,
    MUL_OP_HIGH_SS = 1,
    MUL_OP_HIGH_SU = 2,
    MUL_OP_HIGH_UU = 3,
    MUL_OP_WORD = 4
};

struct MulRequest {
    bool valid;
    MulOperation operation;
    uint64_t lhs;
    uint64_t rhs;
    MulRequest() : valid(false), operation(MUL_OP_LOW), lhs(0), rhs(0) {}
};

struct MulResponse {
    bool valid;
    uint64_t result;
    MulResponse() : valid(false), result(0) {}
};

MulResponse execute_mul(const MulRequest& request);

}

#endif
