#include <cstdint>
#include <cstdlib>
#include <deque>
#include <iostream>

#include "fetch_buffer.hpp"

namespace {

uint32_t next_random(uint32_t& state) {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

boom::FetchInstruction make_item(uint32_t id, uint32_t random) {
    boom::FetchInstruction item;
    item.pc = (static_cast<uint64_t>(random) << 16) | (id << 1);
    item.instruction = random ^ 0xa5a55a5au;
    item.fetch_id = id;
    item.exception_cause = static_cast<uint64_t>(random) << 3;
    item.is_rvc = (random & 1u) != 0;
    item.exception = (random & 0x20u) != 0;
    return item;
}

bool equal(const boom::FetchInstruction& lhs, const boom::FetchInstruction& rhs) {
    return lhs.pc == rhs.pc && lhs.instruction == rhs.instruction &&
           lhs.fetch_id == rhs.fetch_id && lhs.exception_cause == rhs.exception_cause &&
           lhs.is_rvc == rhs.is_rvc && lhs.exception == rhs.exception;
}

unsigned popcount4(uint8_t mask) {
    return (mask & 1u) + ((mask >> 1) & 1u) + ((mask >> 2) & 1u) +
           ((mask >> 3) & 1u);
}

}  // namespace

int main() {
    uint64_t checks = 0;
    uint64_t enqueues = 0;
    uint64_t dequeues = 0;
    uint64_t backpressure = 0;
    uint64_t flushes = 0;
    uint64_t wraps = 0;

    for (uint32_t seed = 0; seed < 256; ++seed) {
        uint32_t random = 0x9e3779b9u ^ (seed * 0x85ebca6bu);
        boom::FetchBufferState dut;
        std::deque<boom::FetchInstruction> reference;
        uint32_t next_id = seed << 20;
        uint8_t prior_tail = 0;

        for (uint32_t cycle = 0; cycle < 4096; ++cycle) {
            const uint32_t control = next_random(random);
            const bool flush = (control & 0x1ffu) == 0;
            const bool ready = (control & 3u) != 0;
            boom::FetchPacket packet;
            packet.valid = (control & 4u) != 0;
            packet.valid_mask = static_cast<uint8_t>((control >> 4) & 0xfu);
            for (uint8_t lane = 0; lane < FETCH_WIDTH; ++lane) {
                packet.slots[lane] = make_item(next_id + lane, next_random(random));
            }

            const bool expected_valid = !flush && !reference.empty();
            const bool expected_pop = expected_valid && ready;
            const unsigned incoming = packet.valid ? popcount4(packet.valid_mask) : 0;
            const unsigned available = FETCH_BUFFER_DEPTH - reference.size() +
                                       (expected_pop ? 1u : 0u);
            const bool expected_ready = !flush && incoming <= available;
            const bool expected_push = packet.valid && expected_ready;
            const boom::FetchInstruction expected_head = reference.empty()
                ? boom::FetchInstruction() : reference.front();

            const boom::FetchBufferResult result =
                boom::fetch_buffer_step(dut, packet, ready, flush);
            if (result.dequeue_valid != expected_valid ||
                result.dequeue_fire != expected_pop ||
                result.enqueue_ready != expected_ready ||
                result.enqueue_fire != expected_push ||
                (expected_valid && !equal(result.dequeue_bits, expected_head))) {
                std::cerr << "FAIL protocol depth=" << FETCH_BUFFER_DEPTH
                          << " seed=" << seed << " cycle=" << cycle << '\n';
                return 1;
            }
            checks += 5;

            if (flush) {
                reference.clear();
                ++flushes;
            } else {
                if (expected_pop) {
                    reference.pop_front();
                    ++dequeues;
                }
                if (expected_push) {
                    for (uint8_t lane = 0; lane < FETCH_WIDTH; ++lane) {
                        if (((packet.valid_mask >> lane) & 1u) != 0) {
                            reference.push_back(packet.slots[lane]);
                            ++enqueues;
                        }
                    }
                } else if (packet.valid && incoming != 0) {
                    ++backpressure;
                }
            }
            if (dut.tail < prior_tail) ++wraps;
            prior_tail = dut.tail;
            if (dut.count != reference.size() || result.count != reference.size() ||
                result.full != (reference.size() == FETCH_BUFFER_DEPTH) ||
                result.empty != reference.empty() || reference.size() > FETCH_BUFFER_DEPTH) {
                std::cerr << "FAIL state depth=" << FETCH_BUFFER_DEPTH
                          << " seed=" << seed << " cycle=" << cycle << '\n';
                return 1;
            }
            checks += 5;
            next_id += FETCH_WIDTH;
        }
    }

    if (enqueues == 0 || dequeues == 0 || backpressure == 0 || flushes == 0 || wraps == 0) {
        std::cerr << "FAIL incomplete coverage\n";
        return 1;
    }
    std::cout << "GATE5_3_B1_FETCH_BUFFER_RANDOM_PASS depth=" << FETCH_BUFFER_DEPTH
              << " seeds=256 cycles_per_seed=4096 checks=" << checks
              << " enqueues=" << enqueues << " dequeues=" << dequeues
              << " backpressure=" << backpressure << " flushes=" << flushes
              << " wraps=" << wraps << '\n';
    return 0;
}
