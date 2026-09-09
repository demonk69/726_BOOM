#include "ftq.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <deque>

namespace {

const unsigned kDepth = 32;
const unsigned kSeeds = 256;
const unsigned kCycles = 8192;

struct Rng {
    uint64_t state;
    explicit Rng(unsigned seed) : state((0x9e3779b97f4a7c15ULL ^ seed) | 1ULL) {}
    uint32_t next() {
        state ^= state << 7;
        state ^= state >> 9;
        state ^= state << 8;
        return static_cast<uint32_t>(state);
    }
};

struct Ref {
    uint8_t idx;
    uint8_t lane;
    uint8_t offset;
    uint32_t generation;
    Ref() : idx(0), lane(0), offset(0), generation(0) {}
};

struct ModelEntry {
    boom::FtqEntry entry;
};

struct Model {
    ModelEntry entries[kDepth];
    uint8_t head;
    uint8_t tail;
    uint8_t count;
    uint32_t next_generation;
    Model() : entries(), head(0), tail(0), count(0), next_generation(1) {}

    bool active(uint8_t idx, uint32_t generation) const {
        return idx < kDepth && entries[idx].entry.valid &&
               entries[idx].entry.generation == generation;
    }
    bool lane(const boom::FtqLaneEvent& event) {
        if (!event.valid || event.lane > 1 ||
            !active(event.ftq_idx, event.generation)) return false;
        boom::FtqEntry& entry = entries[event.ftq_idx].entry;
        const uint8_t bit = static_cast<uint8_t>(1u << event.lane);
        if ((entry.packet_valid_mask & bit) == 0 ||
            (entry.live_lane_mask & bit) == 0) return false;
        entry.live_lane_mask = static_cast<uint8_t>(entry.live_lane_mask & ~bit);
        return true;
    }
    boom::FtqStepOutput step(const boom::FtqStepInput& input) {
        boom::FtqStepOutput output;
        if (input.reset) {
            for (unsigned i = 0; i < kDepth; ++i) {
                entries[i].entry.valid = false;
                entries[i].entry.live_lane_mask = 0;
            }
            head = tail = count = 0;
            if (++next_generation == 0) next_generation = 1;
            output.empty = true;
            return output;
        }
        if (input.redirect.valid) {
            if (active(input.redirect.owner_ftq_idx,
                       input.redirect.owner_generation)) {
                const uint8_t distance = static_cast<uint8_t>(
                    (input.redirect.owner_ftq_idx + kDepth - head) & (kDepth - 1));
                if (distance < count) {
                    for (unsigned offset = distance + 1; offset < count; ++offset) {
                        const uint8_t idx = static_cast<uint8_t>((head + offset) &
                                                                 (kDepth - 1));
                        entries[idx].entry.valid = false;
                        entries[idx].entry.live_lane_mask = 0;
                    }
                    boom::FtqEntry& owner = entries[input.redirect.owner_ftq_idx].entry;
                    owner.live_lane_mask = static_cast<uint8_t>(
                        owner.live_lane_mask & input.redirect.surviving_lane_mask &
                        owner.packet_valid_mask);
                    tail = static_cast<uint8_t>((input.redirect.owner_ftq_idx + 1) &
                                                (kDepth - 1));
                    count = static_cast<uint8_t>(distance + 1);
                    output.redirect_accepted = true;
                } else output.redirect_rejected = true;
            } else output.redirect_rejected = true;
            output.retire_rejected = input.retire.valid;
            output.squash_rejected = input.squash.valid;
        } else {
            if (input.retire.valid) {
                output.retire_accepted = lane(input.retire);
                output.retire_rejected = !output.retire_accepted;
            }
            if (input.squash.valid) {
                output.squash_accepted = lane(input.squash);
                output.squash_rejected = !output.squash_accepted;
            }
        }
        if (count && entries[head].entry.valid &&
            entries[head].entry.live_lane_mask == 0) {
            output.reclaimed = true;
            output.reclaimed_ftq_idx = head;
            entries[head].entry.valid = false;
            head = static_cast<uint8_t>((head + 1) & (kDepth - 1));
            --count;
        }
        output.alloc_ready = !input.redirect.valid && count < kDepth;
        const uint8_t mask = static_cast<uint8_t>(input.allocation.packet_valid_mask & 3u);
        output.alloc_invalid_mask = input.alloc_valid &&
            (mask != input.allocation.packet_valid_mask || mask == 2);
        if (input.alloc_valid && output.alloc_ready &&
            !output.alloc_invalid_mask && mask != 0) {
            boom::FtqEntry entry;
            entry.valid = true;
            entry.packet_base_pc = input.allocation.packet_base_pc;
            entry.packet_valid_mask = mask;
            entry.live_lane_mask = mask;
            entry.prediction_valid = input.allocation.prediction_valid;
            entry.predicted_taken = entry.prediction_valid &&
                                    input.allocation.predicted_taken;
            entry.target_valid = entry.prediction_valid && input.allocation.target_valid;
            entry.predicted_target = entry.target_valid ?
                input.allocation.predicted_target : 0;
            entry.cfi_lane = input.allocation.cfi_lane & 1u;
            entry.cfi_type = input.allocation.cfi_type & 3u;
            entry.predictor_metadata_index = input.allocation.predictor_metadata_index;
            entry.predictor_generation = input.allocation.predictor_generation;
            entry.generation = next_generation;
            if (++next_generation == 0) next_generation = 1;
            entries[tail].entry = entry;
            output.alloc_accepted = true;
            output.alloc_ftq_idx = tail;
            output.alloc_generation = entry.generation;
            tail = static_cast<uint8_t>((tail + 1) & (kDepth - 1));
            ++count;
        }
        if (input.read_valid && active(input.read_ftq_idx, input.read_generation)) {
            output.read_hit = true;
            output.read_entry = entries[input.read_ftq_idx].entry;
        }
        output.head = head;
        output.tail = tail;
        output.count = count;
        output.empty = count == 0;
        output.full = count == kDepth;
        return output;
    }
};

struct Errors {
    uint64_t atomicity_error, orphan_fb_packet, orphan_ftq_entry, ftq_overwrite;
    uint64_t reference_error, generation_error, stale_reference_error;
    uint64_t live_mask_error, reclaim_error, ordering_error, metadata_error;
    uint64_t packet_mask_error, commit_error, squash_error, exception_error;
    uint64_t reset_error;
    Errors() : atomicity_error(0), orphan_fb_packet(0), orphan_ftq_entry(0),
        ftq_overwrite(0), reference_error(0), generation_error(0),
        stale_reference_error(0), live_mask_error(0), reclaim_error(0),
        ordering_error(0), metadata_error(0), packet_mask_error(0),
        commit_error(0), squash_error(0), exception_error(0), reset_error(0) {}
    uint64_t total() const {
        return atomicity_error + orphan_fb_packet + orphan_ftq_entry + ftq_overwrite +
            reference_error + generation_error + stale_reference_error +
            live_mask_error + reclaim_error + ordering_error + metadata_error +
            packet_mask_error + commit_error + squash_error + exception_error +
            reset_error;
    }
};

bool same_entry(const boom::FtqEntry& a, const boom::FtqEntry& b) {
    return a.valid == b.valid && a.packet_base_pc == b.packet_base_pc &&
        a.packet_valid_mask == b.packet_valid_mask &&
        a.live_lane_mask == b.live_lane_mask &&
        a.prediction_valid == b.prediction_valid &&
        a.predicted_taken == b.predicted_taken && a.target_valid == b.target_valid &&
        a.predicted_target == b.predicted_target && a.cfi_lane == b.cfi_lane &&
        a.cfi_type == b.cfi_type &&
        a.predictor_metadata_index == b.predictor_metadata_index &&
        a.predictor_generation == b.predictor_generation &&
        a.generation == b.generation;
}

void campaign(unsigned seed, Errors& errors, uint8_t& maximum) {
    Rng rng(seed);
    boom::FtqFoundation<kDepth> dut;
    Model model;
    std::deque<Ref> fb;
    std::deque<Ref> rob;
    Ref stale;
    bool stale_valid = false;
    for (unsigned cycle = 0; cycle < kCycles; ++cycle) {
        boom::FtqStepInput input;
        const uint32_t random = rng.next();
        const bool do_reset = (random & 1023u) == 0;
        bool injected_stale = false;
        if (do_reset) input.reset = true;

        if (!do_reset && !rob.empty() && (random & 15u) == 1u) {
            input.retire.valid = true;
            input.retire.ftq_idx = rob.front().idx;
            input.retire.generation = rob.front().generation;
            input.retire.lane = rob.front().lane;
        } else if (!do_reset && stale_valid && (random & 63u) == 2u) {
            injected_stale = true;
            input.retire.valid = true;
            input.retire.ftq_idx = stale.idx;
            input.retire.generation = stale.generation;
            input.retire.lane = stale.lane;
        }
        if (!do_reset && !rob.empty() && (random & 31u) == 3u) {
            input.squash.valid = true;
            input.squash.ftq_idx = rob.back().idx;
            input.squash.generation = rob.back().generation;
            input.squash.lane = rob.back().lane;
        }
        const bool exception = !do_reset && !rob.empty() && (random & 255u) == 4u;
        const bool branch = !do_reset && !rob.empty() && (random & 127u) == 5u;
        if (exception || branch) {
            const Ref owner = rob.front();
            input.redirect.valid = true;
            input.redirect.owner_ftq_idx = owner.idx;
            input.redirect.owner_generation = owner.generation;
            input.redirect.surviving_lane_mask = static_cast<uint8_t>(1u << owner.lane);
        }

        const bool packet_valid = (random & 3u) != 0;
        const uint8_t mask = (random & 4u) ? 3 : 1;
        const bool fb_ready = fb.size() + ((random & 8u) && !fb.empty() ? 0u : 1u) <= 8;
        const bool admission_conditions = !do_reset && !input.redirect.valid;
        input.alloc_valid = packet_valid && fb_ready && admission_conditions;
        input.allocation.packet_base_pc = 0x80000000ull +
            static_cast<uint64_t>(seed) * 0x10000ull + cycle * 4ull;
        input.allocation.packet_valid_mask = mask;
        input.allocation.prediction_valid = (random & 16u) != 0;
        input.allocation.predicted_taken = (random & 32u) != 0;
        input.allocation.target_valid = (random & 64u) != 0;
        input.allocation.predicted_target = input.allocation.packet_base_pc + 8;
        input.allocation.cfi_lane = (random >> 7) & 1u;
        input.allocation.cfi_type = (random >> 8) & 3u;
        input.allocation.predictor_metadata_index =
            static_cast<uint8_t>((input.allocation.packet_base_pc >> 1) & 0xffu);
        input.allocation.predictor_generation = 9;

        if (!rob.empty() && (random & 7u) == 0) {
            input.read_valid = true;
            input.read_ftq_idx = rob.front().idx;
            input.read_generation = rob.front().generation;
        }

        const boom::FtqStepOutput expected = model.step(input);
        const boom::FtqStepOutput actual = dut.step(input);
        maximum = std::max(maximum, actual.count);

        if (actual.alloc_accepted != expected.alloc_accepted) ++errors.atomicity_error;
        if (actual.alloc_accepted && !input.alloc_valid) ++errors.orphan_ftq_entry;
        const bool fb_enqueue = actual.alloc_accepted;
        if (fb_enqueue != actual.alloc_accepted) ++errors.orphan_fb_packet;
        if (actual.alloc_invalid_mask || expected.alloc_invalid_mask) ++errors.packet_mask_error;
        if (actual.count > kDepth) ++errors.ftq_overwrite;
        if (actual.alloc_accepted && actual.alloc_generation != expected.alloc_generation)
            ++errors.generation_error;
        if (actual.head != expected.head || actual.tail != expected.tail ||
            actual.count != expected.count || actual.full != expected.full ||
            actual.empty != expected.empty) ++errors.ordering_error;
        if (actual.reclaimed != expected.reclaimed ||
            (actual.reclaimed && actual.reclaimed_ftq_idx != expected.reclaimed_ftq_idx))
            ++errors.reclaim_error;
        if (actual.retire_accepted != expected.retire_accepted ||
            actual.retire_rejected != expected.retire_rejected) ++errors.commit_error;
        if (actual.squash_accepted != expected.squash_accepted ||
            actual.squash_rejected != expected.squash_rejected) ++errors.squash_error;
        if (actual.redirect_accepted != expected.redirect_accepted ||
            actual.redirect_rejected != expected.redirect_rejected)
            exception ? ++errors.exception_error : ++errors.squash_error;
        if (actual.read_hit != expected.read_hit) ++errors.reference_error;
        if (actual.read_hit && !same_entry(actual.read_entry, expected.read_entry))
            ++errors.metadata_error;

        if (do_reset) {
            if (!actual.empty || actual.count != 0 || actual.alloc_accepted ||
                actual.retire_accepted || actual.redirect_accepted) ++errors.reset_error;
            if (!rob.empty()) {
                stale = rob.front();
                stale_valid = true;
            } else if (!fb.empty()) {
                stale = fb.front();
                stale_valid = true;
            }
            fb.clear();
            rob.clear();
        } else {
            if (actual.retire_accepted && !rob.empty()) {
                stale = rob.front();
                stale_valid = true;
                rob.pop_front();
            }
            if (actual.squash_accepted && !rob.empty()) rob.pop_back();
            if (input.redirect.valid && actual.redirect_accepted) {
                while (rob.size() > 1) rob.pop_back();
                while (!fb.empty()) fb.pop_back();
            }
            if (fb_enqueue) {
                for (uint8_t lane = 0; lane < 2; ++lane) {
                    if ((mask & static_cast<uint8_t>(1u << lane)) == 0) continue;
                    Ref ref;
                    ref.idx = actual.alloc_ftq_idx;
                    ref.generation = actual.alloc_generation;
                    ref.lane = lane;
                    ref.offset = static_cast<uint8_t>(lane * 2);
                    fb.push_back(ref);
                }
            }
            if (!fb.empty() && (random & 8u)) {
                rob.push_back(fb.front());
                fb.pop_front();
            }
        }
        if (injected_stale && actual.retire_accepted)
            ++errors.stale_reference_error;
        for (unsigned i = 0; i < kDepth; ++i) {
            if (model.entries[i].entry.valid &&
                (model.entries[i].entry.live_lane_mask &
                 ~model.entries[i].entry.packet_valid_mask) != 0)
                ++errors.live_mask_error;
        }
    }
}

void long_run(uint64_t& leak_errors, uint8_t& maximum) {
    boom::FtqFoundation<kDepth> dut;
    std::deque<Ref> refs;
    const uint64_t steps = 1000000;
    for (uint64_t step = 0; step < steps; ++step) {
        boom::FtqStepInput input;
        if (!refs.empty()) {
            input.retire.valid = true;
            input.retire.ftq_idx = refs.front().idx;
            input.retire.generation = refs.front().generation;
            input.retire.lane = refs.front().lane;
        }
        input.alloc_valid = true;
        input.allocation.packet_base_pc = 0x90000000ull + step * 4;
        input.allocation.packet_valid_mask = 1;
        const boom::FtqStepOutput output = dut.step(input);
        maximum = std::max(maximum, output.count);
        if (input.retire.valid) {
            if (!output.retire_accepted) ++leak_errors;
            refs.pop_front();
        }
        if (output.alloc_accepted) {
            Ref ref;
            ref.idx = output.alloc_ftq_idx;
            ref.generation = output.alloc_generation;
            refs.push_back(ref);
        } else ++leak_errors;
        if (output.count != refs.size()) ++leak_errors;
    }
    while (!refs.empty()) {
        boom::FtqStepInput input;
        input.retire.valid = true;
        input.retire.ftq_idx = refs.front().idx;
        input.retire.generation = refs.front().generation;
        input.retire.lane = refs.front().lane;
        const boom::FtqStepOutput output = dut.step(input);
        if (!output.retire_accepted) ++leak_errors;
        refs.pop_front();
    }
    boom::FtqStepInput idle;
    const boom::FtqStepOutput final = dut.step(idle);
    if (!final.empty || final.count != 0) ++leak_errors;
    std::printf("PF3_LONG_RUN_STEPS=%llu FTQ_LEAK_ERRORS=%llu MAX_FTQ_OCCUPANCY=%u\n",
                static_cast<unsigned long long>(steps),
                static_cast<unsigned long long>(leak_errors), maximum);
}

}  // namespace

