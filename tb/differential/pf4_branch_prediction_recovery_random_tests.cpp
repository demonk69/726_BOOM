#include "boom_state.hpp"

#include <cstdint>
#include <cstdio>

namespace boom { void branch_complete_event(BoomCoreState&, const RobCompleteEvent&); }

namespace {

const unsigned kSeeds = 256;
const unsigned kCycles = 8192;
const uint64_t kLongSteps = 1000000;

struct Rng {
    uint64_t value;
    explicit Rng(unsigned seed) : value(0xd1b54a32d192ed03ULL ^ seed) {}
    uint32_t next() {
        value ^= value >> 12; value ^= value << 25; value ^= value >> 27;
        return static_cast<uint32_t>(value * 0x2545f4914f6cdd1dULL);
    }
};

struct Case {
    uint8_t kind, lane;
    bool rvc, reference, match, prediction_valid, predicted_taken, target_valid;
    bool actual_taken;
    uint64_t pc, predicted_target, actual_target;
};

struct Expected {
    bool correction, direction, target, stale;
};

Expected classify_reference(const Case& x) {
    Expected e = {false, false, false, false};
    if (!x.reference || !x.match) {
        e.correction = e.stale = true;
    } else if (!x.prediction_valid) {
        e.correction = true;
    } else {
        e.direction = x.predicted_taken != x.actual_taken;
        e.target = x.predicted_taken && x.actual_taken &&
            (!x.target_valid || x.predicted_target != x.actual_target);
        e.correction = x.kind == boom::CFI_JALR || e.direction || e.target;
    }
    return e;
}

struct Errors {
    uint64_t classification_error, direction_error, target_error, stale_error;
    uint64_t redirect_error, redirect_pc_error, no_work_error, rob_recovery_error;
    uint64_t ftq_reference_error, generation_error, cfi_mismatch_error;
    uint64_t lane_error, rvc_error, jal_error, jalr_error;
    uint64_t rename_rollback_error, predictor_training_error, ordering_error;
    Errors() : classification_error(0), direction_error(0), target_error(0),
        stale_error(0), redirect_error(0), redirect_pc_error(0), no_work_error(0),
        rob_recovery_error(0), ftq_reference_error(0), generation_error(0),
        cfi_mismatch_error(0), lane_error(0), rvc_error(0), jal_error(0),
        jalr_error(0), rename_rollback_error(0), predictor_training_error(0),
        ordering_error(0) {}
    uint64_t total() const {
        return classification_error + direction_error + target_error + stale_error +
            redirect_error + redirect_pc_error + no_work_error + rob_recovery_error +
            ftq_reference_error + generation_error + cfi_mismatch_error + lane_error +
            rvc_error + jal_error + jalr_error + rename_rollback_error +
            predictor_training_error + ordering_error;
    }
};

struct Coverage {
    uint64_t correct_t, correct_nt, nt_to_t, t_to_nt, lane0, lane1, rvc, base32;
    Coverage() : correct_t(0), correct_nt(0), nt_to_t(0), t_to_nt(0),
        lane0(0), lane1(0), rvc(0), base32(0) {}
};

void run_case(const Case& x, Errors& errors, Coverage& coverage) {
    const Expected expected = classify_reference(x);
    BoomCoreState state;
    boom::FtqStepInput alloc;
    alloc.alloc_valid = true;
    alloc.allocation.packet_base_pc = x.pc - (x.lane ? 4u : 0u);
    alloc.allocation.packet_valid_mask = 3;
    alloc.allocation.prediction_valid = x.prediction_valid;
    alloc.allocation.predicted_taken = x.predicted_taken;
    alloc.allocation.target_valid = x.target_valid;
    alloc.allocation.predicted_target = x.predicted_target;
    alloc.allocation.cfi_lane = x.match ? x.lane : static_cast<uint8_t>(x.lane ^ 1u);
    alloc.allocation.cfi_type = x.kind;
    const boom::FtqStepOutput allocated = state.ftq.step(alloc);

    MicroOp u;
    u.uopc = x.kind == boom::CFI_JAL ? 29 : (x.kind == boom::CFI_JALR ? 30 : 31);
    u.branch.is_br = x.kind == boom::CFI_CONDITIONAL_BRANCH;
    u.branch.is_jal = x.kind == boom::CFI_JAL;
    u.branch.is_jalr = x.kind == boom::CFI_JALR;
    u.branch.br_tag = 1;
    u.ftq_valid = true;
    u.ftq_idx = allocated.alloc_ftq_idx;
    u.ftq_generation = allocated.alloc_generation + (x.reference ? 0u : 1u);
    u.ftq_lane = x.lane;
    u.is_rvc = x.rvc;
    u.debug_pc = x.pc;
    u.queue.rob_idx = 0;
    u.queue.rob_allocation_id = 99;
    state.rob.state = ROB_NORMAL;
    state.rob.entries[0].valid = true;
    state.rob.entries[0].busy = true;
    state.rob.entries[0].uop = u;
    state.rob.entries[1].valid = true;
    state.rob.entries[1].uop.queue.rob_idx = 1;
    state.rob.entries[1].uop.queue.rob_allocation_id = 100;
    state.rob.entries[1].uop.branch.br_mask = 2;
    state.rob.head = 0; state.rob.tail = 2;
    state.branch_state.active_mask = 2;
    state.branch_state.tag_valid[1] = true;
    state.branch_state.snapshot_valid[1] = true;
    state.rename.int_map_table.map_table[3] = 44;
    state.rename.int_map_table.br_snapshots[3][1] = 3;

    RobCompleteEvent event;
    event.uop = u; event.actual_valid = true; event.actual_taken = x.actual_taken;
    event.actual_target = x.actual_target;
    event.fallthrough_pc = x.pc + (x.rvc ? 2u : 4u);
    boom::branch_complete_event(state, event);

    if (state.brupdate.mispredict != expected.correction) ++errors.classification_error;
    if (state.brupdate.direction_mispredict != expected.direction) ++errors.direction_error;
    if (state.brupdate.target_mispredict != expected.target) ++errors.target_error;
    if (state.brupdate.stale_lookup != expected.stale) ++errors.stale_error;
    if (state.ftq_redirect_pending.valid != expected.correction) ++errors.redirect_error;
    const uint64_t next = x.actual_taken ? x.actual_target : event.fallthrough_pc;
    if (state.brupdate.jalr_target != next) ++errors.redirect_pc_error;
    if (state.rob.entries[1].valid == expected.correction) ++errors.rob_recovery_error;
    if (!expected.correction && state.rename.int_map_table.map_table[3] != 44)
        ++errors.no_work_error;
    if (expected.correction && state.rename.int_map_table.map_table[3] != 3)
        ++errors.rename_rollback_error;
    if (state.brupdate.prediction_lookup_valid != x.reference) ++errors.ftq_reference_error;
    if (!x.reference && !state.brupdate.stale_lookup) ++errors.generation_error;
    if (!x.match && !state.brupdate.stale_lookup) ++errors.cfi_mismatch_error;
    if (state.ftq_redirect_pending.surviving_lane_mask !=
        (expected.correction ? static_cast<uint8_t>((1u << (x.lane + 1u)) - 1u) : 0u))
        ++errors.lane_error;
    if (!x.actual_taken && state.brupdate.jalr_target != x.pc + (x.rvc ? 2u : 4u))
        (x.rvc ? ++errors.rvc_error : ++errors.redirect_pc_error);
    if (x.kind == boom::CFI_JAL && state.brupdate.cfi_type != boom::CFI_JAL) ++errors.jal_error;
    if (x.kind == boom::CFI_JALR && !state.brupdate.mispredict) ++errors.jalr_error;
    if (state.rob.entries[0].valid == false) ++errors.ordering_error;
    boom::PredictorStepInput probe;
    probe.req_valid = true;
    probe.request.pc = x.pc;
    probe.request.cfi_type = boom::CFI_CONDITIONAL_BRANCH;
    state.predictor.step(probe);
    boom::PredictorStepInput consume;
    consume.resp_ready = true;
    const boom::PredictorStepOutput prediction = state.predictor.step(consume);
    if (!prediction.resp_valid || prediction.response.taken)
        ++errors.predictor_training_error;

    if (!expected.correction && x.actual_taken) ++coverage.correct_t;
    if (!expected.correction && !x.actual_taken) ++coverage.correct_nt;
    if (x.kind == boom::CFI_CONDITIONAL_BRANCH && !x.predicted_taken && x.actual_taken)
        ++coverage.nt_to_t;
    if (x.kind == boom::CFI_CONDITIONAL_BRANCH && x.predicted_taken && !x.actual_taken)
        ++coverage.t_to_nt;
    x.lane ? ++coverage.lane1 : ++coverage.lane0;
    x.rvc ? ++coverage.rvc : ++coverage.base32;
}

Case random_case(Rng& rng, uint64_t ordinal) {
    const uint32_t a = rng.next();
    Case x;
    x.kind = static_cast<uint8_t>((a % 10u) < 8u ? boom::CFI_CONDITIONAL_BRANCH :
        ((a % 10u) == 8u ? boom::CFI_JAL : boom::CFI_JALR));
    x.lane = static_cast<uint8_t>((a >> 4) & 1u);
    x.rvc = ((a >> 5) & 1u) != 0;
    x.reference = (a & 31u) != 0;
    x.match = ((a >> 6) & 31u) != 0;
    x.prediction_valid = ((a >> 11) & 15u) != 0;
    x.predicted_taken = ((a >> 15) & 1u) != 0;
    x.actual_taken = ((a >> 16) & 1u) != 0;
    if (x.kind != boom::CFI_CONDITIONAL_BRANCH) x.actual_taken = true;
    if (x.kind == boom::CFI_JAL) x.predicted_taken = true;
    x.target_valid = ((a >> 17) & 7u) != 0;
    x.pc = 0x90000000ull + ordinal * 8u;
    x.actual_target = x.pc + 0x80;
    x.predicted_target = ((a >> 20) & 3u) ? x.actual_target : x.actual_target + 8;
    return x;
}

}  // namespace

