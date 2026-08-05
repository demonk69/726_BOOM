#define main m3b_full_core_main
#include "divider_full_core_tests.cpp"
#undef main

int main() {
    const std::vector<TestSpec> tests = {
        {"mul_family_all", {{3, UINT64_C(0xffffffffffffffc1)}, {7, UINT64_C(0xffffffffffffffc1)}}, 0, 0, 0, false, false},
        {"div_family_all", {{3, UINT64_MAX}, {4, UINT64_MAX}, {5, 123}, {6, 123}, {7, UINT64_MAX}, {10, 123}}, 8, 8, 0, false, false},
        {"rv64m_all_13", {{3, 700}, {8, UINT64_MAX}, {9, UINT64_MAX}, {10, 100}, {11, 100}, {12, UINT64_MAX}, {15, 100}}, 8, 8, 0, false, false},
        {"mul_div_dependency", {{3, 42}, {5, 14}, {7, 70}}, 1, 1, 60, false, false},
        {"div_mul_dependency", {{3, 14}, {5, 126}, {6, 0}}, 2, 2, 60, false, false},
        {"high_multiply_mix", {{3, UINT64_MAX}, {4, UINT64_MAX}, {5, 2}, {6, UINT64_C(0xfffffffffffffffa)}}, 0, 0, 0, false, false},
        {"word_multiply_divide_mix", {{3, UINT64_C(0xfffffffffffffd44)}, {4, UINT64_C(0xfffffffffffffff2)}, {6, UINT64_C(0xfffffffffffffffe)}}, 4, 4, 30, false, false},
        {"divide_by_zero_mix", {{3, UINT64_MAX}, {5, 123}, {7, UINT64_MAX}, {8, 123}}, 6, 6, 0, false, false},
        {"signed_overflow_mix", {{3, UINT64_C(0x8000000000000000)}, {4, 0}, {5, 0}, {6, 0}}, 4, 4, 0, false, false},
        {"rv64m_branch_mix", {{5, 9}, {6, 81}}, 2, 1, 60, false, false},
        {"rv64m_load_mix", {{2, 84}, {4, 12}}, 1, 1, 60, true, false},
        {"rv64m_store_mix", {{3, 99}, {4, 99}}, 0, 0, 0, true, false},
        {"rv64m_reset_replay", {{3, 24}, {4, 120}}, 1, 1, 60, false, false},
        {"rv64m_rob_wrap", {{1, 40}, {3, 280}, {4, 40}}, 1, 1, 60, false, false},
        {"rv64m_tohost_stress", {{3, 12}, {4, 144}}, 1, 1, 60, false, false}
    };
    unsigned passed = 0;
    unsigned suite_max_latency = 0;
    for (size_t i = 0; i < tests.size(); ++i)
        if (run_test(tests[i], suite_max_latency)) ++passed;
    std::printf("M3C native full-core RV64M programs: %u/%u %s, max latency %u cycles\n",
                passed, (unsigned)tests.size(), passed == tests.size() ? "PASS" : "FAIL",
                suite_max_latency);
    return passed == tests.size() ? 0 : 1;
}
