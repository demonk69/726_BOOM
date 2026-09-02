#include "ftq.hpp"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace {

using boom::FtqAllocation;
using boom::FtqEntry;
using boom::FtqStepInput;
using boom::FtqStepOutput;

struct Results {
    uint64_t checks;
    uint64_t failures;

    Results() : checks(0), failures(0) {}

    template <typename A, typename B>
    void equal(const A& actual, const B& expected, const std::string& what) {
        ++checks;
        if (actual == expected) return;
        ++failures;
        if (failures <= 20) {
            std::cerr << "FAIL " << what << " actual="
                      << static_cast<uint64_t>(actual) << " expected="
                      << static_cast<uint64_t>(expected) << '\n';
        }
    }

    void require(bool condition, const std::string& what) {
        equal(condition, true, what);
    }
};

void compare_entry(Results& results, const FtqEntry& actual,
                   const FtqEntry& expected, const std::string& tag) {
    results.equal(actual.valid, expected.valid, tag + ".valid");
    results.equal(actual.packet_base_pc, expected.packet_base_pc,
                  tag + ".pc");
    results.equal(actual.packet_valid_mask, expected.packet_valid_mask,
                  tag + ".packet_mask");
    results.equal(actual.live_lane_mask, expected.live_lane_mask,
                  tag + ".live_mask");
    results.equal(actual.prediction_valid, expected.prediction_valid,
                  tag + ".prediction_valid");
    results.equal(actual.predicted_taken, expected.predicted_taken,
                  tag + ".predicted_taken");
    results.equal(actual.target_valid, expected.target_valid,
                  tag + ".target_valid");
    results.equal(actual.predicted_target, expected.predicted_target,
                  tag + ".target");
    results.equal(actual.cfi_lane, expected.cfi_lane, tag + ".cfi_lane");
    results.equal(actual.cfi_type, expected.cfi_type, tag + ".cfi_type");
    results.equal(actual.predictor_metadata_index,
                  expected.predictor_metadata_index, tag + ".metadata");
    results.equal(actual.predictor_generation, expected.predictor_generation,
                  tag + ".predictor_generation");
    results.equal(actual.generation, expected.generation,
                  tag + ".generation");
}

void compare_output(Results& results, const FtqStepOutput& actual,
                    const FtqStepOutput& expected, const std::string& tag) {
#define CHECK_FIELD(name) results.equal(actual.name, expected.name, tag + "." #name)
    CHECK_FIELD(alloc_ready);
    CHECK_FIELD(alloc_accepted);
    CHECK_FIELD(alloc_invalid_mask);
    CHECK_FIELD(alloc_ftq_idx);
    CHECK_FIELD(alloc_generation);
    CHECK_FIELD(retire_accepted);
    CHECK_FIELD(retire_rejected);
    CHECK_FIELD(squash_accepted);
    CHECK_FIELD(squash_rejected);
    CHECK_FIELD(redirect_accepted);
    CHECK_FIELD(redirect_rejected);
    CHECK_FIELD(reclaimed);
    CHECK_FIELD(reclaimed_ftq_idx);
    CHECK_FIELD(read_hit);
    CHECK_FIELD(empty);
    CHECK_FIELD(full);
    CHECK_FIELD(head);
    CHECK_FIELD(tail);
    CHECK_FIELD(count);
#undef CHECK_FIELD
    if (actual.read_hit || expected.read_hit)
        compare_entry(results, actual.read_entry, expected.read_entry,
                      tag + ".read_entry");
}

template <std::size_t Depth>
class ReferenceFtq {
public:
    struct Node {
        uint8_t index;
        FtqEntry entry;
    };

    ReferenceFtq() : head_(0), tail_(0), next_generation_(1) {}

