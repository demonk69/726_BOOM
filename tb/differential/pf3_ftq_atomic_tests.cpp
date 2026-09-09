#include "boom_interfaces.hpp"
#include "boom_state.hpp"
#include "frontend.hpp"
#include "predecode.hpp"

#include <cstdint>
#include <cstdio>

namespace boom {
void rob_commit_module(BoomCoreState&, PipeSignals&);
void branch_complete_event(BoomCoreState&, const MicroOp&, bool, uint64_t);
void exception_recovery_apply(BoomCoreState&, const RobEntry&);
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
            if (failures < 16) std::printf("PF3_FAIL,%s\n", name);
        }
    }
};

boom::FetchInstruction instruction(uint64_t pc, uint32_t bits, bool rvc = false) {
    boom::FetchInstruction value;
    value.pc = pc;
    value.instruction = bits;
    value.original_instruction = rvc ? 1u : bits;
    value.fetch_id = static_cast<uint32_t>(pc);
    value.is_rvc = rvc;
    return value;
}

void initialize(BoomCoreState& state) {
    state.product_ftq_enabled = true;
    state.frontend.reset_done = true;
    state.frontend.pc = 1;
    state.frontend.epoch = 7;
}

void prepare_packet(BoomCoreState& state, uint64_t pc, uint8_t mask,
                    uint32_t lane0, uint32_t lane1 = 0x00000013u,
                    bool lane0_rvc = false, bool lane1_rvc = false) {
    FrontendState& fe = state.frontend;
    fe.pending_packet = boom::FetchPacket();
    fe.pending_packet.valid = mask != 0;
    fe.pending_packet.valid_mask = mask;
    fe.pending_packet.slots[0] = instruction(pc, lane0, lane0_rvc);
    fe.pending_packet.slots[1] = instruction(pc + (lane0_rvc ? 2u : 4u), lane1,
                                             lane1_rvc);
    fe.original_packet_mask = mask;
    fe.final_admission_mask = mask;
    fe.pending_predecode = boom::predecode_cfi_packet(
        mask, fe.pending_packet.slots[0].pc, lane0, lane0_rvc,
        fe.pending_packet.slots[1].pc, lane1, lane1_rvc);
    fe.prediction_pending = false;
    fe.prediction_resolved = false;
    if (fe.pending_predecode.packet_has_cfi &&
        fe.pending_predecode.selected_cfi_result.cfi_type ==
            boom::CFI_CONDITIONAL_BRANCH) {
        fe.prediction_pending = true;
        fe.predictor_request_sent = true;
        fe.prediction_resolved = true;
        fe.predictor_prediction_valid = true;
        fe.predictor_predicted_taken = true;
        fe.predictor_target_valid = true;
        fe.predictor_target = pc + 8;
        fe.predictor_metadata_index = static_cast<uint8_t>((pc >> 1) & 0xffu);
    }
}

void request_read(BoomCoreState& state, uint8_t idx, uint32_t generation,
                  PipeSignals& pipe) {
    state.ftq_read_request.valid = true;
    state.ftq_read_request.ftq_idx = idx;
    state.ftq_read_request.generation = generation;
    boom::frontend_module(state, pipe);
}

