#ifndef BOOM_FTQ_HPP
#define BOOM_FTQ_HPP

#include <cstddef>
#include <cstdint>

#include "predecode.hpp"

namespace boom {

struct FtqEntry {
    bool valid;
    uint64_t packet_base_pc;
    uint8_t packet_valid_mask;
    uint8_t live_lane_mask;
    bool prediction_valid;
    bool predicted_taken;
    bool target_valid;
    uint64_t predicted_target;
    uint8_t cfi_lane;
    uint8_t cfi_type;
    uint8_t predictor_metadata_index;
    uint32_t predictor_generation;
    uint32_t generation;

    FtqEntry()
        : valid(false), packet_base_pc(0), packet_valid_mask(0),
          live_lane_mask(0), prediction_valid(false), predicted_taken(false),
          target_valid(false), predicted_target(0), cfi_lane(0),
          cfi_type(CFI_NONE), predictor_metadata_index(0),
          predictor_generation(0), generation(0) {}
};

struct FtqAllocation {
    uint64_t packet_base_pc;
    uint8_t packet_valid_mask;
    bool prediction_valid;
    bool predicted_taken;
    bool target_valid;
    uint64_t predicted_target;
    uint8_t cfi_lane;
    uint8_t cfi_type;
    uint8_t predictor_metadata_index;
    uint32_t predictor_generation;

    FtqAllocation()
        : packet_base_pc(0), packet_valid_mask(0), prediction_valid(false),
          predicted_taken(false), target_valid(false), predicted_target(0),
          cfi_lane(0), cfi_type(CFI_NONE), predictor_metadata_index(0),
          predictor_generation(0) {}
};

struct FtqLaneEvent {
    bool valid;
    uint8_t ftq_idx;
    uint8_t lane;
    uint32_t generation;

    FtqLaneEvent() : valid(false), ftq_idx(0), lane(0), generation(0) {}
};

struct FtqRedirect {
    bool valid;
    uint8_t owner_ftq_idx;
    uint32_t owner_generation;
    uint8_t surviving_lane_mask;

    FtqRedirect()
        : valid(false), owner_ftq_idx(0), owner_generation(0),
          surviving_lane_mask(0) {}
};

struct FtqStepInput {
    bool reset;
    bool alloc_valid;
    FtqAllocation allocation;
    FtqLaneEvent retire;
    FtqLaneEvent squash;
    FtqRedirect redirect;
    bool read_valid;
    uint8_t read_ftq_idx;
    uint32_t read_generation;

    FtqStepInput()
        : reset(false), alloc_valid(false), allocation(), retire(), squash(),
          redirect(), read_valid(false), read_ftq_idx(0), read_generation(0) {}
};

struct FtqStepOutput {
    bool alloc_ready;
    bool alloc_accepted;
    bool alloc_invalid_mask;
    uint8_t alloc_ftq_idx;
    uint32_t alloc_generation;
    bool retire_accepted;
    bool retire_rejected;
    bool squash_accepted;
    bool squash_rejected;
    bool redirect_accepted;
    bool redirect_rejected;
    bool reclaimed;
    uint8_t reclaimed_ftq_idx;
    bool read_hit;
    FtqEntry read_entry;
    bool empty;
    bool full;
    uint8_t head;
    uint8_t tail;
    uint8_t count;

    FtqStepOutput()
        : alloc_ready(false), alloc_accepted(false), alloc_invalid_mask(false),
          alloc_ftq_idx(0), alloc_generation(0), retire_accepted(false),
          retire_rejected(false), squash_accepted(false),
          squash_rejected(false), redirect_accepted(false),
          redirect_rejected(false), reclaimed(false), reclaimed_ftq_idx(0),
          read_hit(false), read_entry(), empty(true), full(false), head(0),
          tail(0), count(0) {}
};

template <std::size_t Depth, bool FullPayloadReset = false>
class FtqFoundation {
public:
    FtqFoundation();
    FtqStepOutput step(const FtqStepInput& input);

private:
    static_assert(Depth == 2 || Depth == 4 || Depth == 8 || Depth == 16 ||
                  Depth == 32 || Depth == 64,
                  "FtqFoundation depth must be 2, 4, 8, 16, 32, or 64");

    FtqEntry entries_[Depth];
    uint8_t head_;
    uint8_t tail_;
    uint8_t count_;
    uint32_t next_generation_;

    static uint8_t advance(uint8_t index);
    bool apply_lane_event(const FtqLaneEvent& event);
    bool entry_is_active(uint8_t index, uint32_t generation) const;
    void reset_controls();
};

extern template class FtqFoundation<2>;
extern template class FtqFoundation<4>;
extern template class FtqFoundation<8>;
extern template class FtqFoundation<16>;
extern template class FtqFoundation<32>;
extern template class FtqFoundation<64>;
extern template class FtqFoundation<32, true>;

}  // namespace boom

#endif
