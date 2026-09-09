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

template <std::size_t Depth, bool FullPayloadReset>
FtqFoundation<Depth, FullPayloadReset>::FtqFoundation()
    : head_(0), tail_(0), count_(0), next_generation_(1) {
    for (std::size_t i = 0; i < Depth; ++i) entries_[i].valid = false;
}

template <std::size_t Depth, bool FullPayloadReset>
uint8_t FtqFoundation<Depth, FullPayloadReset>::advance(uint8_t index) {
    return static_cast<uint8_t>((index + 1u) & (Depth - 1u));
}

template <std::size_t Depth, bool FullPayloadReset>
bool FtqFoundation<Depth, FullPayloadReset>::entry_is_active(
        uint8_t index, uint32_t generation) const {
    return index < Depth && entries_[index].valid &&
           entries_[index].generation == generation;
}

template <std::size_t Depth, bool FullPayloadReset>
bool FtqFoundation<Depth, FullPayloadReset>::apply_lane_event(
        const FtqLaneEvent& event) {
    if (!event.valid || event.lane > 1 ||
        !entry_is_active(event.ftq_idx, event.generation)) return false;
    const uint8_t lane_bit = static_cast<uint8_t>(1u << event.lane);
    FtqEntry& entry = entries_[event.ftq_idx];
    if ((entry.packet_valid_mask & lane_bit) == 0 ||
        (entry.live_lane_mask & lane_bit) == 0) return false;
    entry.live_lane_mask = static_cast<uint8_t>(entry.live_lane_mask & ~lane_bit);
    return true;
}

template <std::size_t Depth, bool FullPayloadReset>
void FtqFoundation<Depth, FullPayloadReset>::reset_controls() {
    for (std::size_t i = 0; i < Depth; ++i) {
        entries_[i].valid = false;
        entries_[i].live_lane_mask = 0;
        if (FullPayloadReset) entries_[i] = FtqEntry();
    }
    head_ = 0;
    tail_ = 0;
    count_ = 0;
    ++next_generation_;
    if (next_generation_ == 0) next_generation_ = 1;
}

template <std::size_t Depth, bool FullPayloadReset>
FtqStepOutput FtqFoundation<Depth, FullPayloadReset>::step(
        const FtqStepInput& input) {
#if defined(BOOM_FTQ_STORAGE_LUTRAM)
#pragma HLS bind_storage variable=entries_ type=RAM_2P impl=LUTRAM
#elif defined(BOOM_FTQ_STORAGE_BRAM)
#pragma HLS bind_storage variable=entries_ type=RAM_2P impl=BRAM
#endif
    FtqStepOutput output;
    if (input.reset) {
        reset_controls();
        output.head = head_;
        output.tail = tail_;
        output.count = count_;
        output.empty = true;
        return output;
    }
    if (input.redirect.valid) {
        if (entry_is_active(input.redirect.owner_ftq_idx,
                            input.redirect.owner_generation)) {
            const uint8_t owner_distance = static_cast<uint8_t>(
                (input.redirect.owner_ftq_idx + Depth - head_) & (Depth - 1u));
            if (owner_distance < count_) {
                for (std::size_t offset = 0; offset < Depth; ++offset) {
                    if (offset > owner_distance && offset < count_) {
                        const uint8_t index = static_cast<uint8_t>(
                            (head_ + offset) & (Depth - 1u));
                        entries_[index].valid = false;
                        entries_[index].live_lane_mask = 0;
                    }
                }
                FtqEntry& owner = entries_[input.redirect.owner_ftq_idx];
                owner.live_lane_mask = static_cast<uint8_t>(
                    owner.live_lane_mask & input.redirect.surviving_lane_mask &
                    owner.packet_valid_mask);
                tail_ = advance(input.redirect.owner_ftq_idx);
                count_ = static_cast<uint8_t>(owner_distance + 1u);
                output.redirect_accepted = true;
            } else output.redirect_rejected = true;
        } else output.redirect_rejected = true;
        output.retire_rejected = input.retire.valid;
        output.squash_rejected = input.squash.valid;
    } else {
        if (input.retire.valid) {
            output.retire_accepted = apply_lane_event(input.retire);
            output.retire_rejected = !output.retire_accepted;
        }
        if (input.squash.valid) {
            output.squash_accepted = apply_lane_event(input.squash);
            output.squash_rejected = !output.squash_accepted;
        }
    }
    if (count_ != 0 && entries_[head_].valid &&
        entries_[head_].live_lane_mask == 0) {
        output.reclaimed = true;
        output.reclaimed_ftq_idx = head_;
        entries_[head_].valid = false;
        head_ = advance(head_);
        --count_;
    }
    output.alloc_ready = !input.redirect.valid && count_ < Depth;
    const uint8_t mask = static_cast<uint8_t>(input.allocation.packet_valid_mask & 3u);
    output.alloc_invalid_mask = input.alloc_valid &&
        (input.allocation.packet_valid_mask != mask || mask == 2u);
    if (input.alloc_valid && output.alloc_ready && !output.alloc_invalid_mask && mask != 0) {
        FtqEntry entry;
        entry.valid = true;
        entry.packet_base_pc = input.allocation.packet_base_pc;
        entry.packet_valid_mask = mask;
        entry.live_lane_mask = mask;
        entry.prediction_valid = input.allocation.prediction_valid;
        entry.predicted_taken = input.allocation.prediction_valid && input.allocation.predicted_taken;
        entry.target_valid = input.allocation.prediction_valid && input.allocation.target_valid;
        entry.predicted_target = entry.target_valid ? input.allocation.predicted_target : 0;
        entry.cfi_lane = static_cast<uint8_t>(input.allocation.cfi_lane & 1u);
        entry.cfi_type = static_cast<uint8_t>(input.allocation.cfi_type & 3u);
        entry.predictor_metadata_index = input.allocation.predictor_metadata_index;
        entry.predictor_generation = input.allocation.predictor_generation;
        entry.generation = next_generation_;
        ++next_generation_;
        if (next_generation_ == 0) next_generation_ = 1;
        entries_[tail_] = entry;
        output.alloc_accepted = true;
        output.alloc_ftq_idx = tail_;
        output.alloc_generation = entry.generation;
        tail_ = advance(tail_);
        ++count_;
    }
    if (input.read_valid && entry_is_active(input.read_ftq_idx, input.read_generation)) {
        output.read_hit = true;
        output.read_entry = entries_[input.read_ftq_idx];
    }
    output.head = head_;
    output.tail = tail_;
    output.count = count_;
    output.empty = count_ == 0;
    output.full = count_ == Depth;
    return output;
}

}  // namespace boom

#endif
