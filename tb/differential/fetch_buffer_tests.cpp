#include <cstdint>
#include <cstdlib>
#include <iostream>

#include "fetch_buffer.hpp"

namespace {

unsigned checks = 0;

void require(bool condition, const char* message) {
    ++checks;
    if (!condition) {
        std::cerr << "FAIL depth=" << FETCH_BUFFER_DEPTH << ": " << message << '\n';
        std::exit(1);
    }
}

boom::FetchInstruction item(uint32_t id) {
    boom::FetchInstruction value;
    value.pc = 0x1000u + 2u * id;
    value.instruction = 0x13u ^ (id << 7);
    value.fetch_id = id;
    value.exception_cause = 0x80u + id;
    value.is_rvc = (id & 1u) != 0;
    value.exception = (id % 7u) == 0;
    return value;
}

boom::FetchPacket packet(uint8_t mask, uint32_t base) {
    boom::FetchPacket value;
    value.valid = true;
    value.valid_mask = mask;
    for (uint8_t lane = 0; lane < FETCH_WIDTH; ++lane) {
        value.slots[lane] = item(base + lane);
    }
    return value;
}

void expect_item(const boom::FetchInstruction& actual, uint32_t id) {
    const boom::FetchInstruction expected = item(id);
    require(actual.pc == expected.pc, "PC mismatch");
    require(actual.instruction == expected.instruction, "instruction mismatch");
    require(actual.fetch_id == expected.fetch_id, "fetch ID mismatch");
    require(actual.exception_cause == expected.exception_cause, "cause mismatch");
    require(actual.is_rvc == expected.is_rvc, "RVC metadata mismatch");
    require(actual.exception == expected.exception, "exception mismatch");
}

void test_empty_and_masks() {
    boom::FetchBufferState state;
    boom::FetchPacket none;
    boom::FetchBufferResult r = boom::fetch_buffer_step(state, none, true, false);
    require(!r.dequeue_valid && r.empty && r.count == 0, "reset state must be empty");

    r = boom::fetch_buffer_step(state, packet(0xau, 10), false, false);
    require(r.enqueue_fire && r.count == 2, "sparse mask must enqueue two entries");
    r = boom::fetch_buffer_step(state, none, true, false);
    require(r.dequeue_fire, "first compacted item must dequeue");
    expect_item(r.dequeue_bits, 11);
    r = boom::fetch_buffer_step(state, none, true, false);
    expect_item(r.dequeue_bits, 13);
    require(r.empty, "sparse packet must drain exactly two entries");

    r = boom::fetch_buffer_step(state, packet(0, 20), false, false);
    require(r.enqueue_fire && r.empty, "zero mask packet is an accepted no-op");
}

void test_full_atomic_and_reuse() {
    boom::FetchBufferState state;
    uint32_t id = 0;
    while (state.count < FETCH_BUFFER_DEPTH) {
        boom::FetchBufferResult r = boom::fetch_buffer_step(state, packet(1, id), false, false);
        require(r.enqueue_fire, "single entry fill must be accepted");
        ++id;
    }
    boom::FetchBufferResult r = boom::fetch_buffer_step(state, packet(1, 100), false, false);
    require(!r.enqueue_ready && !r.enqueue_fire && r.full, "full queue must backpressure");

    r = boom::fetch_buffer_step(state, packet(1, 100), true, false);
    require(r.dequeue_fire && r.enqueue_fire && r.full, "dequeue must provide same-cycle capacity");
    expect_item(r.dequeue_bits, 0);
    for (uint32_t expected = 1; expected < id; ++expected) {
        r = boom::fetch_buffer_step(state, boom::FetchPacket(), true, false);
        expect_item(r.dequeue_bits, expected);
    }
    r = boom::fetch_buffer_step(state, boom::FetchPacket(), true, false);
    expect_item(r.dequeue_bits, 100);
    require(r.empty, "replacement entry must remain ordered");

    if (FETCH_BUFFER_DEPTH >= 4) {
        while (state.count < FETCH_BUFFER_DEPTH - 2) {
            boom::fetch_buffer_step(state, packet(1, 200 + state.count), false, false);
        }
        const uint8_t before = state.count;
        r = boom::fetch_buffer_step(state, packet(0xf, 300), false, false);
        require(!r.enqueue_fire && state.count == before, "packet enqueue must be atomic");
    }
}

void test_wrap_flush_and_reset() {
    boom::FetchBufferState state;
    for (uint32_t id = 0; id < FETCH_BUFFER_DEPTH; ++id) {
        boom::fetch_buffer_step(state, packet(1, id), false, false);
    }
    for (uint32_t id = 0; id < FETCH_BUFFER_DEPTH / 2; ++id) {
        const boom::FetchBufferResult r = boom::fetch_buffer_step(
            state, boom::FetchPacket(), true, false);
        expect_item(r.dequeue_bits, id);
    }
    for (uint32_t id = FETCH_BUFFER_DEPTH; id < FETCH_BUFFER_DEPTH + FETCH_BUFFER_DEPTH / 2; ++id) {
        require(boom::fetch_buffer_step(state, packet(1, id), false, false).enqueue_fire,
                "tail wrap enqueue failed");
    }
    for (uint32_t id = FETCH_BUFFER_DEPTH / 2;
         id < FETCH_BUFFER_DEPTH + FETCH_BUFFER_DEPTH / 2; ++id) {
        const boom::FetchBufferResult r = boom::fetch_buffer_step(
            state, boom::FetchPacket(), true, false);
        expect_item(r.dequeue_bits, id);
    }
    require(state.count == 0, "wrapped queue did not drain");

    boom::fetch_buffer_step(state, packet(0xf, 400), false, false);
    boom::FetchBufferResult r = boom::fetch_buffer_step(state, packet(1, 500), true, true);
    require(!r.dequeue_valid && !r.dequeue_fire && !r.enqueue_ready && state.count == 0,
            "flush must dominate enqueue and dequeue");
    boom::fetch_buffer_step(state, packet(1, 600), false, false);
    boom::fetch_buffer_reset(state);
    require(state.head == 0 && state.tail == 0 && state.count == 0,
            "explicit reset must clear control state");
}

}  // namespace

int main() {
    test_empty_and_masks();
    test_full_atomic_and_reuse();
    test_wrap_flush_and_reset();
    std::cout << "GATE5_3_B1_FETCH_BUFFER_DIRECTED_PASS depth=" << FETCH_BUFFER_DEPTH
              << " checks=" << checks << '\n';
    return 0;
}
