#include <cstdint>
#include <cstdio>

typedef void (*PilotTop)(uint32_t, uint64_t&, uint64_t&, uint64_t&,
                         uint64_t&, uint64_t&, uint64_t&);

extern "C" void g6_l1r_pilot_lq_reuse(uint32_t, uint64_t&, uint64_t&,
    uint64_t&, uint64_t&, uint64_t&, uint64_t&);
extern "C" void g6_l1r_pilot_stale_response(uint32_t, uint64_t&, uint64_t&,
    uint64_t&, uint64_t&, uint64_t&, uint64_t&);
extern "C" void g6_l1r_pilot_branch_squash(uint32_t, uint64_t&, uint64_t&,
    uint64_t&, uint64_t&, uint64_t&, uint64_t&);
extern "C" void g6_l1r_pilot_sq_identity(uint32_t, uint64_t&, uint64_t&,
    uint64_t&, uint64_t&, uint64_t&, uint64_t&);

static bool run(PilotTop top, int image, const uint64_t expected[6]) {
    uint64_t observed[6] = {};
    top(0x5a, observed[0], observed[1], observed[2], observed[3], observed[4],
        observed[5]);
    for (int i = 0; i < 6; ++i) {
        if (observed[i] != expected[i]) {
            std::printf("G6_L1R_STATELESS_NATIVE_FAIL image=%d obs=%d value=%016llx expected=%016llx\n",
                image, i, (unsigned long long)observed[i],
                (unsigned long long)expected[i]);
            return false;
        }
    }
    std::printf("G6_L1R_STATELESS_NATIVE_PASS image=%d\n", image);
    return true;
}

int main() {
    const uint64_t reuse[6] = {1, 1, 1, 0, 1, 0x101};
    const uint64_t stale[6] = {1, 1, 0, 1, 0x101, 0x2300005b};
    const uint64_t squash[6] = {1, 0, 0, 0, 0x403, 0};
    const uint64_t identity[6] = {1, 0xffff, 1, 0x102, 1, 0};
    bool ok = true;
    ok &= run(g6_l1r_pilot_lq_reuse, 2, reuse);
    ok &= run(g6_l1r_pilot_stale_response, 3, stale);
    ok &= run(g6_l1r_pilot_branch_squash, 4, squash);
    ok &= run(g6_l1r_pilot_sq_identity, 5, identity);
    return ok ? 0 : 1;
}
