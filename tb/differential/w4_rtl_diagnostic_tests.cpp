#include <cstdint>
#include <cstdio>

void synth_w4d_oracle_top(uint8_t scenario, uint64_t* observable);

int main() {
    for (uint8_t scenario = 0; scenario <= 15; scenario++) {
        uint64_t observable = 0;
        synth_w4d_oracle_top(scenario, &observable);
        std::printf("W4_EXPECT scenario=%u observable=%016llx\n",
                    static_cast<unsigned>(scenario),
                    static_cast<unsigned long long>(observable));
    }
    return 0;
}
