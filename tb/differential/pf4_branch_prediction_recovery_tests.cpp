#include "boom_state.hpp"
#include "frontend.hpp"
#include "predictor.hpp"

#include <cstdint>
#include <cstdio>

namespace boom {
void branch_complete_event(BoomCoreState&, const RobCompleteEvent&);
void rename_module(BoomCoreState&);
}

namespace {

struct Checker {
    uint64_t checks;
    uint64_t failures;
    Checker() : checks(0), failures(0) {}
    void expect(bool condition, const char* name) {
        ++checks;
        if (!condition) {
            ++failures;
            if (failures <= 20) std::printf("PF4_FAIL,%s\n", name);
        }
    }
};

enum RefKind { REF_CONDITIONAL, REF_JAL, REF_JALR };

struct Scenario {
    RefKind kind;
    bool has_reference;
    bool cfi_match;
    bool prediction_valid;
    bool predicted_taken;
    bool target_valid;
    uint64_t predicted_target;
    bool actual_taken;
    uint64_t actual_target;
    uint64_t fallthrough;
    bool rvc;
    uint8_t lane;
    Scenario() : kind(REF_CONDITIONAL), has_reference(true), cfi_match(true),
        prediction_valid(true), predicted_taken(false), target_valid(false),
        predicted_target(0), actual_taken(false), actual_target(0),
        fallthrough(0), rvc(false), lane(0) {}
};

struct RefResult {
    bool correction;
    bool direction;
    bool target;
    bool stale;
    uint64_t next_pc;
};

// This model consumes only the test scenario. It neither calls nor mirrors an
// FTQ/product helper and intentionally classifies each architectural rule here.
RefResult reference_classify(const Scenario& x) {
    RefResult r = {false, false, false, false,
                   x.actual_taken ? x.actual_target : x.fallthrough};
    if (!x.has_reference || !x.cfi_match) {
        r.correction = true;
        r.stale = true;
        return r;
    }
    if (x.kind == REF_JALR) {
        r.correction = true;
        return r;
    }
    if (!x.prediction_valid) {
        r.correction = true;
        return r;
    }
    r.direction = x.predicted_taken != x.actual_taken;
    r.target = x.predicted_taken && x.actual_taken &&
        (!x.target_valid || x.predicted_target != x.actual_target);
    r.correction = r.direction || r.target;
    return r;
}

uint8_t cfi_type(RefKind kind) {
    if (kind == REF_JAL) return boom::CFI_JAL;
    if (kind == REF_JALR) return boom::CFI_JALR;
    return boom::CFI_CONDITIONAL_BRANCH;
}

MicroOp make_control(const Scenario& x) {
    MicroOp u;
    u.uopc = x.kind == REF_JAL ? 29 : (x.kind == REF_JALR ? 30 : 31);
    u.branch.is_br = x.kind == REF_CONDITIONAL;
    u.branch.is_jal = x.kind == REF_JAL;
    u.branch.is_jalr = x.kind == REF_JALR;
    u.branch.br_tag = 2;
    u.branch.br_mask = 1;
    u.debug_pc = x.fallthrough - (x.rvc ? 2u : 4u);
    u.is_rvc = x.rvc;
    u.ftq_lane = x.lane;
    u.queue.rob_idx = 5;
    u.queue.rob_allocation_id = 0x1234;
    return u;
}

void initialize_recovery(BoomCoreState& s, const MicroOp& u) {
    s.rob.state = ROB_NORMAL;
    s.rob.head = 4;
    s.rob.tail = 7;
    s.rob.entries[4].valid = true;
    s.rob.entries[4].uop.queue.rob_idx = 4;
    s.rob.entries[4].uop.queue.rob_allocation_id = 0x1000;
    s.rob.entries[5].valid = true;
    s.rob.entries[5].busy = true;
    s.rob.entries[5].uop = u;
    s.rob.entries[6].valid = true;
    s.rob.entries[6].busy = true;
    s.rob.entries[6].uop.queue.rob_idx = 6;
    s.rob.entries[6].uop.queue.rob_allocation_id = 0x1235;
    s.rob.entries[6].uop.branch.br_mask = 4;
    s.branch_state.active_mask = 5;
    s.branch_state.tag_valid[0] = true;
    s.branch_state.tag_valid[2] = true;
    s.branch_state.snapshot_valid[2] = true;
    for (int i = 0; i < LOGICAL_REG_COUNT; ++i) {
        s.rename.int_map_table.map_table[i] = static_cast<uint8_t>(i);
        s.rename.int_map_table.br_snapshots[i][2] = static_cast<uint8_t>(i);
    }
    s.rename.int_map_table.map_table[7] = 47;
}

void install_prediction(BoomCoreState& s, MicroOp& u, const Scenario& x) {
    if (!x.has_reference && !x.cfi_match) return;
    boom::FtqStepInput input;
    input.alloc_valid = true;
    input.allocation.packet_base_pc = u.debug_pc - (x.lane ? 4u : 0u);
    input.allocation.packet_valid_mask = 3;
    input.allocation.prediction_valid = x.prediction_valid;
    input.allocation.predicted_taken = x.predicted_taken;
    input.allocation.target_valid = x.target_valid;
    input.allocation.predicted_target = x.predicted_target;
    input.allocation.cfi_lane = x.cfi_match ? x.lane : static_cast<uint8_t>(x.lane ^ 1u);
    input.allocation.cfi_type = cfi_type(x.kind);
    input.allocation.predictor_metadata_index = 91;
    input.allocation.predictor_generation = 17;
    const boom::FtqStepOutput output = s.ftq.step(input);
    u.ftq_valid = true;
    u.ftq_idx = output.alloc_ftq_idx;
    u.ftq_generation = output.alloc_generation + (x.has_reference ? 0u : 1u);
}

void run_scenario(Checker& check, const Scenario& x) {
    const RefResult ref = reference_classify(x);
    BoomCoreState s;
    MicroOp u = make_control(x);
    install_prediction(s, u, x);
    initialize_recovery(s, u);
    const uint8_t map_before = s.rename.int_map_table.map_table[7];
    RobCompleteEvent event;
    event.valid = true;
    event.uop = u;
    event.actual_valid = true;
    event.actual_taken = x.actual_taken;
    event.actual_target = x.actual_target;
    event.fallthrough_pc = x.fallthrough;
    boom::branch_complete_event(s, event);

    check.expect(s.brupdate.valid, "completion_accepted");
    check.expect(s.brupdate.mispredict == ref.correction, "oracle_classification");
    check.expect(s.brupdate.direction_mispredict == ref.direction,
                 "direction_classification");
    check.expect(s.brupdate.target_mispredict == ref.target,
                 "target_classification");
    check.expect(s.brupdate.stale_lookup == ref.stale, "stale_classification");
    check.expect(s.brupdate.jalr_target == ref.next_pc, "architectural_next_pc");
    check.expect(s.brupdate.taken == x.actual_taken, "actual_direction_recorded");
    check.expect(s.brupdate.actual_target == x.actual_target, "actual_target_recorded");
    check.expect(s.brupdate.fallthrough_pc == x.fallthrough, "fallthrough_recorded");
    check.expect(s.brupdate.cfi_type == cfi_type(x.kind), "cfi_type_recorded");
    check.expect(s.ftq_redirect_pending.valid == (ref.correction && u.ftq_valid),
                 "redirect_work");
    check.expect(s.rob.entries[5].valid, "resolving_rob_survives");
    check.expect(s.rob.entries[6].valid == !ref.correction, "younger_rob_policy");
    check.expect(s.rename.int_map_table.map_table[7] ==
                 (ref.correction ? 7 : map_before), "map_recovery_policy");
    check.expect(s.branch_state.mispredicts == (ref.correction ? 1u : 0u),
                 "recovery_count");
    check.expect(s.branch_state.releases == (ref.correction ? 0u : 1u),
                 "correct_no_recovery_count");
}

void conditional_truth_table(Checker& check) {
    for (unsigned repeat = 0; repeat < 40; ++repeat) {
        for (unsigned lane = 0; lane < 2; ++lane) {
            for (unsigned compressed = 0; compressed < 2; ++compressed) {
                const uint64_t pc = 0x80000000ull + repeat * 0x100 + lane * 8;
                Scenario x;
                x.lane = static_cast<uint8_t>(lane);
                x.rvc = compressed != 0;
                x.fallthrough = pc + (x.rvc ? 2u : 4u);
                x.actual_target = pc + 0x40;
                x.predicted_target = x.actual_target;
                x.target_valid = true;

                x.predicted_taken = true; x.actual_taken = true;
                run_scenario(check, x);                         // T/T target correct
                x.predicted_target ^= 8; run_scenario(check, x); // T/T target wrong
                x.predicted_target = x.actual_target;
                x.actual_taken = false; run_scenario(check, x);  // T/NT
                x.predicted_taken = false; x.actual_taken = true;
                run_scenario(check, x);                          // NT/T
                x.actual_taken = false; run_scenario(check, x);  // NT/NT
            }
        }
    }
}

void stale_and_control_cases(Checker& check) {
    Scenario x;
    x.fallthrough = 0x81000004;
    x.actual_target = 0x81000100;
    x.predicted_target = x.actual_target;
    x.target_valid = true;
    x.actual_taken = false;
    x.predicted_taken = false;
    x.has_reference = false;
    run_scenario(check, x);
    x.has_reference = true;
    x.cfi_match = false;
    run_scenario(check, x);
    x.cfi_match = true;
    x.prediction_valid = false;
    run_scenario(check, x);
    x.prediction_valid = true;
    x.kind = REF_JAL;
    x.predicted_taken = true;
    x.actual_taken = true;
    run_scenario(check, x);
    x.kind = REF_JALR;
    run_scenario(check, x);
}

void same_packet_recovery(Checker& check, uint8_t lane) {
    Scenario x;
    x.lane = lane;
    x.fallthrough = 0x82000004 + lane * 4;
    x.actual_target = 0x82001000;
    x.predicted_taken = false;
    x.actual_taken = true;
    BoomCoreState s;
    MicroOp u = make_control(x);
    install_prediction(s, u, x);
    initialize_recovery(s, u);
    boom::FtqStepInput younger;
    younger.alloc_valid = true;
    younger.allocation.packet_valid_mask = 1;
    younger.allocation.packet_base_pc = 0x82000008;
    s.ftq.step(younger);
    RobCompleteEvent event;
    event.uop = u; event.actual_valid = true; event.actual_taken = true;
    event.actual_target = x.actual_target; event.fallthrough_pc = x.fallthrough;
    boom::branch_complete_event(s, event);
    boom::FtqStepInput redirect;
    redirect.redirect = s.ftq_redirect_pending;
    redirect.read_valid = true;
    redirect.read_ftq_idx = u.ftq_idx;
    redirect.read_generation = u.ftq_generation;
    const boom::FtqStepOutput out = s.ftq.step(redirect);
    check.expect(out.redirect_accepted, "same_packet_redirect_accepted");
    check.expect(out.count == 1, "younger_ftq_removed");
    check.expect(out.read_hit, "owner_ftq_retained");
    check.expect(out.read_entry.live_lane_mask == (lane ? 3u : 1u),
                 "same_packet_lane_survival");
    check.expect(!s.rob.entries[6].valid, "same_packet_younger_rob_removed");
}

MicroOp rename_one(BoomCoreState& s, const MicroOp& input, uint8_t rob_idx,
                   uint32_t allocation_id) {
    s.decode.dec_valids[0] = true;
    s.decode.dec_uops[0] = input;
    boom::rename_module(s);
    MicroOp output = s.rename.dispatch_packets[0].uop;
    output.queue.rob_idx = rob_idx;
    output.queue.rob_allocation_id = allocation_id;
    s.rob.entries[rob_idx].valid = true;
    s.rob.entries[rob_idx].busy = true;
    s.rob.entries[rob_idx].uop = output;
    s.rename.dispatch_packets[0] = RenameDispatchPacket();
    return output;
}

void rename_rollback(Checker& check) {
    BoomCoreState s;
    s.rob.state = ROB_NORMAL;
    MicroOp branch;
    branch.uopc = 31; branch.branch.is_br = true; branch.rename.dst_rtype = DST_N;
    branch = rename_one(s, branch, 0, 100);
    const uint8_t free_before = s.rename.int_free_list.count;
    MicroOp wrong;
    wrong.uopc = 50; wrong.rename.ldst = 9; wrong.rename.dst_rtype = DST_INT;
    wrong = rename_one(s, wrong, 1, 101);
    s.rob.head = 0; s.rob.tail = 2;
    const uint8_t wrong_pdst = wrong.rename.pdst;
    RobCompleteEvent event;
    event.uop = branch; event.actual_valid = true; event.actual_taken = true;
    event.actual_target = 0x83000100; event.fallthrough_pc = 0x83000004;
    boom::branch_complete_event(s, event);
    check.expect(s.rename.int_map_table.map_table[9] == 0, "rename_map_rollback");
    check.expect(s.rename.int_free_list.count == free_before, "rename_free_rollback");
    check.expect(!s.rename.int_free_list.busy_table[wrong_pdst], "rename_busy_rollback");
    check.expect(!s.rob.entries[1].valid, "rename_younger_rob_rollback");
}

void train_counter(boom::PredictorFoundation<256>& predictor, uint64_t pc,
                   bool taken, unsigned times) {
    for (unsigned i = 0; i < times; ++i) {
        boom::PredictorStepInput input;
        input.active_generation = 7;
        input.update.valid = true;
        input.update.commit_qualified = true;
        input.update.cfi_type = boom::CFI_CONDITIONAL_BRANCH;
        input.update.pc = pc;
        input.update.metadata_token = static_cast<uint16_t>((pc >> 1) & 255u);
        input.update.taken = taken;
        input.update.generation = 7;
        predictor.step(input);
    }
}

bool prediction(boom::PredictorFoundation<256>& predictor, uint64_t pc) {
    boom::PredictorStepInput request;
    request.active_generation = 7;
    request.req_valid = true;
    request.request.pc = pc;
    request.request.cfi_type = boom::CFI_CONDITIONAL_BRANCH;
    request.request.generation = 7;
    predictor.step(request);
    boom::PredictorStepInput consume;
    consume.active_generation = 7;
    consume.resp_ready = true;
    return predictor.step(consume).response.taken;
}

void bim_preservation(Checker& check) {
    for (unsigned state = 0; state < 4; ++state) {
        BoomCoreState dut;
        boom::PredictorFoundation<256> reference;
        const uint64_t pc = 0x84000000ull + state * 2;
        if (state == 0) { train_counter(dut.predictor, pc, false, 1); train_counter(reference, pc, false, 1); }
        if (state == 1) { train_counter(dut.predictor, pc, true, 1); train_counter(dut.predictor, pc, false, 1); train_counter(reference, pc, true, 1); train_counter(reference, pc, false, 1); }
        if (state == 2) { train_counter(dut.predictor, pc, true, 1); train_counter(reference, pc, true, 1); }
        if (state == 3) { train_counter(dut.predictor, pc, true, 2); train_counter(reference, pc, true, 2); }
        Scenario x;
        x.fallthrough = pc + 4; x.actual_target = pc + 16;
        x.predicted_taken = false; x.actual_taken = true;
        MicroOp u = make_control(x);
        initialize_recovery(dut, u);
        RobCompleteEvent event;
        event.uop = u; event.actual_valid = true; event.actual_taken = true;
        event.actual_target = x.actual_target; event.fallthrough_pc = x.fallthrough;
        boom::branch_complete_event(dut, event);
        check.expect(prediction(dut.predictor, pc) == prediction(reference, pc),
                     "bim_state_preserved");
        const bool transition_taken = state < 2;
        train_counter(dut.predictor, pc, transition_taken, 1);
        train_counter(reference, pc, transition_taken, 1);
        check.expect(prediction(dut.predictor, pc) == prediction(reference, pc),
                     "bim_exact_transition_preserved");
    }
}

void drain_imem_requests(PipeSignals& pipe) {
    while (!pipe.imem_req.empty()) (void)pipe.imem_req.read();
}

void inject_product_word(BoomCoreState& state, PipeSignals& pipe,
                         uint64_t pc, uint32_t instruction) {
    state.frontend.pc = pc;
    state.frontend.request_sent = true;
    state.frontend.pending_fetch_id = 19;
    state.frontend.pending_epoch = state.frontend.epoch;
    state.frontend.pending_address = pc & ~3ull;
    ImemResponse response;
    response.address = state.frontend.pending_address;
    response.fetch_id = state.frontend.pending_fetch_id;
    response.epoch = state.frontend.pending_epoch;
    response.instruction = instruction;
    pipe.imem_resp.write(response);
    boom::frontend_product_module(state, pipe);
    drain_imem_requests(pipe);
}

void product_frontend_steering(Checker& check, unsigned counter) {
    BoomCoreState state;
    PipeSignals pipe;
    state.product_ftq_enabled = true;
    state.frontend.reset_done = true;
    state.frontend.epoch = 9;
    state.rob.state = ROB_NORMAL;
    state.predictor_generation = 7;
    const uint64_t pc = 0x85000000ull + counter * 0x100;
    if (counter == 0) train_counter(state.predictor, pc, false, 1);
    if (counter == 2) train_counter(state.predictor, pc, true, 1);
    if (counter == 3) train_counter(state.predictor, pc, true, 2);

    // C.BEQZ in lane 0 followed by a valid younger C.NOP in lane 1.
    inject_product_word(state, pipe, pc, 0x0001c001u);
    const uint64_t predicted_target =
        state.frontend.pending_predecode.selected_cfi_result.static_target;
    check.expect(state.frontend.prediction_pending, "product_prediction_pending");
    boom::frontend_product_module(state, pipe);
    drain_imem_requests(pipe);

    const bool predicted_taken = counter >= 2;
    check.expect(state.frontend.predictor_prediction_valid,
                 "product_prediction_valid");
    check.expect(state.frontend.predictor_predicted_taken == predicted_taken,
                 "product_prediction_direction");
    check.expect(state.frontend.accepted_packet_mask ==
                 (predicted_taken ? 1u : 3u), "product_packet_mask");
    check.expect(state.frontend.fetch_buffer.count ==
                 (predicted_taken ? 1u : 2u), "product_fetch_buffer_mask");
    check.expect(state.frontend.pc ==
                 (predicted_taken ? predicted_target : pc + 4u),
                 "product_steering_pc");
    check.expect(state.frontend.ftq_alloc_accepted, "product_ftq_allocation");
    const boom::FtqPredictionLookup lookup = state.ftq.lookup_prediction(
        state.frontend.ftq_alloc_idx, state.frontend.ftq_alloc_generation, 0,
        boom::CFI_CONDITIONAL_BRANCH);
    check.expect(lookup.reference_valid && lookup.cfi_match,
                 "product_ftq_lookup_identity");
    check.expect(lookup.prediction_valid &&
                 lookup.predicted_taken == predicted_taken,
                 "product_ftq_prediction_metadata");
    check.expect(!predicted_taken || lookup.target_valid,
                 "product_taken_target_valid");

    if (!predicted_taken) return;
    Scenario recovery;
    recovery.predicted_taken = true;
    recovery.actual_taken = false;
    recovery.target_valid = true;
    recovery.predicted_target = predicted_target;
    recovery.actual_target = predicted_target;
    recovery.fallthrough = pc + 2;
    recovery.rvc = true;
    MicroOp uop = make_control(recovery);
    uop.ftq_valid = true;
    uop.ftq_idx = state.frontend.ftq_alloc_idx;
    uop.ftq_generation = state.frontend.ftq_alloc_generation;
    uop.ftq_lane = 0;
    initialize_recovery(state, uop);
    RobCompleteEvent event;
    event.uop = uop;
    event.actual_valid = true;
    event.actual_taken = false;
    event.actual_target = predicted_target;
    event.fallthrough_pc = pc + 2;
    boom::branch_complete_event(state, event);
    check.expect(state.brupdate.mispredict, "product_taken_to_nt_mispredict");
    check.expect(state.brupdate.jalr_target == pc + 2,
                 "product_rvc_fallthrough_target");
    boom::frontend_product_module(state, pipe);
    drain_imem_requests(pipe);
    check.expect(state.frontend.pc == pc + 2,
                 "product_fallthrough_refetch");
}

}  // namespace

int main() {
    Checker check;
    conditional_truth_table(check);
    stale_and_control_cases(check);
    same_packet_recovery(check, 0);
    same_packet_recovery(check, 1);
    rename_rollback(check);
    bim_preservation(check);
    for (unsigned counter = 0; counter < 4; ++counter)
        product_frontend_steering(check, counter);
    check.expect(check.checks >= 5000, "mandatory_check_floor");
    std::printf("PF4_NATIVE_%s checks=%llu failures=%llu truth_cases=5 rvc=2 lanes=2 bim_states=4\n",
                check.failures == 0 ? "PASS" : "FAIL",
                static_cast<unsigned long long>(check.checks),
                static_cast<unsigned long long>(check.failures));
    return check.failures == 0 ? 0 : 1;
}
