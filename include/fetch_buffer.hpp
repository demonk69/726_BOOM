#ifndef FETCH_BUFFER_HPP
#define FETCH_BUFFER_HPP

#include <cstdint>

#include "boom_config.hpp"

namespace boom {

struct FetchInstruction {
    uint64_t pc;
    uint32_t instruction;
    uint32_t original_instruction;
    uint32_t fetch_id;
    uint64_t exception_cause;
    bool is_rvc;
    bool exception;
    bool exception_access_fault;
    bool exception_misaligned;

    FetchInstruction()
        : pc(0), instruction(0), original_instruction(0), fetch_id(0),
          exception_cause(0), is_rvc(false), exception(false),
          exception_access_fault(false), exception_misaligned(false) {}
};

struct FetchPacket {
    bool valid;
    uint8_t valid_mask;
    FetchInstruction slots[FETCH_WIDTH];

    FetchPacket() : valid(false), valid_mask(0), slots() {}
};

struct FetchBufferState {
    FetchInstruction entries[FETCH_BUFFER_DEPTH];
    uint8_t head;
    uint8_t tail;
    uint8_t count;

    FetchBufferState() : entries(), head(0), tail(0), count(0) {}
};

struct FetchBufferResult {
    bool enqueue_ready;
    bool enqueue_fire;
    bool dequeue_valid;
    bool dequeue_fire;
    bool full;
    bool empty;
    uint8_t count;
    FetchInstruction dequeue_bits;

    FetchBufferResult()
        : enqueue_ready(false), enqueue_fire(false), dequeue_valid(false),
          dequeue_fire(false), full(false), empty(true), count(0),
          dequeue_bits() {}
};

void fetch_buffer_reset(FetchBufferState& state);

FetchBufferResult fetch_buffer_step(FetchBufferState& state,
                                    const FetchPacket& packet,
                                    bool dequeue_ready,
                                    bool flush);

}  // namespace boom

#endif
