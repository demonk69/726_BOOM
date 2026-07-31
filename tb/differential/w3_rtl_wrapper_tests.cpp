#include <cstdint>
#include <cstdio>

void synth_w3_diagnostic_top(uint8_t scenario, uint64_t& observable);
void synth_w3_completion_diagnostic_top(uint8_t scenario, uint64_t& observable);
void synth_w3_dual_pending_top(uint32_t allocation_base, uint64_t& observable);
void synth_w3_rob_wrap_top(uint32_t allocation_base, uint64_t& observable);
void synth_w3_branch_kill_top(uint32_t allocation_base, uint64_t& observable);

struct WrapperCase {
    const char* name;
    uint8_t scenario;
    uint64_t expected;
};

static const WrapperCase cases[] = {
    {"dual_accept", 0, 0x000c000b0000030bull},
    {"mem_blocked_int_forward", 1, 0x000c001300000217ull},
    {"int_blocked_mem_forward", 2, 0x001d000b00000117ull},
    {"dual_pending", 3, 0x0020000008d00002ull},
    {"branch_kill", 4, 0x0000000008100400ull},
    {"reset_clear", 5, 0x0000000000000000ull},
    {"load_response_int_conflict", 6, 0x003e000000d00002ull},
    {"trace_backpressure", 7, 0x000000000a100000ull},
    {"dmem_backpressure", 8, 0x000000000c110000ull},
    {"rob_wrap_multiple_identity", 9, 0x0000005cf8d00001ull},
    {"stale_completion_identity", 10, 0x0000000000300000ull},
};

int main() {
    int checked = 0;
    for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
#ifdef __VITIS_HLS__
        if (cases[i].scenario != 2 && cases[i].scenario != 3 && cases[i].scenario != 9)
            continue;
#else
        if (cases[i].scenario == 7 || cases[i].scenario == 8) {
            uint64_t ignored = 0;
            synth_w3_diagnostic_top(cases[i].scenario + 100, ignored);
        }
#endif
        uint64_t observed = 0;
        if (cases[i].scenario == 3)
            synth_w3_dual_pending_top(31, observed);
        else if (cases[i].scenario == 9)
            synth_w3_rob_wrap_top(91, observed);
        else if (cases[i].scenario == 4)
            synth_w3_branch_kill_top(41, observed);
        else if (cases[i].scenario == 10)
            synth_w3_completion_diagnostic_top(cases[i].scenario, observed);
        else
            synth_w3_diagnostic_top(cases[i].scenario, observed);
        if (observed != cases[i].expected) {
            std::printf("W3_WRAPPER_FAIL case=%s expected=%016llx observed=%016llx\n",
                        cases[i].name, (unsigned long long)cases[i].expected,
                        (unsigned long long)observed);
            return 1;
        }
        std::printf("W3_WRAPPER_PASS case=%s observed=%016llx\n", cases[i].name,
                    (unsigned long long)observed);
        checked++;
    }
#ifdef __VITIS_HLS__
    if (checked != 3) return 2;
#else
    if (checked != 11) return 2;
#endif
    return 0;
}
