#define main m3b_integration_main
#include "divider_integration_tests.cpp"
#undef main

#include <vector>

static uint64_t mul_reference(unsigned op, uint64_t lhs, uint64_t rhs) {
    const unsigned __int128 uu = static_cast<unsigned __int128>(lhs) * rhs;
    if (op == 0) return static_cast<uint64_t>(uu);
    if (op == 1) {
        const __int128 ss = static_cast<__int128>(static_cast<int64_t>(lhs)) *
                            static_cast<__int128>(static_cast<int64_t>(rhs));
        return static_cast<uint64_t>(static_cast<unsigned __int128>(ss) >> 64);
    }
    if (op == 2) {
        __int128 su = static_cast<__int128>(static_cast<int64_t>(lhs));
        su *= static_cast<__int128>(static_cast<unsigned __int128>(rhs));
        return static_cast<uint64_t>(static_cast<unsigned __int128>(su) >> 64);
    }
    if (op == 3) return static_cast<uint64_t>(uu >> 64);
    return sext32(static_cast<uint32_t>(lhs) * static_cast<uint32_t>(rhs));
}

static uint64_t rv64m_reference(uint8_t uopc, uint64_t lhs, uint64_t rhs) {
    return uopc <= 20 ? mul_reference(uopc - 16, lhs, rhs) :
                        divide_reference(uopc - 21, lhs, rhs);
}

static uint64_t run_integrated(uint8_t uopc, uint64_t lhs, uint64_t rhs,
                               uint8_t pdst, uint32_t allocation_id,
                               const std::string& label) {
    BoomCoreState state;
    const bool mul = uopc <= 20;
    MicroOp uop = make_uop(uopc, mul ? FU_MUL : FU_DIV, IQ_ALU, 0,
                           allocation_id, pdst, 1, 2);
    own(state, uop);
    boom::prf_seed(state, 1, lhs);
    boom::prf_seed(state, 2, rhs);
    dispatch(state, uop);
    boom::issue_module(state);
    check(state.issue.issued_valids[INT_ISSUE_LANE] &&
          state.issue.grants[INT_ISSUE_LANE].accepted, label + " accepted");
    check(!state.issue.issued_valids[MEM_ISSUE_LANE] &&
          !state.issue.issued_valids[FP_ISSUE_LANE], label + " INT lane only");
    boom::execute_module(state);
    if (mul) boom::completion_service_execute(state);
    else finish_divide(state, 0);
    const uint64_t expected = rv64m_reference(uopc, lhs, rhs);
    check(!state.rob.entries[0].busy, label + " ROB complete");
    check(state.completion.total_completion_accepts == 1 &&
          state.completion.total_rob_completes == 1, label + " one completion");
    check(state.completion.total_prf_writes == (pdst ? 1U : 0U),
          label + " PRF write policy");
    check(state.completion.total_wakeups == (pdst ? 1U : 0U) &&
          state.completion.total_bypass == (pdst ? 1U : 0U),
          label + " wakeup bypass policy");
    check(boom::prf_read(state, 0) == 0, label + " x0 unchanged");
    if (pdst) check(boom::prf_read(state, pdst) == expected, label + " value");
    const uint64_t accepts = state.completion.total_completion_accepts;
    pipeline_cycle(state);
    check(state.completion.total_completion_accepts == accepts,
          label + " no duplicate completion");
    return pdst ? boom::prf_read(state, pdst) : expected;
}

static void test_joint_arithmetic_matrix() {
    static const uint64_t vectors[][2] = {
        {0, 0}, {1, 1}, {UINT64_MAX, 1}, {UINT64_MAX, UINT64_MAX},
        {1ULL << 63, UINT64_MAX}, {1ULL << 63, 2},
        {0x7fffffffffffffffULL, 3}, {0x123456789abcdef0ULL, 0},
        {0x0000000080000000ULL, 0x00000000ffffffffULL},
        {0xabcdef0180000001ULL, 17}
    };
    static const char* names[] = {
        "MUL", "MULH", "MULHSU", "MULHU", "MULW", "DIV", "DIVU",
        "REM", "REMU", "DIVW", "DIVUW", "REMW", "REMUW"
    };
    for (uint8_t uopc = 16; uopc <= 28; ++uopc) {
        for (unsigned v = 0; v < sizeof(vectors) / sizeof(vectors[0]); ++v) {
            const uint8_t pdst = v == 0 ? 0 : static_cast<uint8_t>(3 + (v % 48));
            run_integrated(uopc, vectors[v][0], vectors[v][1], pdst,
                           1000 + uopc * 16 + v,
                           std::string(names[uopc - 16]) + " vector " +
                           std::to_string(v));
        }
    }
}

static void test_joint_dependencies() {
    struct Chain { uint8_t producer; uint8_t consumer; const char* name; };
    static const Chain chains[] = {
        {16, 21, "MUL to DIV"}, {21, 16, "DIV to MUL"},
        {17, 23, "MULH to REM"}, {23, 20, "REM to MULW"},
        {25, 16, "DIVW to MUL"}, {20, 26, "MULW to DIVUW"}
    };
    for (unsigned i = 0; i < sizeof(chains) / sizeof(chains[0]); ++i) {
        const uint64_t first = run_integrated(chains[i].producer,
            0xfedcba9876543211ULL, 37, 20, 3000 + i * 2,
            std::string(chains[i].name) + " producer");
        const uint64_t second = run_integrated(chains[i].consumer, first, 7,
            21, 3001 + i * 2, std::string(chains[i].name) + " consumer");
        check(second == rv64m_reference(chains[i].consumer, first, 7),
              std::string(chains[i].name) + " dependency value");
    }
}

static void test_all_mul_during_divider_busy() {
    for (uint8_t mul_uopc = 16; mul_uopc <= 20; ++mul_uopc) {
        BoomCoreState state;
        MicroOp div = make_uop(22, FU_DIV, IQ_ALU, 0, 4000 + mul_uopc,
                               10, 1, 2);
        start_divide(state, div, UINT64_MAX, 19);
        check(state.execute.divider.arithmetic.busy,
              "divider busy before MUL family overlap");
        MicroOp mul = make_uop(mul_uopc, FU_MUL, IQ_ALU, 1,
                               4100 + mul_uopc, 11, 3, 4);
        own(state, mul);
        boom::prf_seed(state, 3, 0xfedcba9876543211ULL);
        boom::prf_seed(state, 4, 13);
        dispatch(state, mul);
        boom::issue_module(state);
        boom::execute_module(state);
        boom::completion_service_execute(state);
        check(boom::prf_read(state, 11) == rv64m_reference(
                  mul_uopc, 0xfedcba9876543211ULL, 13),
              "MUL family result while divider busy");
        check(state.execute.divider.token_valid,
              "MUL family overlap preserves divider token");
        finish_divide(state, 0);
        check(boom::prf_read(state, 10) == UINT64_MAX / 19,
              "divider completes after MUL family overlap");
    }
}

int main() {
    const int legacy = m3b_integration_main();
    test_joint_arithmetic_matrix();
    test_joint_dependencies();
    test_all_mul_during_divider_busy();
    std::printf("M3C_RV64M_DIRECTED checks=%u failures=%u status=%s\n",
                checks, failures, failures == 0 && legacy == 0 ? "PASS" : "FAIL");
    return failures == 0 && legacy == 0 ? 0 : 1;
}
