#include "mul.hpp"
#include <cstdint>
#include <cstdio>

struct Rng {
    uint64_t state;
    explicit Rng(uint64_t seed) : state(seed) {}
    uint64_t next() {
        state ^= state >> 12;
        state ^= state << 25;
        state ^= state >> 27;
        return state * 0x2545f4914f6cdd1dULL;
    }
};

static uint64_t reference(boom::MulOperation operation, uint64_t lhs, uint64_t rhs) {
    const unsigned __int128 up = (unsigned __int128)lhs * (unsigned __int128)rhs;
    if (operation == boom::MUL_OP_LOW) return (uint64_t)up;
    if (operation == boom::MUL_OP_HIGH_SS) {
        const __int128 product = (__int128)(int64_t)lhs * (__int128)(int64_t)rhs;
        return (uint64_t)((unsigned __int128)product >> 64);
    }
    if (operation == boom::MUL_OP_HIGH_SU) {
        uint64_t high = (uint64_t)(up >> 64);
        return (lhs >> 63) != 0 ? high - rhs : high;
    }
    if (operation == boom::MUL_OP_HIGH_UU) return (uint64_t)(up >> 64);
    const uint64_t product = (uint64_t)(uint32_t)lhs * (uint64_t)(uint32_t)rhs;
    return (uint64_t)(int64_t)(int32_t)(uint32_t)product;
}

static uint64_t patterned(Rng& rng, unsigned mode, unsigned index) {
    static const uint64_t edges[] = {
        0, 1, 0xffffffffffffffffULL, 0x8000000000000000ULL,
        0x7fffffffffffffffULL, 0x0000000080000000ULL,
        0x0001000100010001ULL, 0xff00ff00ff00ff00ULL
    };
    if (mode == 0) return edges[index % (sizeof(edges) / sizeof(edges[0]))];
    if (mode == 1) return 1ULL << (rng.next() & 63);
    if (mode == 2) return ~(1ULL << (rng.next() & 63));
    if (mode == 3) return rng.next() & rng.next();
    if (mode == 4) return rng.next() | rng.next();
    return rng.next();
}

int main() {
    const unsigned seeds = 256;
    const unsigned vectors_per_seed = 512;
    uint64_t operation_counts[5] = {};
    uint64_t edge_vectors = 0;
    uint64_t random_vectors = 0;

    for (unsigned seed_index = 0; seed_index < seeds; seed_index++) {
        const uint64_t seed = 0x6a09e667f3bcc909ULL ^
                              (0x9e3779b97f4a7c15ULL * (seed_index + 1));
        Rng rng(seed);
        for (unsigned index = 0; index < vectors_per_seed; index++) {
            const boom::MulOperation operation =
                (boom::MulOperation)((seed_index * vectors_per_seed + index) % 5);
            const unsigned mode = index % 16;
            const bool edge = mode < 5;
            const uint64_t lhs = patterned(rng, edge ? mode : 5, index + seed_index);
            const uint64_t rhs = patterned(rng, edge ? (mode + 2) % 5 : 5,
                                           index * 3 + seed_index);
            boom::MulRequest request;
            request.valid = true;
            request.operation = operation;
            request.lhs = lhs;
            request.rhs = rhs;
            const boom::MulResponse response = boom::execute_mul(request);
            const uint64_t expected = reference(operation, lhs, rhs);
            operation_counts[(unsigned)operation]++;
            if (edge) edge_vectors++; else random_vectors++;
            if (!response.valid || response.result != expected) {
                std::printf("FAIL seed=0x%016llx index=%u operation=%u lhs=0x%016llx rhs=0x%016llx expected=0x%016llx actual=0x%016llx\n",
                            (unsigned long long)seed, index, (unsigned)operation,
                            (unsigned long long)lhs, (unsigned long long)rhs,
                            (unsigned long long)expected,
                            (unsigned long long)response.result);
                return 1;
            }
        }
    }

    const uint64_t total = (uint64_t)seeds * vectors_per_seed;
    std::printf("METRIC,total_vectors,%llu\n", (unsigned long long)total);
    for (unsigned operation = 0; operation < 5; operation++)
        std::printf("METRIC,vectors_operation_%u,%llu\n", operation,
                    (unsigned long long)operation_counts[operation]);
    std::printf("METRIC,edge_vectors,%llu\n", (unsigned long long)edge_vectors);
    std::printf("METRIC,random_vectors,%llu\n", (unsigned long long)random_vectors);
    std::printf("METRIC,mismatches,0\n");
    std::printf("METRIC,first_failure_seed,0\n");
    std::printf("METRIC,first_failure_index,0\n");
    std::printf("M2A arithmetic random: 256 seeds, 131072 vectors, 0 mismatches\n");
    return 0;
}
