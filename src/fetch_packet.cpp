#include "fetch_packet.hpp"

#include "rvc.hpp"

namespace boom {
namespace {

static_assert(FETCH_PACKET_WIDTH == 2, "canonical response packet width must be two");

FetchInstruction fetch_fault(uint64_t pc, uint16_t original, uint64_t cause,
                             bool access_fault) {
    FetchInstruction entry;
    entry.pc = pc;
    entry.original_instruction = original;
    entry.exception = true;
    entry.exception_cause = cause;
    entry.exception_access_fault = access_fault;
    entry.exception_misaligned = !access_fault && cause == 0;
    return entry;
}

FetchInstruction base_instruction(uint64_t pc, uint32_t instruction,
                                   uint32_t fetch_id) {
    FetchInstruction entry;
    entry.pc = pc;
    entry.instruction = instruction;
    entry.fetch_id = fetch_id;
    return entry;
}

FetchInstruction compressed_instruction(uint64_t pc, uint16_t parcel,
                                         uint32_t fetch_id) {
    const RvcDecodeResult decoded = decompress_rvc(parcel);
    FetchInstruction entry;
    entry.pc = pc;
    entry.original_instruction = parcel;
    entry.fetch_id = fetch_id;
    entry.is_rvc = true;
    if (decoded.legal) {
        entry.instruction = decoded.instruction;
    } else {
        entry.exception = true;
        entry.exception_cause = 2;
    }
    return entry;
}

void append(FetchPacketResult& result, const FetchInstruction& entry) {
    const uint8_t lane = result.packet.valid_mask == 0 ? 0 : 1;
    result.packet.slots[lane] = entry;
    result.packet.valid_mask = lane == 0 ? 1 : 3;
    result.packet.valid = true;
}

bool append_parcel(FetchPacketResult& result, uint64_t pc, uint16_t parcel,
                    uint32_t fetch_id) {
    if ((parcel & 0x3u) != 0x3u) {
        const FetchInstruction entry = compressed_instruction(pc, parcel, fetch_id);
        append(result, entry);
        return entry.exception;
    }
    if ((parcel & 0x1fu) == 0x1fu) {
        FetchInstruction entry = fetch_fault(pc, parcel, 2, false);
        entry.fetch_id = fetch_id;
        append(result, entry);
        return true;
    }
    return false;
}

}  // namespace

FetchPacketResult build_fetch_packet(const FetchPacketInput& input) {
    FetchPacketResult result;
    result.next_pc = input.pc;

    const uint64_t first_pc = input.carry_valid ? input.carry_pc : input.pc;
    if (input.exception) {
        FetchInstruction entry = fetch_fault(first_pc, 0, input.exception_cause, true);
        entry.fetch_id = input.fetch_id;
        append(result, entry);
        result.next_pc = first_pc + 4;
        return result;
    }

    if (input.carry_valid) {
        const uint32_t instruction =
            (input.instruction << 16) | static_cast<uint32_t>(input.carry);
        append(result, base_instruction(input.carry_pc, instruction, input.fetch_id));
        const uint64_t upper_pc = input.carry_pc + 4;
        const uint16_t upper = static_cast<uint16_t>(input.instruction >> 16);
        result.next_pc = upper_pc + 2;
        if ((upper & 0x3u) == 0x3u && (upper & 0x1fu) != 0x1fu) {
            result.carry_valid = true;
            result.carry = upper;
            result.carry_pc = upper_pc;
            result.next_pc = upper_pc;
        } else {
            append_parcel(result, upper_pc, upper, input.fetch_id);
        }
        return result;
    }

    const bool start_upper = (input.pc & 0x2ULL) != 0;
    const uint16_t first = start_upper ?
        static_cast<uint16_t>(input.instruction >> 16) :
        static_cast<uint16_t>(input.instruction);
    if ((first & 0x3u) == 0x3u && (first & 0x1fu) != 0x1fu) {
        if (!start_upper) {
            append(result, base_instruction(input.pc, input.instruction, input.fetch_id));
            result.next_pc = input.pc + 4;
        } else {
            result.carry_valid = true;
            result.carry = first;
            result.carry_pc = input.pc;
        }
        return result;
    }

    const bool terminal = append_parcel(result, input.pc, first, input.fetch_id);
    result.next_pc = input.pc + 2;
    if (terminal || start_upper) return result;

    const uint64_t upper_pc = input.pc + 2;
    const uint16_t upper = static_cast<uint16_t>(input.instruction >> 16);
    if ((upper & 0x3u) == 0x3u && (upper & 0x1fu) != 0x1fu) {
        result.carry_valid = true;
        result.carry = upper;
        result.carry_pc = upper_pc;
        return result;
    }
    append_parcel(result, upper_pc, upper, input.fetch_id);
    result.next_pc = upper_pc + 2;
    return result;
}

}  // namespace boom
