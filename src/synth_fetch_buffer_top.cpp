#include "fetch_buffer.hpp"

void synth_fetch_buffer_top(
    bool runtime_reset, bool flush, bool dequeue_ready,
    bool packet_valid, uint8_t valid_mask,
    uint64_t pc0, uint32_t instruction0, uint32_t fetch_id0, uint64_t cause0,
    bool is_rvc0, bool exception0,
    uint64_t pc1, uint32_t instruction1, uint32_t fetch_id1, uint64_t cause1,
    bool is_rvc1, bool exception1,
    uint64_t pc2, uint32_t instruction2, uint32_t fetch_id2, uint64_t cause2,
    bool is_rvc2, bool exception2,
    uint64_t pc3, uint32_t instruction3, uint32_t fetch_id3, uint64_t cause3,
    bool is_rvc3, bool exception3,
    bool& enqueue_ready, bool& enqueue_fire,
    bool& dequeue_valid, bool& dequeue_fire,
    uint64_t& dequeue_pc, uint32_t& dequeue_instruction,
    uint32_t& dequeue_fetch_id, uint64_t& dequeue_cause,
    bool& dequeue_is_rvc, bool& dequeue_exception,
    bool& full, bool& empty, uint8_t& count) {
    static boom::FetchBufferState state;
    boom::FetchPacket packet;
    packet.valid = packet_valid;
    packet.valid_mask = valid_mask;

    packet.slots[0].pc = pc0;
    packet.slots[0].instruction = instruction0;
    packet.slots[0].fetch_id = fetch_id0;
    packet.slots[0].exception_cause = cause0;
    packet.slots[0].is_rvc = is_rvc0;
    packet.slots[0].exception = exception0;
    packet.slots[1].pc = pc1;
    packet.slots[1].instruction = instruction1;
    packet.slots[1].fetch_id = fetch_id1;
    packet.slots[1].exception_cause = cause1;
    packet.slots[1].is_rvc = is_rvc1;
    packet.slots[1].exception = exception1;
    packet.slots[2].pc = pc2;
    packet.slots[2].instruction = instruction2;
    packet.slots[2].fetch_id = fetch_id2;
    packet.slots[2].exception_cause = cause2;
    packet.slots[2].is_rvc = is_rvc2;
    packet.slots[2].exception = exception2;
    packet.slots[3].pc = pc3;
    packet.slots[3].instruction = instruction3;
    packet.slots[3].fetch_id = fetch_id3;
    packet.slots[3].exception_cause = cause3;
    packet.slots[3].is_rvc = is_rvc3;
    packet.slots[3].exception = exception3;

    boom::FetchBufferResult result;
    if (runtime_reset) {
        boom::fetch_buffer_reset(state);
        result.count = 0;
        result.empty = true;
    } else {
        result = boom::fetch_buffer_step(state, packet, dequeue_ready, flush);
    }

    enqueue_ready = result.enqueue_ready;
    enqueue_fire = result.enqueue_fire;
    dequeue_valid = result.dequeue_valid;
    dequeue_fire = result.dequeue_fire;
    dequeue_pc = result.dequeue_bits.pc;
    dequeue_instruction = result.dequeue_bits.instruction;
    dequeue_fetch_id = result.dequeue_bits.fetch_id;
    dequeue_cause = result.dequeue_bits.exception_cause;
    dequeue_is_rvc = result.dequeue_bits.is_rvc;
    dequeue_exception = result.dequeue_bits.exception;
    full = result.full;
    empty = result.empty;
    count = result.count;
}
