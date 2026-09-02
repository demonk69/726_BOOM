#include "ftq.hpp"
#include "predictor.hpp"
#include "predecode.hpp"

#include <cstdint>
#include <cstdio>

namespace {

const uint32_t kGeneration = 37;
const uint32_t kNop0 = 0x00100093u;
const uint32_t kNop1 = 0x00200113u;

struct Checker {
    uint64_t checks;
    uint64_t failures;

    Checker() : checks(0), failures(0) {}

    void expect(bool condition, const char* label) {
        ++checks;
        if (!condition) {
            if (failures < 24) std::printf("FAIL,%s\n", label);
            ++failures;
        }
    }
};

struct PacketSpec {
    uint8_t mask;
    uint32_t lane0_instruction;
    uint32_t lane1_instruction;
    bool has_cfi;
    uint8_t cfi_lane;
    uint8_t cfi_type;
    bool expected_taken;
};

struct RetainedEntry {
    uint8_t index;
    uint32_t generation;
    boom::FtqEntry expected;
};

uint32_t encode_branch(int32_t immediate, uint32_t funct3) {
    const uint32_t bits = static_cast<uint32_t>(immediate) & 0x1fffu;
    return (((bits >> 12) & 1u) << 31) |
           (((bits >> 5) & 0x3fu) << 25) | (3u << 20) | (2u << 15) |
           ((funct3 & 7u) << 12) | (((bits >> 1) & 0xfu) << 8) |
           (((bits >> 11) & 1u) << 7) | 0x63u;
}

uint32_t encode_jal(int32_t immediate) {
    const uint32_t bits = static_cast<uint32_t>(immediate) & 0x1fffffu;
    return (((bits >> 20) & 1u) << 31) |
           (((bits >> 1) & 0x3ffu) << 21) |
           (((bits >> 11) & 1u) << 20) |
           (((bits >> 12) & 0xffu) << 12) | (1u << 7) | 0x6fu;
}

uint32_t encode_jalr(int32_t immediate) {
    return ((static_cast<uint32_t>(immediate) & 0xfffu) << 20) |
           (1u << 15) | (5u << 7) | 0x67u;
}

void train(boom::PredictorFoundation<256>& predictor, uint64_t pc,
           bool taken) {
    boom::PredictorStepInput input;
    input.active_generation = kGeneration;
    input.update.valid = true;
    input.update.commit_qualified = true;
    input.update.cfi_type = boom::CFI_CONDITIONAL_BRANCH;
    input.update.pc = pc;
    input.update.metadata_token =
        static_cast<uint16_t>((pc >> 1) & 0xffu);
    input.update.taken = taken;
    input.update.generation = kGeneration;
    predictor.step(input);
}

boom::PredictorResponse predict(boom::PredictorFoundation<256>& predictor,
                                const boom::PredictorRequest& request,
                                Checker& checker) {
    boom::PredictorStepInput input;
    input.active_generation = kGeneration;
    input.req_valid = true;
    input.request = request;
    boom::PredictorStepOutput output = predictor.step(input);
    checker.expect(output.req_ready && !output.resp_valid,
                   "predictor_request_accept");

    input.req_valid = false;
    output = predictor.step(input);
    checker.expect(!output.req_ready && output.resp_valid,
                   "predictor_one_step_response");
    const boom::PredictorResponse response = output.response;

    input.resp_ready = true;
    output = predictor.step(input);
    checker.expect(output.resp_valid && !output.req_ready,
                   "predictor_response_consume");
    return response;
}

PacketSpec make_spec(unsigned scenario, unsigned serial) {
    PacketSpec spec = {3u, kNop0, kNop1, true, 0u,
                       boom::CFI_CONDITIONAL_BRANCH, false};
    const int32_t offset = static_cast<int32_t>((serial & 31u) * 2u + 4u);
    switch (scenario % 9u) {
    case 0:
        spec.mask = 1u;
        spec.lane0_instruction = encode_branch(offset, 0u);
        spec.expected_taken = true;
        break;
    case 1:
        spec.lane0_instruction = encode_branch(-offset, 1u);
        break;
    case 2:
        spec.lane1_instruction = encode_branch(offset, 4u);
        spec.cfi_lane = 1u;
        spec.expected_taken = true;
        break;
    case 3:
        spec.mask = 1u;
        spec.lane0_instruction = encode_jal(offset);
        spec.cfi_type = boom::CFI_JAL;
        spec.expected_taken = true;
        break;
    case 4:
        spec.lane1_instruction = encode_jal(-offset);
        spec.cfi_lane = 1u;
        spec.cfi_type = boom::CFI_JAL;
        spec.expected_taken = true;
        break;
    case 5:
        spec.lane0_instruction = encode_jalr(offset);
        spec.cfi_type = boom::CFI_JALR;
        break;
    case 6:
        spec.lane1_instruction = encode_jalr(-offset);
        spec.cfi_lane = 1u;
        spec.cfi_type = boom::CFI_JALR;
        break;
    case 7:
        spec.mask = 1u;
        spec.has_cfi = false;
        spec.cfi_type = boom::CFI_NONE;
        break;
    default:
        spec.has_cfi = false;
        spec.cfi_type = boom::CFI_NONE;
        break;
    }
    return spec;
}

RetainedEntry compose_and_allocate(
        boom::PredictorFoundation<256>& predictor,
        boom::FtqFoundation<32>& ftq, const PacketSpec& spec,
        uint64_t base_pc, uint64_t request_token, Checker& checker) {
    const uint64_t lane0_pc = base_pc;
    const uint64_t lane1_pc = base_pc + 4u;
    const boom::CfiPacketPredecodeResult packet = boom::predecode_cfi_packet(
        spec.mask, lane0_pc, spec.lane0_instruction, false,
        lane1_pc, spec.lane1_instruction, false);
    checker.expect(packet.packet_has_cfi == spec.has_cfi,
                   "packet_cfi_presence");
    checker.expect(!spec.has_cfi || packet.selected_cfi_lane == spec.cfi_lane,
                   "packet_cfi_lane");
    checker.expect(!spec.has_cfi ||
                       packet.selected_cfi_result.cfi_type == spec.cfi_type,
                   "packet_cfi_type");

    const uint64_t selected_pc = spec.cfi_lane == 0u ? lane0_pc : lane1_pc;
    if (spec.cfi_type == boom::CFI_CONDITIONAL_BRANCH) {
        // Two updates force either prediction from every possible counter state.
        train(predictor, selected_pc, spec.expected_taken);
        train(predictor, selected_pc, spec.expected_taken);
    }

    boom::PredictorRequest request;
    request.pc = spec.has_cfi ? selected_pc : lane0_pc;
    request.cfi_lane = spec.has_cfi ? spec.cfi_lane : 0u;
    request.cfi_type = spec.has_cfi ? packet.selected_cfi_result.cfi_type :
        static_cast<uint8_t>(boom::CFI_NONE);
    request.static_target_valid = spec.has_cfi &&
        packet.selected_cfi_result.static_target_valid;
    request.static_target = packet.selected_cfi_result.static_target;
    request.generation = kGeneration;
    request.request_token = request_token;
    const boom::PredictorResponse response =
        predict(predictor, request, checker);

    const uint16_t exact_bim_index =
        static_cast<uint16_t>((request.pc >> 1) & 0xffu);
    checker.expect(response.metadata_token == exact_bim_index,
                   "predictor_exact_bim_index");
    checker.expect(response.cfi_lane == request.cfi_lane &&
                       response.cfi_type == request.cfi_type,
                   "predictor_cfi_identity");
    checker.expect(response.generation == kGeneration &&
                       response.request_token == request_token,
                   "predictor_request_identity");
    checker.expect(response.taken == spec.expected_taken,
                   "predictor_taken_result");

    if (spec.cfi_type == boom::CFI_JALR ||
        spec.cfi_type == boom::CFI_NONE) {
        checker.expect(!response.prediction_valid && !response.target_valid &&
                           response.target == 0u,
                       "unpredicted_jalr_or_non_cfi");
    } else {
        checker.expect(response.prediction_valid,
                       "direct_cfi_prediction_valid");
        checker.expect(response.target_valid == response.taken,
                       "direct_cfi_target_validity");
        checker.expect(!response.target_valid ||
                           response.target == request.static_target,
                       "direct_cfi_exact_target");
    }

    const uint8_t allocation_mask = response.taken ?
        boom::mask_younger_packet_lanes(spec.mask, packet) : spec.mask;
    boom::FtqStepInput ftq_input;
    ftq_input.alloc_valid = true;
    ftq_input.allocation.packet_base_pc = base_pc;
    ftq_input.allocation.packet_valid_mask = allocation_mask;
    ftq_input.allocation.prediction_valid = response.prediction_valid;
    ftq_input.allocation.predicted_taken = response.taken;
    ftq_input.allocation.target_valid = response.target_valid;
    ftq_input.allocation.predicted_target = response.target;
    ftq_input.allocation.cfi_lane = response.cfi_lane;
    ftq_input.allocation.cfi_type = response.cfi_type;
    ftq_input.allocation.predictor_metadata_index =
        static_cast<uint8_t>(response.metadata_token);
    ftq_input.allocation.predictor_generation = response.generation;
    const boom::FtqStepOutput allocation = ftq.step(ftq_input);
    checker.expect(allocation.alloc_ready && allocation.alloc_accepted &&
                       !allocation.alloc_invalid_mask,
                   "ftq_allocation_accept");

    RetainedEntry retained;
    retained.index = allocation.alloc_ftq_idx;
    retained.generation = allocation.alloc_generation;
    retained.expected.valid = true;
    retained.expected.packet_base_pc = base_pc;
    retained.expected.packet_valid_mask = allocation_mask;
    retained.expected.live_lane_mask = allocation_mask;
    retained.expected.prediction_valid = response.prediction_valid;
    retained.expected.predicted_taken =
        response.prediction_valid && response.taken;
    retained.expected.target_valid =
        response.prediction_valid && response.target_valid;
    retained.expected.predicted_target = retained.expected.target_valid ?
        response.target : 0u;
    retained.expected.cfi_lane = static_cast<uint8_t>(response.cfi_lane & 1u);
    retained.expected.cfi_type = static_cast<uint8_t>(response.cfi_type & 3u);
    retained.expected.predictor_metadata_index =
        static_cast<uint8_t>(exact_bim_index);
    retained.expected.predictor_generation = response.generation;
    retained.expected.generation = allocation.alloc_generation;
    return retained;
}

void check_retained(boom::FtqFoundation<32>& ftq,
                    const RetainedEntry& retained, Checker& checker) {
    boom::FtqStepInput input;
    input.read_valid = true;
    input.read_ftq_idx = retained.index;
    input.read_generation = retained.generation;
    const boom::FtqStepOutput output = ftq.step(input);
    const boom::FtqEntry& got = output.read_entry;
    const boom::FtqEntry& expected = retained.expected;
    checker.expect(output.read_hit, "long_lived_ftq_read_hit");
    checker.expect(got.valid == expected.valid, "ftq_exact_valid");
    checker.expect(got.packet_base_pc == expected.packet_base_pc,
                   "ftq_exact_packet_pc");
    checker.expect(got.packet_valid_mask == expected.packet_valid_mask,
                   "ftq_exact_packet_mask");
    checker.expect(got.live_lane_mask == expected.live_lane_mask,
                   "ftq_exact_live_mask");
    checker.expect(got.prediction_valid == expected.prediction_valid,
                   "ftq_exact_prediction_valid");
    checker.expect(got.predicted_taken == expected.predicted_taken,
                   "ftq_exact_predicted_taken");
    checker.expect(got.target_valid == expected.target_valid,
                   "ftq_exact_target_valid");
    checker.expect(got.predicted_target == expected.predicted_target,
                   "ftq_exact_target");
    checker.expect(got.cfi_lane == expected.cfi_lane, "ftq_exact_cfi_lane");
    checker.expect(got.cfi_type == expected.cfi_type, "ftq_exact_cfi_type");
    checker.expect(got.predictor_metadata_index ==
                       expected.predictor_metadata_index,
                   "ftq_exact_bim_index_readback");
    checker.expect(got.predictor_generation == expected.predictor_generation,
                   "ftq_exact_predictor_generation");
    checker.expect(got.generation == expected.generation,
                   "ftq_exact_generation");
}

void retire_entry(boom::FtqFoundation<32>& ftq,
                  const RetainedEntry& retained, Checker& checker) {
    for (uint8_t lane = 0; lane < 2u; ++lane) {
        if ((retained.expected.packet_valid_mask &
             static_cast<uint8_t>(1u << lane)) == 0u) {
            continue;
        }
        boom::FtqStepInput input;
        input.retire.valid = true;
        input.retire.ftq_idx = retained.index;
        input.retire.lane = lane;
        input.retire.generation = retained.generation;
        const boom::FtqStepOutput output = ftq.step(input);
        checker.expect(output.retire_accepted && !output.retire_rejected,
                       "ftq_ordered_retire");
    }
}

}  // namespace