void test_admission_and_metadata(Checker& check) {
    const uint32_t nop = 0x00000013u;
    {
        BoomCoreState state;
        PipeSignals pipe;
        initialize(state);
        prepare_packet(state, 0x80000000ull, 3, nop, nop);
        boom::frontend_module(state, pipe);
        check.expect(state.frontend.packet_accept, "two_lane_accept");
        check.expect(state.frontend.ftq_alloc_accepted, "two_lane_allocate");
        check.expect(state.frontend.fetch_buffer.count == 2, "two_lane_fb_count");
        const boom::FetchInstruction& a = state.frontend.fetch_buffer.entries[0];
        const boom::FetchInstruction& b = state.frontend.fetch_buffer.entries[1];
        check.expect(a.ftq_valid && b.ftq_valid, "two_lane_reference_valid");
        check.expect(a.ftq_idx == b.ftq_idx && a.ftq_generation == b.ftq_generation,
                     "packet_shared_owner");
        check.expect(a.ftq_lane == 0 && b.ftq_lane == 1, "packet_lane_identity");
        check.expect(a.ftq_halfword_offset == 0 && b.ftq_halfword_offset == 2,
                     "base32_offsets");
        request_read(state, a.ftq_idx, a.ftq_generation, pipe);
        check.expect(state.ftq_last_output.read_hit, "two_lane_lookup");
        check.expect(state.ftq_last_output.read_entry.packet_valid_mask == 3 &&
                     state.ftq_last_output.read_entry.live_lane_mask == 3,
                     "two_lane_live_mask");
    }
    {
        BoomCoreState state;
        PipeSignals pipe;
        initialize(state);
        prepare_packet(state, 0x80000100ull, 1, 0x0080006fu);
        boom::frontend_module(state, pipe);
        const uint8_t idx = state.frontend.ftq_alloc_idx;
        const uint32_t generation = state.frontend.ftq_alloc_generation;
        request_read(state, idx, generation, pipe);
        const boom::FtqEntry& entry = state.ftq_last_output.read_entry;
        check.expect(state.ftq_last_output.read_hit, "jal_lookup");
        check.expect(entry.packet_valid_mask == 1 && entry.live_lane_mask == 1,
                     "jal_mask01");
        check.expect(entry.prediction_valid && entry.predicted_taken &&
                     entry.target_valid && entry.predicted_target == 0x80000108ull,
                     "jal_metadata");
        check.expect(entry.cfi_lane == 0 && entry.cfi_type == boom::CFI_JAL,
                     "jal_cfi");
    }
    {
        BoomCoreState state;
        PipeSignals pipe;
        initialize(state);
        prepare_packet(state, 0x80000200ull, 3, 0x00000463u, nop);
        boom::frontend_module(state, pipe);
        const uint8_t idx = state.frontend.ftq_alloc_idx;
        const uint32_t generation = state.frontend.ftq_alloc_generation;
        request_read(state, idx, generation, pipe);
        const boom::FtqEntry& entry = state.ftq_last_output.read_entry;
        check.expect(state.ftq_last_output.read_hit, "conditional_lookup");
        check.expect(entry.packet_valid_mask == 3 && entry.live_lane_mask == 3,
                     "conditional_shadow_mask11");
        check.expect(entry.prediction_valid && entry.predicted_taken &&
                     entry.target_valid && entry.predicted_target == 0x80000208ull,
                     "conditional_metadata");
        check.expect(state.frontend.pc == 1, "conditional_no_steering");
    }
    {
        BoomCoreState state;
        PipeSignals pipe;
        initialize(state);
        prepare_packet(state, 0x80000300ull, 3, 0x00008067u, nop);
        boom::frontend_module(state, pipe);
        const uint8_t idx = state.frontend.ftq_alloc_idx;
        const uint32_t generation = state.frontend.ftq_alloc_generation;
        request_read(state, idx, generation, pipe);
        const boom::FtqEntry& entry = state.ftq_last_output.read_entry;
        check.expect(!entry.prediction_valid && !entry.target_valid &&
                     entry.predicted_target == 0, "jalr_unpredicted");
        check.expect(entry.cfi_type == boom::CFI_JALR, "jalr_cfi");
    }
    {
        BoomCoreState state;
        PipeSignals pipe;
        initialize(state);
        prepare_packet(state, 0x80000400ull, 3, 0x0001u, 0x0001u, true, true);
        boom::frontend_module(state, pipe);
        const boom::FetchInstruction& a = state.frontend.fetch_buffer.entries[0];
        const boom::FetchInstruction& b = state.frontend.fetch_buffer.entries[1];
        check.expect(a.ftq_halfword_offset == 0 && b.ftq_halfword_offset == 1,
                     "rvc_offsets_0_2");
    }
    {
        BoomCoreState state;
        PipeSignals pipe;
        initialize(state);
        prepare_packet(state, 0x80000500ull, 3, nop, 0x0001u, false, true);
        boom::frontend_module(state, pipe);
        check.expect(state.frontend.fetch_buffer.entries[1].ftq_halfword_offset == 2,
                     "mixed_offset_plus4");
    }
}

void test_backpressure_and_turnover(Checker& check) {
    BoomCoreState state;
    PipeSignals pipe;
    initialize(state);
    const uint32_t nop = 0x00000013u;
    uint8_t first_idx = 0;
    uint32_t first_generation = 0;
    for (unsigned i = 0; i < FTQ_DEPTH; ++i) {
        state.frontend.fetch_buffer = boom::FetchBufferState();
        prepare_packet(state, 0x81000000ull + i * 4u, 1, nop);
        boom::frontend_module(state, pipe);
        check.expect(state.frontend.packet_accept, "fill_atomic_accept");
        if (i == 0) {
            first_idx = state.frontend.ftq_alloc_idx;
            first_generation = state.frontend.ftq_alloc_generation;
        }
    }
    check.expect(state.ftq_last_output.full && state.ftq_last_output.count == FTQ_DEPTH,
                 "ftq_full");
    state.frontend.fetch_buffer = boom::FetchBufferState();
    prepare_packet(state, 0x82000000ull, 1, nop);
    boom::frontend_module(state, pipe);
    check.expect(!state.frontend.packet_accept &&
                 state.frontend.fetch_buffer.count == 0,
                 "ftq_full_no_orphan_fb");
    check.expect(state.frontend.pending_packet.valid, "ftq_full_holds_packet");

    state.ftq_retire_pending.valid = true;
    state.ftq_retire_pending.ftq_idx = first_idx;
    state.ftq_retire_pending.generation = first_generation;
    state.ftq_retire_pending.lane = 0;
    boom::frontend_module(state, pipe);
    check.expect(state.frontend.packet_accept, "full_reclaim_allocate_turnover");
    check.expect(state.ftq_last_output.reclaimed && state.ftq_last_output.alloc_accepted,
                 "turnover_both_events");
    check.expect(state.ftq_last_output.count == FTQ_DEPTH,
                 "turnover_count_stable");

    BoomCoreState fb_full;
    PipeSignals fb_pipe;
    initialize(fb_full);
    fb_full.frontend.fetch_buffer.count = FETCH_BUFFER_DEPTH;
    fb_full.frontend.fetch_buffer.head = 0;
    fb_full.frontend.fetch_buffer.tail = 0;
    fb_full.rename.dispatch_packets[0].valid = true;
    prepare_packet(fb_full, 0x83000000ull, 1, nop);
    boom::frontend_module(fb_full, fb_pipe);
    check.expect(!fb_full.frontend.packet_accept &&
                 fb_full.ftq_last_output.count == 0,
                 "fb_full_no_orphan_ftq");
}

