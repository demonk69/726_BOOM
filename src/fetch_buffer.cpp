#include "fetch_buffer.hpp"

namespace boom {
namespace {

uint8_t wrap_index(uint8_t index, uint8_t increment) {
#pragma HLS INLINE
    return static_cast<uint8_t>((index + increment) & (FETCH_BUFFER_DEPTH - 1));
}

uint8_t packet_count(uint8_t mask) {
#pragma HLS INLINE
    uint8_t count = 0;
    for (uint8_t lane = 0; lane < FETCH_WIDTH; ++lane) {
#pragma HLS UNROLL
        count = static_cast<uint8_t>(count + ((mask >> lane) & 1u));
    }
    return count;
}

}  // namespace

void fetch_buffer_reset(FetchBufferState& state) {
#pragma HLS INLINE
    state.head = 0;
    state.tail = 0;
    state.count = 0;
#ifdef FETCH_BUFFER_RESET_PAYLOAD
    for (uint8_t i = 0; i < FETCH_BUFFER_DEPTH; ++i) {
        state.entries[i] = FetchInstruction();
    }
#endif
}

FetchBufferResult fetch_buffer_step(FetchBufferState& state,
                                    const FetchPacket& packet,
                                    bool dequeue_ready,
                                    bool flush) {
#pragma HLS INLINE
#if defined(FETCH_BUFFER_STORAGE_LUTRAM)
#pragma HLS BIND_STORAGE variable=state.entries type=ram_1p impl=lutram
#elif defined(FETCH_BUFFER_STORAGE_BRAM)
#pragma HLS BIND_STORAGE variable=state.entries type=ram_1p impl=bram
#endif
    FetchBufferResult result;
    const bool dequeue_valid = state.count != 0;
    const bool dequeue_fire = dequeue_valid && dequeue_ready && !flush;
    const uint8_t mask = static_cast<uint8_t>(packet.valid_mask &
                                               ((1u << FETCH_WIDTH) - 1u));
    const uint8_t incoming = packet.valid ? packet_count(mask) : 0;
    const uint8_t available = static_cast<uint8_t>(FETCH_BUFFER_DEPTH - state.count +
                                                    (dequeue_fire ? 1 : 0));

    result.dequeue_valid = dequeue_valid && !flush;
    if (dequeue_valid) {
        result.dequeue_bits = state.entries[state.head];
    }
    result.dequeue_fire = dequeue_fire;
    result.enqueue_ready = !flush && incoming <= available;
    result.enqueue_fire = packet.valid && result.enqueue_ready;

    if (flush) {
        fetch_buffer_reset(state);
    } else {
        if (dequeue_fire) {
            state.head = wrap_index(state.head, 1);
        }
        if (result.enqueue_fire) {
            uint8_t packed_lane = 0;
            for (uint8_t lane = 0; lane < FETCH_WIDTH; ++lane) {
#pragma HLS UNROLL
                if (((mask >> lane) & 1u) != 0) {
                    state.entries[wrap_index(state.tail, packed_lane)] = packet.slots[lane];
                    ++packed_lane;
                }
            }
            state.tail = wrap_index(state.tail, incoming);
        }
        state.count = static_cast<uint8_t>(state.count - (dequeue_fire ? 1 : 0) +
                                           (result.enqueue_fire ? incoming : 0));
    }

    result.count = state.count;
    result.full = state.count == FETCH_BUFFER_DEPTH;
    result.empty = state.count == 0;
    return result;
}

}  // namespace boom