int main() {
    Checker checker;
    boom::PredictorFoundation<256> predictor;
    boom::FtqFoundation<32> ftq;

    boom::PredictorStepInput predictor_reset;
    predictor_reset.reset = true;
    predictor_reset.active_generation = kGeneration;
    checker.expect(!predictor.step(predictor_reset).req_ready,
                   "predictor_reset_idle");
    boom::FtqStepInput ftq_reset;
    ftq_reset.reset = true;
    const boom::FtqStepOutput reset_output = ftq.step(ftq_reset);
    checker.expect(reset_output.empty && reset_output.count == 0u,
                   "ftq_reset_empty");

    uint64_t token = 0;
    for (unsigned batch = 0; batch < 12u; ++batch) {
        RetainedEntry retained[32];
        for (unsigned slot = 0; slot < 32u; ++slot) {
            const unsigned serial = batch * 32u + slot;
            const uint64_t base_pc = 0x80000000ull +
                static_cast<uint64_t>(serial) * 8u;
            retained[slot] = compose_and_allocate(
                predictor, ftq, make_spec(serial, serial), base_pc,
                token++, checker);
        }

        boom::FtqStepInput status_input;
        const boom::FtqStepOutput full = ftq.step(status_input);
        checker.expect(full.full && full.count == 32u,
                       "ftq_batch_remains_full");

        // Exercise unrelated predictor state while every FTQ payload is retained.
        for (unsigned perturb = 0; perturb < 64u; ++perturb) {
            const uint64_t pc = 0x90000000ull +
                static_cast<uint64_t>(batch * 64u + perturb) * 2u;
            train(predictor, pc, (perturb & 1u) != 0u);
        }

        for (unsigned slot = 0; slot < 32u; ++slot)
            check_retained(ftq, retained[slot], checker);
        for (unsigned slot = 0; slot < 32u; ++slot)
            retire_entry(ftq, retained[slot], checker);

        const boom::FtqStepOutput empty = ftq.step(status_input);
        checker.expect(empty.empty && empty.count == 0u,
                       "ftq_batch_fully_reclaimed");
    }

    std::printf("PREDICTOR_FTQ_COMPOSITION,checks=%llu,failures=%llu\n",
                static_cast<unsigned long long>(checker.checks),
                static_cast<unsigned long long>(checker.failures));
    return checker.failures == 0u && checker.checks >= 5000u ? 0 : 1;
}
