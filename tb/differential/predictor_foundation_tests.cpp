#include "predictor.hpp"

#include <cstdint>
#include <cstdio>

struct Checker {
    uint64_t checks;
    uint64_t failures;
    Checker() : checks(0), failures(0) {}
    void expect(bool condition, const char* label) {
        ++checks;
        if (!condition) {
            if (failures < 20) std::printf("FAIL,%s\n", label);
            ++failures;
        }
    }
};

static boom::PredictorRequest request(uint64_t pc, uint8_t type,
                                      uint64_t token, uint32_t generation) {
    boom::PredictorRequest r;
    r.pc = pc;
    r.cfi_lane = static_cast<uint8_t>((pc >> 1) & 1u);
    r.cfi_type = type;
    r.static_target_valid = true;
    r.static_target = pc + 0x40;
    r.generation = generation;
    r.request_token = token;
    return r;
}

template <std::size_t Entries>
static boom::PredictorResponse lookup(boom::PredictorFoundation<Entries>& dut,
                                      const boom::PredictorRequest& request_value,
                                      Checker& c) {
    boom::PredictorStepInput in;
    in.active_generation = request_value.generation;
    in.req_valid = true;
    in.request = request_value;
    boom::PredictorStepOutput out = dut.step(in);
    c.expect(out.req_ready && !out.resp_valid, "lookup_accept");
    in.req_valid = false;
    out = dut.step(in);
    c.expect(!out.req_ready && out.resp_valid, "lookup_one_cycle");
    const boom::PredictorResponse response = out.response;
    in.resp_ready = true;
    out = dut.step(in);
    c.expect(out.resp_valid && !out.req_ready, "consume_still_blocks");
    return response;
}

template <std::size_t Entries>
static void train(boom::PredictorFoundation<Entries>& dut, uint64_t pc,
                  bool taken, uint32_t generation, uint16_t token) {
    boom::PredictorStepInput in;
    in.active_generation = generation;
    in.update.valid = true;
    in.update.commit_qualified = true;
    in.update.cfi_type = boom::CFI_CONDITIONAL_BRANCH;
    in.update.pc = pc;
    in.update.metadata_token = token;
    in.update.taken = taken;
    in.update.generation = generation;
    dut.step(in);
}

