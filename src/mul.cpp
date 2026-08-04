#include "mul.hpp"

#if defined(BOOM_USE_AP_INT) || defined(__SYNTHESIS__)
#include <ap_int.h>
#endif

namespace boom {

MulResponse execute_mul(const MulRequest& request) {
    MulResponse response;
    if (!request.valid) return response;

    response.valid = true;

#if defined(BOOM_USE_AP_INT) || defined(__SYNTHESIS__)
    const ap_uint<64> lhs_u = request.lhs;
    const ap_uint<64> rhs_u = request.rhs;
    const ap_uint<128> unsigned_product = lhs_u * rhs_u;

    switch (request.operation) {
    case MUL_OP_LOW:
        response.result = (uint64_t)unsigned_product.range(63, 0);
        break;
    case MUL_OP_HIGH_SS: {
        const ap_int<64> lhs_s = request.lhs;
        const ap_int<64> rhs_s = request.rhs;
        const ap_int<128> lhs_wide = lhs_s;
        const ap_int<128> rhs_wide = rhs_s;
        const ap_int<128> product = lhs_wide * rhs_wide;
        response.result = (uint64_t)product.range(127, 64);
        break;
    }
    case MUL_OP_HIGH_SU: {
        uint64_t high = (uint64_t)unsigned_product.range(127, 64);
        if ((request.lhs >> 63) != 0) high -= request.rhs;
        response.result = high;
        break;
    }
    case MUL_OP_HIGH_UU:
        response.result = (uint64_t)unsigned_product.range(127, 64);
        break;
    case MUL_OP_WORD: {
        const ap_uint<32> lhs_word = request.lhs;
        const ap_uint<32> rhs_word = request.rhs;
        const ap_uint<64> word_product = lhs_word * rhs_word;
        const uint32_t low_word = (uint32_t)word_product.range(31, 0);
        response.result = (uint64_t)(int64_t)(int32_t)low_word;
        break;
    }
    default:
        response.valid = false;
        response.result = 0;
        break;
    }
#else
    const unsigned __int128 unsigned_product =
        (unsigned __int128)request.lhs * (unsigned __int128)request.rhs;

    switch (request.operation) {
    case MUL_OP_LOW:
        response.result = (uint64_t)unsigned_product;
        break;
    case MUL_OP_HIGH_SS: {
        const __int128 product = (__int128)(int64_t)request.lhs *
                                 (__int128)(int64_t)request.rhs;
        response.result = (uint64_t)((unsigned __int128)product >> 64);
        break;
    }
    case MUL_OP_HIGH_SU: {
        uint64_t high = (uint64_t)(unsigned_product >> 64);
        if ((request.lhs >> 63) != 0) high -= request.rhs;
        response.result = high;
        break;
    }
    case MUL_OP_HIGH_UU:
        response.result = (uint64_t)(unsigned_product >> 64);
        break;
    case MUL_OP_WORD: {
        const uint64_t product = (uint64_t)(uint32_t)request.lhs *
                                 (uint64_t)(uint32_t)request.rhs;
        response.result = (uint64_t)(int64_t)(int32_t)(uint32_t)product;
        break;
    }
    default:
        response.valid = false;
        response.result = 0;
        break;
    }
#endif

    return response;
}

}
