#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <iostream>
#include <string>
#include <vector>

#include "ftq.hpp"

namespace {

constexpr uint32_t kSeeds = 256;
constexpr uint32_t kCyclesPerSeed = 16384;

uint32_t random32(uint32_t& state) {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

struct Handle {
    uint8_t index;
    uint32_t generation;
};

struct ReferenceNode {
    uint8_t index;
    boom::FtqEntry entry;
};

template <std::size_t Depth>
class ReferenceFtq {
public:
    ReferenceFtq() : head_(0), tail_(0), next_generation_(1) {}

    boom::FtqStepOutput step(const boom::FtqStepInput& input) {
        boom::FtqStepOutput output;
        if (input.reset) {
            queue_.clear();
            head_ = 0;
            tail_ = 0;
            increment_generation();
            output.head = head_;
            output.tail = tail_;
            output.count = 0;
            output.empty = true;
            return output;
        }

        if (input.redirect.valid) {
            typename std::deque<ReferenceNode>::iterator owner = find(
                input.redirect.owner_ftq_idx, input.redirect.owner_generation);
            if (owner != queue_.end()) {
                owner->entry.live_lane_mask = static_cast<uint8_t>(
                    owner->entry.live_lane_mask & input.redirect.surviving_lane_mask &
                    owner->entry.packet_valid_mask);
                queue_.erase(owner + 1, queue_.end());
                tail_ = advance(owner->index);
                output.redirect_accepted = true;
            } else {
                output.redirect_rejected = true;
            }
            output.retire_rejected = input.retire.valid;
            output.squash_rejected = input.squash.valid;
        } else {
            if (input.retire.valid) {
                output.retire_accepted = apply_event(input.retire);
                output.retire_rejected = !output.retire_accepted;
            }
            if (input.squash.valid) {
                output.squash_accepted = apply_event(input.squash);
                output.squash_rejected = !output.squash_accepted;
            }
        }

        if (!queue_.empty() && queue_.front().entry.live_lane_mask == 0) {
            output.reclaimed = true;
            output.reclaimed_ftq_idx = queue_.front().index;
            queue_.pop_front();
            head_ = advance(head_);
        }

        output.alloc_ready = !input.redirect.valid && queue_.size() < Depth;
        const uint8_t mask = static_cast<uint8_t>(input.allocation.packet_valid_mask & 3u);
        output.alloc_invalid_mask = input.alloc_valid &&
            (input.allocation.packet_valid_mask != mask || mask == 2u);
        if (input.alloc_valid && output.alloc_ready && !output.alloc_invalid_mask &&
            mask != 0) {
            ReferenceNode node;
            node.index = tail_;
            node.entry.valid = true;
            node.entry.packet_base_pc = input.allocation.packet_base_pc;
            node.entry.packet_valid_mask = mask;
            node.entry.live_lane_mask = mask;
            node.entry.prediction_valid = input.allocation.prediction_valid;
            node.entry.predicted_taken = input.allocation.prediction_valid &&
                                         input.allocation.predicted_taken;
            node.entry.target_valid = input.allocation.prediction_valid &&
                                      input.allocation.target_valid;
            node.entry.predicted_target = node.entry.target_valid
                ? input.allocation.predicted_target : 0;
            node.entry.cfi_lane = static_cast<uint8_t>(input.allocation.cfi_lane & 1u);
            node.entry.cfi_type = static_cast<uint8_t>(input.allocation.cfi_type & 3u);
            node.entry.predictor_metadata_index =
                input.allocation.predictor_metadata_index;
            node.entry.predictor_generation =
                input.allocation.predictor_generation;
            node.entry.generation = next_generation_;
            increment_generation();
            queue_.push_back(node);
            output.alloc_accepted = true;
            output.alloc_ftq_idx = tail_;
            output.alloc_generation = node.entry.generation;
            tail_ = advance(tail_);
        }

        if (input.read_valid) {
            typename std::deque<ReferenceNode>::const_iterator hit = find_const(
                input.read_ftq_idx, input.read_generation);
            if (hit != queue_.end()) {
                output.read_hit = true;
                output.read_entry = hit->entry;
            }
        }
        output.head = head_;
        output.tail = tail_;
        output.count = static_cast<uint8_t>(queue_.size());
        output.empty = queue_.empty();
        output.full = queue_.size() == Depth;
        return output;
    }

    const std::deque<ReferenceNode>& entries() const { return queue_; }
    std::size_t size() const { return queue_.size(); }
    uint8_t head() const { return head_; }
    uint8_t tail() const { return tail_; }

private:
    uint8_t advance(uint8_t index) const {
        return static_cast<uint8_t>((index + 1u) % Depth);
    }

    void increment_generation() {
        ++next_generation_;
        if (next_generation_ == 0) next_generation_ = 1;
    }

    typename std::deque<ReferenceNode>::iterator find(uint8_t index,
                                                       uint32_t generation) {
        for (typename std::deque<ReferenceNode>::iterator it = queue_.begin();
             it != queue_.end(); ++it) {
            if (it->index == index && it->entry.generation == generation) return it;
        }
        return queue_.end();
    }

    typename std::deque<ReferenceNode>::const_iterator find_const(
            uint8_t index, uint32_t generation) const {
        for (typename std::deque<ReferenceNode>::const_iterator it = queue_.begin();
             it != queue_.end(); ++it) {
            if (it->index == index && it->entry.generation == generation) return it;
        }
        return queue_.end();
    }

    bool apply_event(const boom::FtqLaneEvent& event) {
        if (event.lane > 1) return false;
        typename std::deque<ReferenceNode>::iterator node =
            find(event.ftq_idx, event.generation);
        if (node == queue_.end()) return false;
        const uint8_t bit = static_cast<uint8_t>(1u << event.lane);
        if ((node->entry.packet_valid_mask & bit) == 0 ||
            (node->entry.live_lane_mask & bit) == 0) return false;
        node->entry.live_lane_mask = static_cast<uint8_t>(
            node->entry.live_lane_mask & ~bit);
        return true;
    }

    std::deque<ReferenceNode> queue_;
    uint8_t head_;
    uint8_t tail_;
    uint32_t next_generation_;
};

struct Counters {
    uint64_t allocation_error = 0;
    uint64_t overflow_error = 0;
    uint64_t underflow_error = 0;
    uint64_t reclaim_error = 0;
    uint64_t ordering_error = 0;
    uint64_t live_mask_error = 0;
    uint64_t duplicate_retire_error = 0;
    uint64_t stale_reference_error = 0;
    uint64_t redirect_error = 0;
    uint64_t reset_error = 0;
    uint64_t metadata_error = 0;
    uint64_t pointer_error = 0;
    uint64_t wrap_error = 0;
    uint64_t checks = 0;
    bool diagnosed = false;

    uint64_t total_errors() const {
        return allocation_error + overflow_error + underflow_error + reclaim_error +
               ordering_error + live_mask_error + duplicate_retire_error +
               stale_reference_error + redirect_error + reset_error + metadata_error +
               pointer_error + wrap_error;
    }
};

void compare_value(bool equal, uint64_t& errors, Counters& counters,
                   std::size_t depth, uint32_t seed, uint32_t cycle,
                   const char* field, uint64_t expected, uint64_t actual) {
    ++counters.checks;
    if (equal) return;
    ++errors;
    if (!counters.diagnosed) {
        std::cerr << "FTQ mismatch depth=" << depth << " seed=" << seed
                  << " cycle=" << cycle << " field=" << field
                  << " expected=" << expected << " actual=" << actual << '\n';
        counters.diagnosed = true;
    }
}

bool is_stale_event(const boom::FtqLaneEvent& event,
                    const std::deque<ReferenceNode>& entries) {
    if (!event.valid) return false;
    for (std::deque<ReferenceNode>::const_iterator it = entries.begin();
         it != entries.end(); ++it) {
        if (it->index == event.ftq_idx && it->entry.generation == event.generation)
            return false;
    }
    return true;
}

template <std::size_t Depth>
void make_event(boom::FtqLaneEvent& event, uint32_t kind, uint32_t& random,
                const ReferenceFtq<Depth>& reference,
                const std::vector<Handle>& history) {
    event.valid = kind != 0;
    if (!event.valid) return;
    if ((kind == 1 || kind == 2) && !reference.entries().empty()) {
        const ReferenceNode& node = reference.entries()[
            random32(random) % reference.entries().size()];
        event.ftq_idx = node.index;
        event.generation = node.entry.generation;
        if (kind == 1 && node.entry.live_lane_mask != 0) {
            if (node.entry.live_lane_mask == 3)
                event.lane = static_cast<uint8_t>(random32(random) & 1u);
            else
                event.lane = (node.entry.live_lane_mask & 1u) ? 0 : 1;
        } else {
            event.lane = static_cast<uint8_t>(random32(random) & 3u);
        }
    } else if (kind == 3 && !history.empty()) {
        const Handle& old = history[random32(random) % history.size()];
        event.ftq_idx = old.index;
        event.generation = old.generation ^ 0x80000000u;
        event.lane = static_cast<uint8_t>(random32(random) & 1u);
    } else {
        event.ftq_idx = static_cast<uint8_t>(Depth + (random32(random) & 0x7fu));
        event.generation = random32(random);
        event.lane = static_cast<uint8_t>(2u + (random32(random) & 1u));
    }
}

boom::FtqAllocation random_allocation(uint32_t& random, uint32_t seed,
                                      uint32_t cycle) {
    boom::FtqAllocation allocation;
    const uint64_t high = random32(random);
    allocation.packet_base_pc = (high << 32) | random32(random);
    allocation.packet_base_pc ^= (static_cast<uint64_t>(seed) << 48) | cycle;
    static const uint8_t masks[] = {0, 1, 2, 3, 0xff, 4, 0x82, 3};
    allocation.packet_valid_mask = masks[random32(random) & 7u];
    allocation.prediction_valid = (random32(random) & 1u) != 0;
    allocation.predicted_taken = (random32(random) & 1u) != 0;
    allocation.target_valid = (random32(random) & 1u) != 0;
    allocation.predicted_target = (static_cast<uint64_t>(random32(random)) << 32) |
                                  random32(random);
    allocation.cfi_lane = static_cast<uint8_t>(random32(random));
    allocation.cfi_type = static_cast<uint8_t>(random32(random));
    allocation.predictor_metadata_index = static_cast<uint8_t>(random32(random));
    allocation.predictor_generation = random32(random);
    return allocation;
}

template <std::size_t Depth>
boom::FtqStepInput make_input(uint32_t seed, uint32_t cycle, uint32_t& random,
                              const ReferenceFtq<Depth>& reference,
                              const std::vector<Handle>& history) {
    boom::FtqStepInput input;
    input.allocation = random_allocation(random, seed, cycle);
    input.alloc_valid = (random32(random) & 3u) != 0;
    make_event(input.retire, random32(random) % 6u, random, reference, history);
    make_event(input.squash, random32(random) % 7u, random, reference, history);

    input.redirect.valid = (random32(random) & 31u) == 0;
    if (input.redirect.valid && !reference.entries().empty() &&
        (random32(random) & 3u) != 0) {
        const ReferenceNode& owner = reference.entries()[
            random32(random) % reference.entries().size()];
        input.redirect.owner_ftq_idx = owner.index;
        input.redirect.owner_generation = owner.entry.generation;
    } else {
        input.redirect.owner_ftq_idx = static_cast<uint8_t>(random32(random));
        input.redirect.owner_generation = random32(random) ^ 0x40000000u;
    }
    input.redirect.surviving_lane_mask = static_cast<uint8_t>(random32(random));

    input.read_valid = (random32(random) & 7u) != 0;
    if (!reference.entries().empty() && (random32(random) & 3u) != 0) {
        const ReferenceNode& node = reference.entries()[
            random32(random) % reference.entries().size()];
        input.read_ftq_idx = node.index;
        input.read_generation = node.entry.generation;
    } else if (!history.empty()) {
        const Handle& old = history[random32(random) % history.size()];
        input.read_ftq_idx = old.index;
        input.read_generation = old.generation ^ 0x20000000u;
    } else {
        input.read_ftq_idx = static_cast<uint8_t>(Depth + 1);
        input.read_generation = random32(random);
    }

    const uint32_t phase = cycle & 511u;
    if (phase == 0) {
        input.reset = true;
    } else if (phase <= Depth) {
        input.redirect.valid = false;
        input.retire.valid = false;
        input.squash.valid = false;
        input.alloc_valid = true;
        input.allocation.packet_valid_mask = 1;
    } else if (phase <= Depth + 8) {
        input.redirect.valid = false;
        input.retire.valid = false;
        input.squash.valid = false;
        input.alloc_valid = true;
        input.allocation.packet_valid_mask = 3;
    } else if (phase <= 2 * Depth + 8) {
        input.redirect.valid = false;
        input.alloc_valid = false;
        input.retire.valid = !reference.entries().empty();
        if (input.retire.valid) {
            const ReferenceNode& front = reference.entries().front();
            input.retire.ftq_idx = front.index;
            input.retire.generation = front.entry.generation;
            input.retire.lane = (front.entry.live_lane_mask & 1u) ? 0 : 1;
            input.squash = input.retire;
        } else {
            input.squash.valid = false;
        }
    } else if (phase <= 3 * Depth + 8) {
        input.redirect.valid = false;
        input.retire.valid = false;
        input.squash.valid = false;
        input.alloc_valid = true;
        input.allocation.packet_valid_mask = (phase & 1u) ? 1 : 3;
    } else if (phase == 3 * Depth + 9 && !reference.entries().empty()) {
        const ReferenceNode& owner = reference.entries()[reference.size() / 2];
        input.redirect.valid = true;
        input.redirect.owner_ftq_idx = owner.index;
        input.redirect.owner_generation = owner.entry.generation;
        input.redirect.surviving_lane_mask = static_cast<uint8_t>(random32(random));
        input.alloc_valid = true;
    }
    return input;
}

template <std::size_t Depth>
void compare_outputs(const boom::FtqStepInput& input,
                     const boom::FtqStepOutput& expected,
                     const boom::FtqStepOutput& actual,
                     const ReferenceFtq<Depth>& reference,
                     const std::deque<ReferenceNode>& before,
                     Counters& counters, uint32_t seed, uint32_t cycle,
                     uint8_t old_head, uint8_t old_tail) {
    uint64_t& allocation = input.reset ? counters.reset_error : counters.allocation_error;
    compare_value(expected.alloc_ready == actual.alloc_ready, allocation, counters,
                  Depth, seed, cycle, "alloc_ready", expected.alloc_ready, actual.alloc_ready);
    compare_value(expected.alloc_accepted == actual.alloc_accepted, allocation, counters,
                  Depth, seed, cycle, "alloc_accepted", expected.alloc_accepted, actual.alloc_accepted);
    compare_value(expected.alloc_invalid_mask == actual.alloc_invalid_mask, allocation, counters,
                  Depth, seed, cycle, "alloc_invalid_mask", expected.alloc_invalid_mask,
                  actual.alloc_invalid_mask);
    compare_value(expected.alloc_ftq_idx == actual.alloc_ftq_idx, allocation, counters,
                  Depth, seed, cycle, "alloc_ftq_idx", expected.alloc_ftq_idx,
                  actual.alloc_ftq_idx);
    compare_value(expected.alloc_generation == actual.alloc_generation, allocation, counters,
                  Depth, seed, cycle, "alloc_generation", expected.alloc_generation,
                  actual.alloc_generation);

    const bool duplicate = input.retire.valid && input.squash.valid &&
        input.retire.ftq_idx == input.squash.ftq_idx &&
        input.retire.generation == input.squash.generation &&
        input.retire.lane == input.squash.lane;
    const bool stale = is_stale_event(input.retire, before) ||
                       is_stale_event(input.squash, before);
    uint64_t& event_errors = input.reset ? counters.reset_error
        : duplicate ? counters.duplicate_retire_error
        : stale ? counters.stale_reference_error : counters.live_mask_error;
    compare_value(expected.retire_accepted == actual.retire_accepted, event_errors, counters,
                  Depth, seed, cycle, "retire_accepted", expected.retire_accepted,
                  actual.retire_accepted);
    compare_value(expected.retire_rejected == actual.retire_rejected, event_errors, counters,
                  Depth, seed, cycle, "retire_rejected", expected.retire_rejected,
                  actual.retire_rejected);
    compare_value(expected.squash_accepted == actual.squash_accepted, event_errors, counters,
                  Depth, seed, cycle, "squash_accepted", expected.squash_accepted,
                  actual.squash_accepted);
    compare_value(expected.squash_rejected == actual.squash_rejected, event_errors, counters,
                  Depth, seed, cycle, "squash_rejected", expected.squash_rejected,
                  actual.squash_rejected);

    uint64_t& redirect = input.reset ? counters.reset_error : counters.redirect_error;
    compare_value(expected.redirect_accepted == actual.redirect_accepted, redirect, counters,
                  Depth, seed, cycle, "redirect_accepted", expected.redirect_accepted,
                  actual.redirect_accepted);
    compare_value(expected.redirect_rejected == actual.redirect_rejected, redirect, counters,
                  Depth, seed, cycle, "redirect_rejected", expected.redirect_rejected,
                  actual.redirect_rejected);

    uint64_t& reclaim = input.reset ? counters.reset_error : counters.reclaim_error;
    compare_value(expected.reclaimed == actual.reclaimed, reclaim, counters,
                  Depth, seed, cycle, "reclaimed", expected.reclaimed, actual.reclaimed);
    compare_value(expected.reclaimed_ftq_idx == actual.reclaimed_ftq_idx, reclaim, counters,
                  Depth, seed, cycle, "reclaimed_ftq_idx", expected.reclaimed_ftq_idx,
                  actual.reclaimed_ftq_idx);

    const bool stale_read = input.read_valid && !expected.read_hit;
    uint64_t& read_status = input.reset ? counters.reset_error
        : stale_read ? counters.stale_reference_error : counters.metadata_error;
    compare_value(expected.read_hit == actual.read_hit, read_status, counters,
                  Depth, seed, cycle, "read_hit", expected.read_hit, actual.read_hit);

#define CHECK_ENTRY(field, category) \
    compare_value(expected.read_entry.field == actual.read_entry.field, category, counters, \
                  Depth, seed, cycle, "read_entry." #field, \
                  static_cast<uint64_t>(expected.read_entry.field), \
                  static_cast<uint64_t>(actual.read_entry.field))
    uint64_t& metadata = input.reset ? counters.reset_error : counters.metadata_error;
    uint64_t& live_mask = input.reset ? counters.reset_error : counters.live_mask_error;
    CHECK_ENTRY(valid, metadata);
    CHECK_ENTRY(packet_base_pc, metadata);
    CHECK_ENTRY(packet_valid_mask, metadata);
    CHECK_ENTRY(live_lane_mask, live_mask);
    CHECK_ENTRY(prediction_valid, metadata);
    CHECK_ENTRY(predicted_taken, metadata);
    CHECK_ENTRY(target_valid, metadata);
    CHECK_ENTRY(predicted_target, metadata);
    CHECK_ENTRY(cfi_lane, metadata);
    CHECK_ENTRY(cfi_type, metadata);
    CHECK_ENTRY(predictor_metadata_index, metadata);
    CHECK_ENTRY(predictor_generation, metadata);
    CHECK_ENTRY(generation, metadata);
#undef CHECK_ENTRY

    const bool wrapped = (!input.reset &&
        ((expected.head < old_head) || (expected.tail < old_tail)));
    uint64_t& pointers = input.reset ? counters.reset_error
        : wrapped ? counters.wrap_error : counters.pointer_error;
    compare_value(expected.head == actual.head, pointers, counters,
                  Depth, seed, cycle, "head", expected.head, actual.head);
    compare_value(expected.tail == actual.tail, pointers, counters,
                  Depth, seed, cycle, "tail", expected.tail, actual.tail);
    compare_value(expected.count == actual.count, pointers, counters,
                  Depth, seed, cycle, "count", expected.count, actual.count);
    compare_value(expected.empty == actual.empty, pointers, counters,
                  Depth, seed, cycle, "empty", expected.empty, actual.empty);
    compare_value(expected.full == actual.full, pointers, counters,
                  Depth, seed, cycle, "full", expected.full, actual.full);

    compare_value(reference.size() <= Depth, counters.overflow_error, counters,
                  Depth, seed, cycle, "reference_overflow", 1, reference.size() <= Depth);
    compare_value(!(before.empty() && expected.reclaimed), counters.underflow_error, counters,
                  Depth, seed, cycle, "reference_underflow", 1,
                  !(before.empty() && expected.reclaimed));
    bool ordered = true;
    uint8_t index = reference.head();
    for (typename std::deque<ReferenceNode>::const_iterator it = reference.entries().begin();
         it != reference.entries().end(); ++it) {
        if (it->index != index) ordered = false;
        index = static_cast<uint8_t>((index + 1u) % Depth);
    }
    compare_value(ordered && index == reference.tail(), counters.ordering_error, counters,
                  Depth, seed, cycle, "reference_order", 1,
                  ordered && index == reference.tail());
}

template <std::size_t Depth>
void run_depth(Counters& counters) {
    for (uint32_t seed = 0; seed < kSeeds; ++seed) {
        uint32_t random = 0x9e3779b9u ^ (seed * 0x85ebca6bu) ^
                          static_cast<uint32_t>(Depth * 0xc2b2ae35u);
        boom::FtqFoundation<Depth> dut;
        ReferenceFtq<Depth> reference;
        std::vector<Handle> history;
        history.reserve(kCyclesPerSeed);
        for (uint32_t cycle = 0; cycle < kCyclesPerSeed; ++cycle) {
            const boom::FtqStepInput input = make_input(
                seed, cycle, random, reference, history);
            const std::deque<ReferenceNode> before = reference.entries();
            const uint8_t old_head = reference.head();
            const uint8_t old_tail = reference.tail();
            const boom::FtqStepOutput expected = reference.step(input);
            const boom::FtqStepOutput actual = dut.step(input);
            compare_outputs(input, expected, actual, reference, before, counters,
                            seed, cycle, old_head, old_tail);
            if (expected.alloc_accepted) {
                Handle handle;
                handle.index = expected.alloc_ftq_idx;
                handle.generation = expected.alloc_generation;
                history.push_back(handle);
            }
        }
    }
}

}  // namespace

int main() {
    Counters counters;
    run_depth<8>(counters);
    run_depth<16>(counters);
    run_depth<32>(counters);
    run_depth<64>(counters);

    const uint64_t total_cycles = static_cast<uint64_t>(kSeeds) *
                                  kCyclesPerSeed * 4u;
    std::cout << (counters.total_errors() == 0 ? "FTQ_RANDOM_PASS" : "FTQ_RANDOM_FAIL")
              << " allocation_error=" << counters.allocation_error
              << " overflow_error=" << counters.overflow_error
              << " underflow_error=" << counters.underflow_error
              << " reclaim_error=" << counters.reclaim_error
              << " ordering_error=" << counters.ordering_error
              << " live_mask_error=" << counters.live_mask_error
              << " duplicate_retire_error=" << counters.duplicate_retire_error
              << " stale_reference_error=" << counters.stale_reference_error
              << " redirect_error=" << counters.redirect_error
              << " reset_error=" << counters.reset_error
              << " metadata_error=" << counters.metadata_error
              << " pointer_error=" << counters.pointer_error
              << " wrap_error=" << counters.wrap_error
              << " checks=" << counters.checks
              << " seeds=" << kSeeds
              << " cycles=" << total_cycles
              << " cycles_per_seed=" << kCyclesPerSeed
              << " depths=8/16/32/64\n";
    return counters.total_errors() == 0 ? 0 : 1;
}