int main() {
    Errors errors;
    Coverage coverage;
    uint64_t ordinal = 0;
    for (unsigned seed = 0; seed < kSeeds; ++seed) {
        Rng rng(seed);
        for (unsigned cycle = 0; cycle < kCycles; ++cycle)
            run_case(random_case(rng, ordinal++), errors, coverage);
    }
    std::printf("PF4_RANDOM_%s seeds=%u cycles_per_seed=%u classification_error=%llu direction_error=%llu target_error=%llu stale_error=%llu redirect_error=%llu redirect_pc_error=%llu no_work_error=%llu rob_recovery_error=%llu ftq_reference_error=%llu generation_error=%llu cfi_mismatch_error=%llu lane_error=%llu rvc_error=%llu jal_error=%llu jalr_error=%llu rename_rollback_error=%llu predictor_training_error=%llu ordering_error=%llu correct_t=%llu correct_nt=%llu nt_to_t=%llu t_to_nt=%llu lane0=%llu lane1=%llu rvc=%llu base32=%llu\n",
        errors.total() == 0 ? "PASS" : "FAIL", kSeeds, kCycles,
        (unsigned long long)errors.classification_error, (unsigned long long)errors.direction_error,
        (unsigned long long)errors.target_error, (unsigned long long)errors.stale_error,
        (unsigned long long)errors.redirect_error, (unsigned long long)errors.redirect_pc_error,
        (unsigned long long)errors.no_work_error, (unsigned long long)errors.rob_recovery_error,
        (unsigned long long)errors.ftq_reference_error, (unsigned long long)errors.generation_error,
        (unsigned long long)errors.cfi_mismatch_error, (unsigned long long)errors.lane_error,
        (unsigned long long)errors.rvc_error, (unsigned long long)errors.jal_error,
        (unsigned long long)errors.jalr_error, (unsigned long long)errors.rename_rollback_error,
        (unsigned long long)errors.predictor_training_error, (unsigned long long)errors.ordering_error,
        (unsigned long long)coverage.correct_t, (unsigned long long)coverage.correct_nt,
        (unsigned long long)coverage.nt_to_t, (unsigned long long)coverage.t_to_nt,
        (unsigned long long)coverage.lane0, (unsigned long long)coverage.lane1,
        (unsigned long long)coverage.rvc, (unsigned long long)coverage.base32);

    Errors long_errors;
    Coverage long_coverage;
    Rng long_rng(0x504634u);
    for (uint64_t step = 0; step < kLongSteps; ++step)
        run_case(random_case(long_rng, ordinal++), long_errors, long_coverage);
    std::printf("PF4_LONG_RUN_%s steps=%llu errors=%llu correct_t=%llu correct_nt=%llu nt_to_t=%llu t_to_nt=%llu\n",
        long_errors.total() == 0 ? "PASS" : "FAIL",
        (unsigned long long)kLongSteps, (unsigned long long)long_errors.total(),
        (unsigned long long)long_coverage.correct_t, (unsigned long long)long_coverage.correct_nt,
        (unsigned long long)long_coverage.nt_to_t, (unsigned long long)long_coverage.t_to_nt);
    const bool covered = coverage.correct_t && coverage.correct_nt && coverage.nt_to_t &&
        coverage.t_to_nt && long_coverage.correct_t && long_coverage.correct_nt &&
        long_coverage.nt_to_t && long_coverage.t_to_nt;
    return errors.total() == 0 && long_errors.total() == 0 && covered ? 0 : 1;
}