void test_commit_squash_exception_reset(Checker& check) {
    const uint32_t nop = 0x00000013u;
    {
        BoomCoreState state;
        PipeSignals pipe;
        initialize(state);
        prepare_packet(state, 0x84000000ull, 1, nop);
        boom::frontend_module(state, pipe);
        MicroOp uop = state.frontend.fetch_buffer.entries[0].ftq_valid ?
            MicroOp() : MicroOp();
        const boom::FetchInstruction ref = state.frontend.fetch_buffer.entries[0];
        uop.debug_pc = ref.pc;
        uop.inst = ref.instruction;
        uop.ftq_valid = true;
        uop.ftq_idx = ref.ftq_idx;
        uop.ftq_generation = ref.ftq_generation;
        uop.ftq_lane = ref.ftq_lane;
        uop.ftq_halfword_offset = ref.ftq_halfword_offset;
        uop.queue.rob_idx = 0;
        uop.queue.rob_allocation_id = 11;
        state.rob.entries[0].valid = true;
        state.rob.entries[0].busy = false;
        state.rob.entries[0].uop = uop;
        state.rob.head = 0;
        state.rob.tail = 1;
        boom::rob_commit_module(state, pipe);
        boom::frontend_module(state, pipe);
        check.expect(state.ftq_last_output.retire_accepted, "normal_commit_retire");
        check.expect(state.ftq_last_output.reclaimed && state.ftq_last_output.empty,
                     "normal_commit_reclaim");
        state.ftq_retire_pending.valid = true;
        state.ftq_retire_pending.ftq_idx = ref.ftq_idx;
        state.ftq_retire_pending.generation = ref.ftq_generation;
        state.ftq_retire_pending.lane = ref.ftq_lane;
        boom::frontend_module(state, pipe);
        check.expect(state.ftq_last_output.retire_rejected,
                     "duplicate_commit_rejected");
    }
    {
        BoomCoreState state;
        PipeSignals pipe;
        initialize(state);
        prepare_packet(state, 0x85000000ull, 3, 0x00000463u, nop);
        boom::frontend_module(state, pipe);
        const boom::FetchInstruction owner = state.frontend.fetch_buffer.entries[0];
        state.frontend.fetch_buffer = boom::FetchBufferState();
        prepare_packet(state, 0x85000008ull, 1, nop);
        boom::frontend_module(state, pipe);
        MicroOp branch;
        branch.ftq_valid = true;
        branch.ftq_idx = owner.ftq_idx;
        branch.ftq_generation = owner.ftq_generation;
        branch.ftq_lane = 0;
        branch.queue.rob_idx = 0;
        branch.queue.rob_allocation_id = 22;
        branch.branch.is_br = true;
        branch.branch.br_tag = 0;
        state.rob.entries[0].valid = true;
        state.rob.entries[0].uop = branch;
        state.rob.head = 0;
        state.rob.tail = 1;
        boom::branch_complete_event(state, branch, true, 0x85000100ull);
        boom::frontend_module(state, pipe);
        check.expect(state.ftq_last_output.redirect_accepted, "branch_redirect_accepted");
        check.expect(state.ftq_last_output.count == 1, "branch_kills_younger_suffix");
        state.brupdate = BranchUpdate();
        request_read(state, owner.ftq_idx, owner.ftq_generation, pipe);
        check.expect(state.ftq_last_output.read_hit &&
                     state.ftq_last_output.read_entry.live_lane_mask == 1,
                     "same_packet_lane1_killed_owner_retained");
    }
    {
        BoomCoreState state;
        PipeSignals pipe;
        initialize(state);
        prepare_packet(state, 0x86000000ull, 1, 0xffffffffu);
        boom::frontend_module(state, pipe);
        const boom::FetchInstruction ref = state.frontend.fetch_buffer.entries[0];
        RobEntry owner;
        owner.valid = true;
        owner.exception = true;
        owner.uop.debug_pc = ref.pc;
        owner.uop.inst = ref.instruction;
        owner.uop.exception = true;
        owner.uop.exc_cause = 2;
        owner.uop.ftq_valid = true;
        owner.uop.ftq_idx = ref.ftq_idx;
        owner.uop.ftq_generation = ref.ftq_generation;
        owner.uop.ftq_lane = ref.ftq_lane;
        boom::exception_recovery_apply(state, owner);
        check.expect(state.exception_commit.valid, "exception_captured_before_cleanup");
        boom::frontend_module(state, pipe);
        check.expect(state.ftq_last_output.redirect_accepted,
                     "exception_owner_redirect");
        state.global_flush = false;
        boom::frontend_module(state, pipe);
        check.expect(state.ftq_last_output.retire_accepted &&
                     state.ftq_last_output.empty, "exception_owner_post_trap_retire");
    }
    {
        BoomCoreState state;
        PipeSignals pipe;
        initialize(state);
        prepare_packet(state, 0x87000000ull, 1, nop);
        boom::frontend_module(state, pipe);
        const uint8_t idx = state.frontend.ftq_alloc_idx;
        const uint32_t generation = state.frontend.ftq_alloc_generation;
        state.global_flush = true;
        boom::frontend_module(state, pipe);
        check.expect(state.ftq_last_output.empty, "global_flush_ftq_cleanup");
        request_read(state, idx, generation, pipe);
        check.expect(!state.ftq_last_output.read_hit, "pre_flush_reference_stale");
        prepare_packet(state, 0x87000100ull, 1, nop);
        boom::frontend_module(state, pipe);
        check.expect(state.frontend.ftq_alloc_generation != generation,
                     "generation_changes_after_flush_reset");
    }
}

