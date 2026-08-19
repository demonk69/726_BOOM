#ifndef FETCH_PACKET_HPP
#define FETCH_PACKET_HPP

#include <cstdint>

#include "fetch_buffer.hpp"

namespace boom {

struct FetchPacketInput {
    uint64_t pc;
    uint32_t instruction;
    uint32_t fetch_id;
    bool exception;
    uint64_t exception_cause;
    bool carry_valid;
    uint16_t carry;
    uint64_t carry_pc;

    FetchPacketInput()
        : pc(0), instruction(0), fetch_id(0), exception(false),
          exception_cause(0), carry_valid(false), carry(0), carry_pc(0) {}
};

struct FetchPacketResult {
    FetchPacket packet;
    uint64_t next_pc;
    bool carry_valid;
    uint16_t carry;
    uint64_t carry_pc;

    FetchPacketResult()
        : packet(), next_pc(0), carry_valid(false), carry(0), carry_pc(0) {}
};

// Pure canonical constructor for one matched response and, optionally, its
// immediately preceding cross-word parcel.
FetchPacketResult build_fetch_packet(const FetchPacketInput& input);

}  // namespace boom

#endif
