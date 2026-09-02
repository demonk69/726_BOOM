#include "ftq.hpp"

namespace boom {

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
        !entry_is_active(event.ftq_idx, event.generation)) {
        return false;
    }
    const uint8_t lane_bit = static_cast<uint8_t>(1u << event.lane);
    FtqEntry& entry = entries_[event.ftq_idx];
    if ((entry.packet_valid_mask & lane_bit) == 0 ||
        (entry.live_lane_mask & lane_bit) == 0) {
        return false;
    }
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
            } else {
                output.redirect_rejected = true;
            }
        } else {
            output.redirect_rejected = true;
        }
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
    if (input.alloc_valid && output.alloc_ready && !output.alloc_invalid_mask &&
        mask != 0) {
        FtqEntry entry;
        entry.valid = true;
        entry.packet_base_pc = input.allocation.packet_base_pc;
        entry.packet_valid_mask = mask;
        entry.live_lane_mask = mask;
        entry.prediction_valid = input.allocation.prediction_valid;
        entry.predicted_taken = input.allocation.prediction_valid &&
                                input.allocation.predicted_taken;
        entry.target_valid = input.allocation.prediction_valid &&
                             input.allocation.target_valid;
        entry.predicted_target = entry.target_valid ?
            input.allocation.predicted_target : 0;
        entry.cfi_lane = static_cast<uint8_t>(input.allocation.cfi_lane & 1u);
        entry.cfi_type = static_cast<uint8_t>(input.allocation.cfi_type & 3u);
        entry.predictor_metadata_index =
            input.allocation.predictor_metadata_index;
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

    if (input.read_valid &&
        entry_is_active(input.read_ftq_idx, input.read_generation)) {
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

template class FtqFoundation<2>;
template class FtqFoundation<4>;
template class FtqFoundation<8>;
template class FtqFoundation<16>;
template class FtqFoundation<32>;
template class FtqFoundation<64>;
template class FtqFoundation<32, true>;

}  // namespace boom