template <unsigned Depth>
void bounded_sequences(Checker& check, unsigned length) {
    uint64_t sequences = 1;
    for (unsigned i = 0; i < length; ++i) sequences *= 5;
    for (uint64_t code = 0; code < sequences; ++code) {
        boom::FtqFoundation<Depth> dut;
        uint64_t events = code;
        uint8_t last_idx = 0;
        uint32_t last_generation = 0;
        for (unsigned step = 0; step < length; ++step) {
            const unsigned event = static_cast<unsigned>(events % 5);
            events /= 5;
            boom::FtqStepInput input;
            if (event == 0) {
                input.alloc_valid = true;
                input.allocation.packet_base_pc = 0x90000000ull + step * 4;
                input.allocation.packet_valid_mask = (step & 1u) ? 3 : 1;
            } else if (event == 1 && last_generation != 0) {
                input.retire.valid = true;
                input.retire.ftq_idx = last_idx;
                input.retire.generation = last_generation;
                input.retire.lane = 0;
            } else if (event == 2 && last_generation != 0) {
                input.squash.valid = true;
                input.squash.ftq_idx = last_idx;
                input.squash.generation = last_generation;
                input.squash.lane = 1;
            } else if (event == 3) {
                input.reset = true;
            }
            const boom::FtqStepOutput output = dut.step(input);
            if (output.alloc_accepted) {
                last_idx = output.alloc_ftq_idx;
                last_generation = output.alloc_generation;
            }
            check.expect(output.count <= Depth, "bounded_count");
            check.expect(output.head < Depth && output.tail < Depth, "bounded_pointer");
            check.expect(output.full == (output.count == Depth), "bounded_full");
            check.expect(output.empty == (output.count == 0), "bounded_empty");
            check.expect(!(output.alloc_accepted && !input.alloc_valid),
                         "bounded_no_phantom_allocate");
        }
    }
    std::printf("PF3_EXHAUSTIVE depth=%u length=%u sequences=%llu errors=%llu\n",
                Depth, length, static_cast<unsigned long long>(sequences),
                static_cast<unsigned long long>(check.failures));
}

}  // namespace

int main() {
    Checker check;
    test_admission_and_metadata(check);
    test_backpressure_and_turnover(check);
    test_commit_squash_exception_reset(check);
    bounded_sequences<2>(check, 6);
    bounded_sequences<4>(check, 4);
    std::printf("PF3_DIRECTED_%s checks=%llu failures=%llu\n",
                check.failures == 0 ? "PASS" : "FAIL",
                static_cast<unsigned long long>(check.checks),
                static_cast<unsigned long long>(check.failures));
    return check.failures == 0 ? 0 : 1;
}