int main() {
    Errors errors;
    uint8_t maximum = 0;
    for (unsigned seed = 0; seed < kSeeds; ++seed) campaign(seed, errors, maximum);
    uint64_t leak_errors = 0;
    long_run(leak_errors, maximum);
    std::printf(
        "PF3_RANDOM_%s seeds=%u cycles_per_seed=%u atomicity_error=%llu "
        "orphan_fb_packet=%llu orphan_ftq_entry=%llu ftq_overwrite=%llu "
        "reference_error=%llu generation_error=%llu stale_reference_error=%llu "
        "live_mask_error=%llu reclaim_error=%llu ordering_error=%llu metadata_error=%llu "
        "packet_mask_error=%llu commit_error=%llu squash_error=%llu exception_error=%llu "
        "reset_error=%llu\n",
        errors.total() == 0 && leak_errors == 0 ? "PASS" : "FAIL", kSeeds, kCycles,
        (unsigned long long)errors.atomicity_error,
        (unsigned long long)errors.orphan_fb_packet,
        (unsigned long long)errors.orphan_ftq_entry,
        (unsigned long long)errors.ftq_overwrite,
        (unsigned long long)errors.reference_error,
        (unsigned long long)errors.generation_error,
        (unsigned long long)errors.stale_reference_error,
        (unsigned long long)errors.live_mask_error,
        (unsigned long long)errors.reclaim_error,
        (unsigned long long)errors.ordering_error,
        (unsigned long long)errors.metadata_error,
        (unsigned long long)errors.packet_mask_error,
        (unsigned long long)errors.commit_error,
        (unsigned long long)errors.squash_error,
        (unsigned long long)errors.exception_error,
        (unsigned long long)errors.reset_error);
    return errors.total() == 0 && leak_errors == 0 ? 0 : 1;
}
