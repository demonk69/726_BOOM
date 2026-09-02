#include "predecode.hpp"

namespace boom {
namespace {

static int64_t predecode_sign_extend(uint32_t value, unsigned width) {
    const uint64_t sign = uint64_t(1) << (width - 1);
    return static_cast<int64_t>((static_cast<uint64_t>(value) ^ sign) - sign);
}

static int64_t predecode_branch_immediate(uint32_t instruction) {
    const uint32_t immediate =
        ((instruction >> 31) & 0x1u) << 12 |
        ((instruction >> 7) & 0x1u) << 11 |
        ((instruction >> 25) & 0x3fu) << 5 |
        ((instruction >> 8) & 0xfu) << 1;
    return predecode_sign_extend(immediate, 13);
}

static int64_t predecode_jal_immediate(uint32_t instruction) {
    const uint32_t immediate =
        ((instruction >> 31) & 0x1u) << 20 |
        ((instruction >> 12) & 0xffu) << 12 |
        ((instruction >> 20) & 0x1u) << 11 |
        ((instruction >> 21) & 0x3ffu) << 1;
    return predecode_sign_extend(immediate, 21);
}

static bool predecode_is_link_register(uint32_t reg) {
    return reg == 1u || reg == 5u;
}

}  // namespace

CfiPredecodeResult predecode_cfi(uint64_t pc, uint32_t instruction,
                                 bool is_rvc) {
    CfiPredecodeResult result;
    result.valid = true;
    result.instruction_length = is_rvc ? 2 : 4;

    const uint32_t opcode = instruction & 0x7fu;
    const uint32_t funct3 = (instruction >> 12) & 0x7u;
    const uint32_t rd = (instruction >> 7) & 0x1fu;
    const uint32_t rs1 = (instruction >> 15) & 0x1fu;

    const bool legal_branch_funct3 = funct3 == 0u || funct3 == 1u ||
        funct3 == 4u || funct3 == 5u || funct3 == 6u || funct3 == 7u;
    if (opcode == 0x63u && legal_branch_funct3) {
        result.is_cfi = true;
        result.cfi_type = CFI_CONDITIONAL_BRANCH;
        result.is_conditional = true;
        result.static_target_valid = true;
        result.static_target = pc +
            static_cast<uint64_t>(predecode_branch_immediate(instruction));
    } else if (opcode == 0x6fu) {
        result.is_cfi = true;
        result.cfi_type = CFI_JAL;
        result.is_jal = true;
        result.is_call = predecode_is_link_register(rd);
        result.static_target_valid = true;
        result.static_target = pc +
            static_cast<uint64_t>(predecode_jal_immediate(instruction));
    } else if (opcode == 0x67u && funct3 == 0u) {
        result.is_cfi = true;
        result.cfi_type = CFI_JALR;
        result.is_jalr = true;
        result.is_call = predecode_is_link_register(rd);
        const uint32_t immediate = instruction >> 20;
        result.is_return = rd == 0u && predecode_is_link_register(rs1) &&
            immediate == 0u;
    }

    return result;
}

CfiPacketPredecodeResult predecode_cfi_packet(
    uint8_t valid_mask,
    uint64_t lane0_pc, uint32_t lane0_instruction, bool lane0_is_rvc,
    uint64_t lane1_pc, uint32_t lane1_instruction, bool lane1_is_rvc) {
    CfiPacketPredecodeResult packet;
    const CfiPredecodeResult lane0 =
        predecode_cfi(lane0_pc, lane0_instruction, lane0_is_rvc);
    const CfiPredecodeResult lane1 =
        predecode_cfi(lane1_pc, lane1_instruction, lane1_is_rvc);

    if ((valid_mask & 0x1u) != 0 && lane0.is_cfi) {
        packet.packet_has_cfi = true;
        packet.selected_cfi_lane = 0;
        packet.selected_cfi_result = lane0;
        packet.younger_lane_mask = valid_mask & 0x2u;
    } else if ((valid_mask & 0x2u) != 0 && lane1.is_cfi) {
        packet.packet_has_cfi = true;
        packet.selected_cfi_lane = 1;
        packet.selected_cfi_result = lane1;
    }
    return packet;
}

uint8_t mask_younger_packet_lanes(uint8_t valid_mask,
                                  const CfiPacketPredecodeResult& packet) {
    return packet.packet_has_cfi ?
        static_cast<uint8_t>(valid_mask & ~packet.younger_lane_mask) : valid_mask;
}

}  // namespace boom