template <std::size_t Entries>
static void run_depth(Checker& c) {
    boom::PredictorFoundation<Entries> dut;
    const uint32_t generation = static_cast<uint32_t>(Entries + 7);
    boom::PredictorStepInput reset;
    reset.reset = true;
    reset.active_generation = generation;
    boom::PredictorStepOutput out = dut.step(reset);
    c.expect(!out.req_ready && !out.resp_valid, "reset_outputs_idle");

    for (std::size_t i = 0; i < Entries; ++i) {
        const uint64_t pc = 0x10000u + static_cast<uint64_t>(i << 1);
        const boom::PredictorResponse r = lookup(
            dut, request(pc, boom::CFI_CONDITIONAL_BRANCH, i, generation), c);
        c.expect(r.prediction_valid && !r.taken && !r.target_valid,
                 "logical_weak_nt");
        c.expect(r.metadata_token == i && r.generation == generation &&
                 r.request_token == i, "response_identity");
    }

    const uint64_t pc = 0x2468;
    const uint16_t index = static_cast<uint16_t>((pc >> 1) & (Entries - 1));
    train(dut, pc, true, generation, index);       // 01 -> 10
    c.expect(lookup(dut, request(pc, boom::CFI_CONDITIONAL_BRANCH, 1, generation), c).taken,
             "transition_01_10");
    train(dut, pc, true, generation, index);       // 10 -> 11
    train(dut, pc, true, generation, index);       // 11 -> 11
    c.expect(lookup(dut, request(pc, boom::CFI_CONDITIONAL_BRANCH, 2, generation), c).taken,
             "saturate_11");
    train(dut, pc, false, generation, index);      // 11 -> 10
    c.expect(lookup(dut, request(pc, boom::CFI_CONDITIONAL_BRANCH, 3, generation), c).taken,
             "transition_11_10");
    train(dut, pc, false, generation, index);      // 10 -> 01
    c.expect(!lookup(dut, request(pc, boom::CFI_CONDITIONAL_BRANCH, 4, generation), c).taken,
             "transition_10_01");
    train(dut, pc, false, generation, index);      // 01 -> 00
    train(dut, pc, false, generation, index);      // 00 -> 00
    c.expect(!lookup(dut, request(pc, boom::CFI_CONDITIONAL_BRANCH, 5, generation), c).taken,
             "saturate_00");

    boom::PredictorStepInput conflict;
    conflict.active_generation = generation;
    conflict.req_valid = true;
    conflict.request = request(pc, boom::CFI_CONDITIONAL_BRANCH, 6, generation);
    conflict.update.valid = conflict.update.commit_qualified = true;
    conflict.update.cfi_type = boom::CFI_CONDITIONAL_BRANCH;
    conflict.update.pc = pc;
    conflict.update.metadata_token = index;
    conflict.update.taken = true;
    conflict.update.generation = generation;
    dut.step(conflict);
    conflict = boom::PredictorStepInput();
    conflict.active_generation = generation;
    out = dut.step(conflict);
    c.expect(out.resp_valid && !out.response.taken, "forward_00_01");
    conflict.resp_ready = true;
    dut.step(conflict);

    boom::PredictorStepInput stale;
    stale.active_generation = generation;
    stale.update.valid = stale.update.commit_qualified = true;
    stale.update.cfi_type = boom::CFI_CONDITIONAL_BRANCH;
    stale.update.pc = pc;
    stale.update.metadata_token = index;
    stale.update.taken = true;
    stale.update.generation = generation - 1;
    dut.step(stale);
    train(dut, pc, true, generation, static_cast<uint16_t>((index + 1) & (Entries - 1)));
    c.expect(!lookup(dut, request(pc, boom::CFI_CONDITIONAL_BRANCH, 7, generation), c).taken,
             "stale_and_bad_metadata_dropped");

    boom::PredictorStepInput issue;
    issue.active_generation = generation;
    issue.req_valid = true;
    issue.request = request(pc, boom::CFI_JAL, 0xabc, generation);
    dut.step(issue);
    issue.req_valid = false;
    out = dut.step(issue);
    const boom::PredictorResponse held = out.response;
    for (unsigned i = 0; i < 8; ++i) {
        issue.req_valid = true;
        issue.request.request_token++;
        out = dut.step(issue);
        c.expect(out.resp_valid && !out.req_ready &&
                 out.response.request_token == held.request_token &&
                 out.response.target == held.target, "held_response_stable");
    }
    issue.reset = true;
    issue.active_generation = generation + 1;
    out = dut.step(issue);
    c.expect(!out.resp_valid && !out.req_ready, "reset_clears_pending");
    issue = boom::PredictorStepInput();
    issue.active_generation = generation + 1;
    out = dut.step(issue);
    c.expect(out.req_ready && !out.resp_valid, "ready_after_reset");

    boom::PredictorRequest jal = request(pc, boom::CFI_JAL, 8, generation + 1);
    boom::PredictorResponse r = lookup(dut, jal, c);
    c.expect(r.prediction_valid && r.taken && r.target_valid &&
             r.target == jal.static_target, "jal_prediction");
    jal.static_target_valid = false;
    r = lookup(dut, jal, c);
    c.expect(r.prediction_valid && r.taken && !r.target_valid && r.target == 0,
             "jal_without_target");
    r = lookup(dut, request(pc, boom::CFI_JALR, 9, generation + 1), c);
    c.expect(!r.prediction_valid && !r.taken && !r.target_valid,
             "jalr_unpredicted");
    r = lookup(dut, request(pc, boom::CFI_NONE, 10, generation + 1), c);
    c.expect(!r.prediction_valid && !r.taken && !r.target_valid,
             "none_unpredicted");
}

int main() {
    Checker c;
    run_depth<64>(c);
    run_depth<128>(c);
    run_depth<256>(c);
    run_depth<512>(c);
    std::printf("PREDICTOR_FOUNDATION_DIRECTED,checks=%llu,failures=%llu\n",
                static_cast<unsigned long long>(c.checks),
                static_cast<unsigned long long>(c.failures));
    return c.failures == 0 && c.checks >= 500 ? 0 : 1;
}
