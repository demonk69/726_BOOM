#include "boom_config.hpp"
#include "boom_state.hpp"
#include <cstdio>

namespace boom {
void execute_module(BoomCoreState& state);
void issue_module(BoomCoreState& state);
}

static int failures = 0;

#define CHECK(condition, message) do { \
    if (!(condition)) { std::printf("FAIL: %s\n", message); failures++; } \
} while (0)

static MicroOp make_add(uint8_t rob_idx, uint8_t pdst) {
    MicroOp uop;
    uop.uopc = 1;
    uop.iq_type = IQ_ALU;
    uop.fu_code = FU_ALU;
    uop.queue.rob_idx = rob_idx;
    uop.rename.pdst = pdst;
    uop.rename.dst_rtype = DST_INT;
    return uop;
}

int main() {
    BoomCoreState state;
    CHECK(sizeof(state.issue.issued_valids) / sizeof(state.issue.issued_valids[0]) == ISSUE_WIDTH,
          "issue valid interface width mismatch");
    CHECK(sizeof(state.execute.alu_results) / sizeof(state.execute.alu_results[0]) == ISSUE_WIDTH,
          "execute result interface width mismatch");

    for (int i = 0; i < 2; i++) {
        IssueSlotEntry& entry = state.issue.alu_iq.entries[i];
        entry.valid = true;
        entry.request = true;
        entry.uop = make_add((uint8_t)i, (uint8_t)(i + 1));
    }
    state.issue.alu_iq.count = 2;
    state.issue.alu_iq.tail = 2;

    boom::issue_module(state);
    CHECK(state.issue.issued_valids[INT_ISSUE_LANE], "fixed INT intake did not accept the ALU uop");
    CHECK(!state.issue.issued_valids[MEM_ISSUE_LANE], "ALU uop activated MEM intake");
    CHECK(!state.issue.issued_valids[FP_ISSUE_LANE], "reserved FP lane became active");
    for (int lane = 0; lane < ISSUE_WIDTH; lane++) {
        state.execute.alu_results[lane].valid = true;
    }
    state.execute.alu_results[MEM_ISSUE_LANE].result = 0x55;
    state.execute.alu_results[INT_ISSUE_LANE].valid = false;

    boom::execute_module(state);
    CHECK(state.execute.alu_results[INT_ISSUE_LANE].valid, "INT lane did not produce a result");
    CHECK(state.execute.alu_results[MEM_ISSUE_LANE].valid &&
          state.execute.alu_results[MEM_ISSUE_LANE].result == 0x55,
          "held MEM completion was overwritten");
    CHECK(!state.execute.alu_results[FP_ISSUE_LANE].valid, "reserved execute lane was not cleared");
    CHECK(state.issue.alu_iq.count == 1, "same-class queue entry was dropped");

    if (failures != 0) return 1;
    std::printf("Gate 4.0 W1 lane interface tests: PASS\n");
    return 0;
}