    FtqStepOutput step(const FtqStepInput& input) {
        FtqStepOutput output;
        if (input.reset) {
            queue_.clear();
            head_ = 0;
            tail_ = 0;
            bump_generation();
            output.head = 0;
            output.tail = 0;
            output.count = 0;
            output.empty = true;
            return output;
        }

        if (input.redirect.valid) {
            typename std::deque<Node>::iterator owner = find(
                input.redirect.owner_ftq_idx,
                input.redirect.owner_generation);
            if (owner == queue_.end()) {
                output.redirect_rejected = true;
            } else {
                const uint8_t owner_index = owner->index;
                owner->entry.live_lane_mask = static_cast<uint8_t>(
                    owner->entry.live_lane_mask &
                    input.redirect.surviving_lane_mask &
                    owner->entry.packet_valid_mask);
                queue_.erase(owner + 1, queue_.end());
                tail_ = advance(owner_index);
                output.redirect_accepted = true;
            }
            output.retire_rejected = input.retire.valid;
            output.squash_rejected = input.squash.valid;
        } else {
            if (input.retire.valid) {
                output.retire_accepted = apply(input.retire.ftq_idx,
                                               input.retire.generation,
                                               input.retire.lane);
                output.retire_rejected = !output.retire_accepted;
            }
            if (input.squash.valid) {
                output.squash_accepted = apply(input.squash.ftq_idx,
                                               input.squash.generation,
                                               input.squash.lane);
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
        const uint8_t mask = static_cast<uint8_t>(
            input.allocation.packet_valid_mask & 3u);
        output.alloc_invalid_mask = input.alloc_valid &&
            (input.allocation.packet_valid_mask != mask || mask == 2u);
        if (input.alloc_valid && output.alloc_ready &&
            !output.alloc_invalid_mask && mask != 0) {
            Node node;
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
            node.entry.predicted_target = node.entry.target_valid ?
                input.allocation.predicted_target : 0;
            node.entry.cfi_lane = static_cast<uint8_t>(
                input.allocation.cfi_lane & 1u);
            node.entry.cfi_type = static_cast<uint8_t>(
                input.allocation.cfi_type & 3u);
            node.entry.predictor_metadata_index =
                input.allocation.predictor_metadata_index;
            node.entry.predictor_generation =
                input.allocation.predictor_generation;
            node.entry.generation = next_generation_;
            bump_generation();
            queue_.push_back(node);
            output.alloc_accepted = true;
            output.alloc_ftq_idx = tail_;
            output.alloc_generation = node.entry.generation;
            tail_ = advance(tail_);
        }

        if (input.read_valid) {
            typename std::deque<Node>::const_iterator node = find_const(
                input.read_ftq_idx, input.read_generation);
            if (node != queue_.end()) {
                output.read_hit = true;
                output.read_entry = node->entry;
            }
        }
        output.head = head_;
        output.tail = tail_;
        output.count = static_cast<uint8_t>(queue_.size());
        output.empty = queue_.empty();
        output.full = queue_.size() == Depth;
        return output;
    }

    const std::deque<Node>& queue() const { return queue_; }
    uint8_t head() const { return head_; }
    uint8_t tail() const { return tail_; }

    std::string signature() const {
        std::ostringstream stream;
        stream << static_cast<unsigned>(head_) << ':'
               << static_cast<unsigned>(tail_) << ':' << next_generation_;
        for (typename std::deque<Node>::const_iterator it = queue_.begin();
             it != queue_.end(); ++it) {
            stream << '|' << static_cast<unsigned>(it->index) << ','
                   << it->entry.generation << ','
                   << static_cast<unsigned>(it->entry.packet_valid_mask) << ','
                   << static_cast<unsigned>(it->entry.live_lane_mask);
        }
        return stream.str();
    }

private:
    std::deque<Node> queue_;
    uint8_t head_;
    uint8_t tail_;
    uint32_t next_generation_;

    static uint8_t advance(uint8_t index) {
        return static_cast<uint8_t>((index + 1u) % Depth);
    }

    void bump_generation() {
        ++next_generation_;
        if (next_generation_ == 0) next_generation_ = 1;
    }

    typename std::deque<Node>::iterator find(uint8_t index,
                                              uint32_t generation) {
        for (typename std::deque<Node>::iterator it = queue_.begin();
             it != queue_.end(); ++it) {
            if (it->index == index && it->entry.generation == generation)
                return it;
        }
        return queue_.end();
    }

    typename std::deque<Node>::const_iterator find_const(
            uint8_t index, uint32_t generation) const {
        for (typename std::deque<Node>::const_iterator it = queue_.begin();
             it != queue_.end(); ++it) {
            if (it->index == index && it->entry.generation == generation)
                return it;
        }
        return queue_.end();
    }

    bool apply(uint8_t index, uint32_t generation, uint8_t lane) {
        typename std::deque<Node>::iterator node = find(index, generation);
        if (node == queue_.end() || lane > 1) return false;
        const uint8_t bit = static_cast<uint8_t>(1u << lane);
        if ((node->entry.packet_valid_mask & bit) == 0 ||
            (node->entry.live_lane_mask & bit) == 0)
            return false;
        node->entry.live_lane_mask = static_cast<uint8_t>(
            node->entry.live_lane_mask & ~bit);
        return true;
    }
};

template <std::size_t Depth>
class Harness {
public:
    explicit Harness(Results& results, const std::string& name)
        : results_(results), name_(name), cycle_(0) {}

    FtqStepOutput step(const FtqStepInput& input) {
        const FtqStepOutput actual = dut_.step(input);
        const FtqStepOutput expected = reference_.step(input);
        std::ostringstream tag;
        tag << name_ << ".cycle" << cycle_++;
        compare_output(results_, actual, expected, tag.str());
        results_.require(actual.count <= Depth, tag.str() + ".count_bound");
        results_.equal(actual.empty, actual.count == 0,
                       tag.str() + ".empty_invariant");
        results_.equal(actual.full, actual.count == Depth,
                       tag.str() + ".full_invariant");
        results_.require(actual.head < Depth, tag.str() + ".head_bound");
        results_.require(actual.tail < Depth, tag.str() + ".tail_bound");
        return actual;
    }

    const ReferenceFtq<Depth>& reference() const { return reference_; }

private:
    Results& results_;
    std::string name_;
    uint64_t cycle_;
    boom::FtqFoundation<Depth> dut_;
    ReferenceFtq<Depth> reference_;
};

FtqStepInput allocation(uint8_t mask, uint64_t serial) {
    FtqStepInput input;
    input.alloc_valid = true;
    input.allocation.packet_base_pc = 0x1000u + serial * 8u;
    input.allocation.packet_valid_mask = mask;
    input.allocation.prediction_valid = (serial & 1u) != 0;
    input.allocation.predicted_taken = true;
    input.allocation.target_valid = true;
    input.allocation.predicted_target = 0x80000000u + serial * 4u;
    input.allocation.cfi_lane = static_cast<uint8_t>(serial);
    input.allocation.cfi_type = static_cast<uint8_t>(serial + 4u);
    input.allocation.predictor_metadata_index = static_cast<uint8_t>(
        serial * 13u + 7u);
    input.allocation.predictor_generation = static_cast<uint32_t>(
        serial * 17u + 11u);
    return input;
}

FtqStepInput lane_event(bool retire, uint8_t index, uint32_t generation,
                        uint8_t lane) {
    FtqStepInput input;
    boom::FtqLaneEvent& event = retire ? input.retire : input.squash;
    event.valid = true;
    event.ftq_idx = index;
    event.generation = generation;
    event.lane = lane;
    return input;
}

FtqStepInput read_event(uint8_t index, uint32_t generation) {
    FtqStepInput input;
    input.read_valid = true;
    input.read_ftq_idx = index;
    input.read_generation = generation;
    return input;
}

template <std::size_t Depth>
void directed_tests(Results& results) {
    std::ostringstream name;
    name << "directed_d" << Depth;
    Harness<Depth> h(results, name.str());

    FtqStepInput reset;
    reset.reset = true;
    FtqStepOutput out = h.step(reset);
    results.require(out.empty && out.head == 0 && out.tail == 0,
                    name.str() + ".reset");

    out = h.step(allocation(0, 1));
    results.require(!out.alloc_accepted && !out.alloc_invalid_mask,
                    name.str() + ".mask00");
    out = h.step(allocation(2, 2));
    results.require(!out.alloc_accepted && out.alloc_invalid_mask,
                    name.str() + ".mask10");
    out = h.step(allocation(7, 3));
    results.require(!out.alloc_accepted && out.alloc_invalid_mask,
                    name.str() + ".mask_high_bits");

    out = h.step(allocation(1, 5));
    const uint8_t first_idx = out.alloc_ftq_idx;
    const uint32_t first_gen = out.alloc_generation;
    results.require(out.alloc_accepted, name.str() + ".mask01");
    out = h.step(read_event(first_idx, first_gen));
    results.require(out.read_hit, name.str() + ".metadata_hit");
    results.equal(out.read_entry.packet_base_pc, UINT64_C(0x1028),
                  name.str() + ".metadata_pc");
    results.equal(out.read_entry.predictor_metadata_index,
                  static_cast<uint8_t>(72), name.str() + ".metadata_index");
    results.equal(out.read_entry.predictor_generation,
                  static_cast<uint32_t>(96),
                  name.str() + ".predictor_generation");
    results.require(out.read_entry.prediction_valid &&
                    out.read_entry.predicted_taken &&
                    out.read_entry.target_valid,
                    name.str() + ".metadata_prediction");
    out = h.step(lane_event(true, first_idx, first_gen, 1));
    results.require(out.retire_rejected && !out.reclaimed,
                    name.str() + ".invalid_lane_rejected");
    out = h.step(lane_event(true, first_idx, first_gen, 0));
    results.require(out.retire_accepted && out.reclaimed,
                    name.str() + ".retire_reclaim");
    out = h.step(lane_event(true, first_idx, first_gen, 0));
    results.require(out.retire_rejected, name.str() + ".stale_rejected");

    out = h.step(allocation(3, 6));
    const uint8_t dual_idx = out.alloc_ftq_idx;
    const uint32_t dual_gen = out.alloc_generation;
    out = h.step(lane_event(true, dual_idx, dual_gen, 0));
    results.require(out.retire_accepted && !out.reclaimed,
                    name.str() + ".partial_retire");
    out = h.step(lane_event(false, dual_idx, dual_gen, 0));
    results.require(out.squash_rejected, name.str() + ".duplicate_lane");
    out = h.step(lane_event(false, dual_idx, dual_gen, 1));
    results.require(out.squash_accepted && out.reclaimed,
                    name.str() + ".retire_squash");

    std::vector<uint8_t> indices;
    std::vector<uint32_t> generations;
    for (std::size_t i = 0; i < Depth; ++i) {
        out = h.step(allocation(1, 100 + i));
        results.require(out.alloc_accepted, name.str() + ".fill");
        indices.push_back(out.alloc_ftq_idx);
        generations.push_back(out.alloc_generation);
    }
    results.require(out.full, name.str() + ".full");
    out = h.step(allocation(1, 500));
    results.require(!out.alloc_ready && !out.alloc_accepted,
                    name.str() + ".full_backpressure");
    out = h.step(lane_event(true, indices[1], generations[1], 0));
    results.require(out.retire_accepted && !out.reclaimed && out.full,
                    name.str() + ".ordered_reclaim");

    FtqStepInput replace = allocation(1, 501);
    replace.retire.valid = true;
    replace.retire.ftq_idx = indices[0];
    replace.retire.generation = generations[0];
    replace.retire.lane = 0;
    out = h.step(replace);
    results.require(out.reclaimed && out.alloc_ready && out.alloc_accepted &&
                    out.full, name.str() + ".alloc_while_reclaim");
    results.equal(out.alloc_ftq_idx, indices[0], name.str() + ".wrapped_slot");

    out = h.step(FtqStepInput());
    results.require(out.reclaimed, name.str() + ".ordered_head_now_reclaims");
    reset.reset = true;
    out = h.step(reset);
    results.require(out.empty, name.str() + ".second_reset");

    std::vector<uint8_t> redirect_idx;
    std::vector<uint32_t> redirect_gen;
    for (std::size_t i = 0; i < 3; ++i) {
        out = h.step(allocation(3, 700 + i));
        redirect_idx.push_back(out.alloc_ftq_idx);
        redirect_gen.push_back(out.alloc_generation);
    }
    FtqStepInput redirect;
    redirect.redirect.valid = true;
    redirect.redirect.owner_ftq_idx = redirect_idx[1];
    redirect.redirect.owner_generation = redirect_gen[1];
    redirect.redirect.surviving_lane_mask = 1;
    redirect.alloc_valid = true;
    redirect.allocation = allocation(1, 900).allocation;
    redirect.retire.valid = true;
    redirect.retire.ftq_idx = redirect_idx[1];
    redirect.retire.generation = redirect_gen[1];
    redirect.retire.lane = 0;
    out = h.step(redirect);
    results.require(out.redirect_accepted && out.retire_rejected &&
                    !out.alloc_ready && !out.alloc_accepted && out.count == 2,
                    name.str() + ".redirect_priority");
    out = h.step(read_event(redirect_idx[1], redirect_gen[1]));
    results.require(out.read_hit && out.read_entry.live_lane_mask == 1,
                    name.str() + ".owner_retained_younger_lane_killed");
    out = h.step(read_event(redirect_idx[2], redirect_gen[2]));
    results.require(!out.read_hit, name.str() + ".younger_entry_killed");
    out = h.step(lane_event(false, redirect_idx[2], redirect_gen[2], 0));
    results.require(out.squash_rejected, name.str() + ".killed_event_stale");
    out = h.step(lane_event(true, redirect_idx[0], redirect_gen[0], 0));
    results.require(out.retire_accepted && !out.reclaimed,
                    name.str() + ".head_lane0");
    out = h.step(lane_event(true, redirect_idx[0], redirect_gen[0], 1));
    results.require(out.retire_accepted && out.reclaimed,
                    name.str() + ".head_lane1");
    out = h.step(lane_event(true, redirect_idx[1], redirect_gen[1], 0));
    results.require(out.retire_accepted && out.reclaimed,
                    name.str() + ".owner_reclaims");

    out = h.step(allocation(3, 1000));
    const uint8_t stale_idx = out.alloc_ftq_idx;
    const uint32_t stale_gen = out.alloc_generation;
    reset.reset = true;
    h.step(reset);
    out = h.step(read_event(stale_idx, stale_gen));
    results.require(!out.read_hit, name.str() + ".reset_stale_read");
    out = h.step(lane_event(true, stale_idx, stale_gen, 0));
    results.require(out.retire_rejected,
                    name.str() + ".reset_stale_event");
}

enum ExhaustiveEvent {
    EX_IDLE,
    EX_RESET,
    EX_ALLOC_00,
    EX_ALLOC_01,
    EX_ALLOC_10,
    EX_ALLOC_11,
    EX_RETIRE_0,
    EX_RETIRE_1,
    EX_SQUASH_0,
    EX_REDIRECT,
    EX_STALE,
    EX_EVENT_COUNT
};

FtqStepInput exhaustive_input(ExhaustiveEvent event,
                              const ReferenceFtq<2>& reference,
                              uint64_t serial) {
    if (event == EX_RESET) {
        FtqStepInput input;
        input.reset = true;
        return input;
    }
    if (event >= EX_ALLOC_00 && event <= EX_ALLOC_11) {
        static const uint8_t masks[] = {0, 1, 2, 3};
        return allocation(masks[event - EX_ALLOC_00], serial);
    }
    if (event == EX_IDLE) return FtqStepInput();

    FtqStepInput input;
    const bool active = !reference.queue().empty();
    const uint8_t index = active ? reference.queue().front().index : 0;
    const uint32_t generation = active ?
        reference.queue().front().entry.generation : 1;
    if (event == EX_REDIRECT) {
        input.redirect.valid = true;
        input.redirect.owner_ftq_idx = index;
        input.redirect.owner_generation = generation;
        input.redirect.surviving_lane_mask = 1;
    } else if (event == EX_STALE) {
        input.retire.valid = true;
        input.retire.ftq_idx = index;
        input.retire.generation = generation + 0x10000u;
        input.retire.lane = 0;
    } else {
        boom::FtqLaneEvent& lane = event == EX_SQUASH_0 ?
            input.squash : input.retire;
        lane.valid = true;
        lane.ftq_idx = index;
        lane.generation = generation;
        lane.lane = event == EX_RETIRE_1 ? 1 : 0;
    }
    return input;
}

struct ExhaustiveStats {
    uint64_t sequences;
    uint64_t errors;
    std::set<std::string> states;

    ExhaustiveStats() : sequences(0), errors(0), states() {}
};

void run_exhaustive_sequence(const std::vector<ExhaustiveEvent>& sequence,
                             Results& results, ExhaustiveStats& stats) {
    const uint64_t before = results.failures;
    Harness<2> harness(results, "exhaustive");
    for (std::size_t i = 0; i < sequence.size(); ++i) {
        const FtqStepInput input = exhaustive_input(
            sequence[i], harness.reference(), i + 1u);
        harness.step(input);
    }
    ++stats.sequences;
    stats.states.insert(harness.reference().signature());
    if (results.failures != before) ++stats.errors;
}

void enumerate_exhaustive(std::vector<ExhaustiveEvent>& sequence,
                          std::size_t target_length, Results& results,
                          ExhaustiveStats& stats) {
    if (sequence.size() == target_length) {
        run_exhaustive_sequence(sequence, results, stats);
        return;
    }
    for (int event = 0; event < EX_EVENT_COUNT; ++event) {
        sequence.push_back(static_cast<ExhaustiveEvent>(event));
        enumerate_exhaustive(sequence, target_length, results, stats);
        sequence.pop_back();
    }
}

}  // namespace

int main() {
    Results results;
    directed_tests<8>(results);
    directed_tests<16>(results);
    directed_tests<32>(results);
    directed_tests<64>(results);

    ExhaustiveStats exhaustive;
    std::vector<ExhaustiveEvent> sequence;
    for (std::size_t length = 0; length <= 6; ++length)
        enumerate_exhaustive(sequence, length, results, exhaustive);

    std::cout << "FTQ_SUMMARY checks=" << results.checks
              << " failures=" << results.failures
              << " exhaustive_states=" << exhaustive.states.size()
              << " exhaustive_sequences=" << exhaustive.sequences
              << " exhaustive_errors=" << exhaustive.errors << '\n';
    return results.failures == 0 ? 0 : 1;
}
