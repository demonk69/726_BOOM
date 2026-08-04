#include <cstdint>
#include <cstdio>

void synth_w4_core_step_retention_top(uint8_t seed, uint8_t phase,
                                      uint64_t& observable);

int main() {
    uint64_t observable = 0;
    synth_w4_core_step_retention_top(37, 0, observable);
    if (observable != 0x77f) {
        std::printf("W4_CORE_STEP_RETENTION_STAGE0_FAIL expected=%016llx observed=%016llx\n",
                    0x77fULL, static_cast<unsigned long long>(observable));
        return 1;
    }
    std::printf("W4_CORE_STEP_RETENTION_STAGE0_PASS expected=%016llx observed=%016llx\n",
                0x77fULL, static_cast<unsigned long long>(observable));
    synth_w4_core_step_retention_top(37, 1, observable);
    if (observable != 0x7ff) {
        std::printf("W4_CORE_STEP_RETENTION_FAIL expected=%016llx observed=%016llx\n",
                    0x7ffULL, static_cast<unsigned long long>(observable));
        return 1;
    }
    std::printf("W4_CORE_STEP_RETENTION_PASS expected=%016llx observed=%016llx\n",
                0x7ffULL, static_cast<unsigned long long>(observable));
    return 0;
}
