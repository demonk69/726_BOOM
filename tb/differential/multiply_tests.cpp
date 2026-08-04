#include "mul.hpp"
#include <cstdint>
#include <cstdio>
#include <limits>

using boom::MulOperation;

static uint64_t reference(MulOperation operation, uint64_t lhs, uint64_t rhs) {
    const unsigned __int128 up = (unsigned __int128)lhs * (unsigned __int128)rhs;
    switch (operation) {
    case boom::MUL_OP_LOW:
        return (uint64_t)up;
    case boom::MUL_OP_HIGH_SS: {
        const __int128 product = (__int128)(int64_t)lhs * (__int128)(int64_t)rhs;
        return (uint64_t)((unsigned __int128)product >> 64);
    }
    case boom::MUL_OP_HIGH_SU: {
        uint64_t high = (uint64_t)(up >> 64);
        return (lhs >> 63) != 0 ? high - rhs : high;
    }
    case boom::MUL_OP_HIGH_UU:
        return (uint64_t)(up >> 64);
    case boom::MUL_OP_WORD: {
        const uint64_t product = (uint64_t)(uint32_t)lhs * (uint64_t)(uint32_t)rhs;
        return (uint64_t)(int64_t)(int32_t)(uint32_t)product;
    }
    }
    return 0;
}

struct Vector {
    MulOperation operation;
    uint64_t lhs;
    uint64_t rhs;
};

int main() {
    const uint64_t smin = 0x8000000000000000ULL;
    const uint64_t smax = 0x7fffffffffffffffULL;
    const uint64_t umax = 0xffffffffffffffffULL;
    const Vector vectors[] = {
        {boom::MUL_OP_LOW,0,0},{boom::MUL_OP_LOW,1,1},{boom::MUL_OP_LOW,umax,1},
        {boom::MUL_OP_LOW,umax,umax},{boom::MUL_OP_LOW,smin,1},{boom::MUL_OP_LOW,smin,umax},
        {boom::MUL_OP_LOW,smax,smax},{boom::MUL_OP_LOW,umax,2},{boom::MUL_OP_LOW,smin,smin},
        {boom::MUL_OP_LOW,0x123456789abcdef0ULL,0xfedcba9876543210ULL},

        {boom::MUL_OP_HIGH_SS,0,0},{boom::MUL_OP_HIGH_SS,1,1},{boom::MUL_OP_HIGH_SS,umax,1},
        {boom::MUL_OP_HIGH_SS,umax,umax},{boom::MUL_OP_HIGH_SS,smin,1},
        {boom::MUL_OP_HIGH_SS,smin,umax},{boom::MUL_OP_HIGH_SS,smin,smin},
        {boom::MUL_OP_HIGH_SS,smax,smax},{boom::MUL_OP_HIGH_SS,7,(uint64_t)-11},
        {boom::MUL_OP_HIGH_SS,(uint64_t)-13,17},

        {boom::MUL_OP_HIGH_SU,0,umax},{boom::MUL_OP_HIGH_SU,1,umax},
        {boom::MUL_OP_HIGH_SU,umax,umax},{boom::MUL_OP_HIGH_SU,smin,1},
        {boom::MUL_OP_HIGH_SU,smin,umax},{boom::MUL_OP_HIGH_SU,smax,umax},
        {boom::MUL_OP_HIGH_SU,(uint64_t)-9,0x0001000100010001ULL},
        {boom::MUL_OP_HIGH_SU,(uint64_t)-3,smin},{boom::MUL_OP_HIGH_SU,5,smin},
        {boom::MUL_OP_HIGH_SU,0x8000000000000001ULL,0xffffffff00000001ULL},

        {boom::MUL_OP_HIGH_UU,0,umax},{boom::MUL_OP_HIGH_UU,1,umax},
        {boom::MUL_OP_HIGH_UU,umax,umax},{boom::MUL_OP_HIGH_UU,smin,2},
        {boom::MUL_OP_HIGH_UU,smin,smin},{boom::MUL_OP_HIGH_UU,0x0001000100010001ULL,0x0010001000100010ULL},
        {boom::MUL_OP_HIGH_UU,0xff00ff00ff00ff00ULL,0xf0f0f0f0f0f0f0f0ULL},
        {boom::MUL_OP_HIGH_UU,0xffffffff00000001ULL,0xffffffff00000001ULL},

        {boom::MUL_OP_WORD,0,0},{boom::MUL_OP_WORD,1,1},{boom::MUL_OP_WORD,umax,1},
        {boom::MUL_OP_WORD,umax,umax},{boom::MUL_OP_WORD,0x7fffffffULL,2},
        {boom::MUL_OP_WORD,0x80000000ULL,1},{boom::MUL_OP_WORD,0xffffffffULL,0xffffffffULL},
        {boom::MUL_OP_WORD,0x1234567800000003ULL,7},
        {boom::MUL_OP_WORD,9,0xabcdef0100000005ULL},
        {boom::MUL_OP_WORD,0x0000000000007fffULL,2},
        {boom::MUL_OP_WORD,0x0000000040000000ULL,2},
        {boom::MUL_OP_WORD,0x00000000ffffffffULL,2},
    };

    const unsigned count = sizeof(vectors) / sizeof(vectors[0]);
    for (unsigned i = 0; i < count; i++) {
        boom::MulRequest request;
        request.valid = true;
        request.operation = vectors[i].operation;
        request.lhs = vectors[i].lhs;
        request.rhs = vectors[i].rhs;
        const boom::MulResponse response = boom::execute_mul(request);
        const uint64_t expected = reference(request.operation, request.lhs, request.rhs);
        if (!response.valid || response.result != expected) {
            std::printf("FAIL operation=%u lhs=0x%016llx rhs=0x%016llx expected=0x%016llx actual=0x%016llx\n",
                        (unsigned)request.operation, (unsigned long long)request.lhs,
                        (unsigned long long)request.rhs, (unsigned long long)expected,
                        (unsigned long long)response.result);
            return 1;
        }
    }

    boom::MulRequest invalid;
    if (boom::execute_mul(invalid).valid) {
        std::printf("FAIL invalid request produced a valid response\n");
        return 1;
    }
    std::printf("M2A directed multiply: %u passed, 0 failed\n", count + 1);
    return 0;
}
