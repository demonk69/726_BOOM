#ifndef BOOM_PREDECODE_HPP
#define BOOM_PREDECODE_HPP

#include <cstdint>

namespace boom {

enum CfiType : uint8_t {
    CFI_NONE = 0,
    CFI_CONDITIONAL_BRANCH = 1,
    CFI_JAL = 2,
    CFI_JALR = 3
};

struct CfiPredecodeResult {
    bool valid;
    bool is_cfi;
    uint8_t cfi_type;
    bool is_conditional;
    bool is_jal;
    bool is_jalr;
    bool is_call;
    bool is_return;
    bool static_target_valid;
    uint64_t static_target;
    uint8_t instruction_length;

    CfiPredecodeResult()
        : valid(false), is_cfi(false), cfi_type(CFI_NONE),
          is_conditional(false), is_jal(false), is_jalr(false),
          is_call(false), is_return(false), static_target_valid(false),
          static_target(0), instruction_length(0) {}
};

struct CfiPacketPredecodeResult {
    bool packet_has_cfi;
    uint8_t selected_cfi_lane;
    CfiPredecodeResult selected_cfi_result;
    uint8_t younger_lane_mask;

    CfiPacketPredecodeResult()
        : packet_has_cfi(false), selected_cfi_lane(0),
          selected_cfi_result(), younger_lane_mask(0) {}
};

CfiPredecodeResult predecode_cfi(uint64_t pc, uint32_t instruction,
                                 bool is_rvc);

CfiPacketPredecodeResult predecode_cfi_packet(
    uint8_t valid_mask,
    uint64_t lane0_pc, uint32_t lane0_instruction, bool lane0_is_rvc,
    uint64_t lane1_pc, uint32_t lane1_instruction, bool lane1_is_rvc);

uint8_t mask_younger_packet_lanes(uint8_t valid_mask,
                                  const CfiPacketPredecodeResult& packet);

}  // namespace boom

#endif
