#include "predecode.hpp"

#include <cstdint>
#include <cstdio>

static uint64_t state;
static uint32_t random32() {
    state ^= state << 13; state ^= state >> 7; state ^= state << 17;
    return static_cast<uint32_t>(state ^ (state >> 32));
}

static uint32_t branch(uint32_t imm, uint32_t f3) {
    imm &= 0x1fffu;
    return (((imm >> 12) & 1u) << 31) | (((imm >> 5) & 0x3fu) << 25) |
        (2u << 20) | (1u << 15) | (f3 << 12) |
        (((imm >> 1) & 0xfu) << 8) | (((imm >> 11) & 1u) << 7) | 0x63u;
}

static uint32_t make_instruction(uint32_t kind) {
    switch (kind & 3u) {
    case 0: return 0x00100093u ^ ((kind & 31u) << 7);
    case 1: return branch((kind * 62u) & 0x1ffeu, (kind & 1u) ? 1u : 4u);
    case 2: return ((kind * 2046u) & 0xfffff000u) | (1u << 7) | 0x6fu;
    default: return ((kind & 0xfffu) << 20) | (3u << 15) | (5u << 7) | 0x67u;
    }
}

static bool ref_is_cfi(uint32_t inst) {
    const uint32_t op = inst & 0x7fu, f3 = (inst >> 12) & 7u;
    return op == 0x6fu || (op == 0x67u && f3 == 0) ||
           (op == 0x63u && (f3 < 2 || f3 >= 4));
}

int main() {
    uint64_t directed_checks = 0, directed_errors = 0;
    const uint8_t masks[] = {0, 1, 3};
    for (unsigned repeat = 0; repeat < 12; ++repeat) {
        for (unsigned m = 0; m < 3; ++m) {
            for (unsigned scenario = 0; scenario < 4; ++scenario) {
                const uint32_t lane0 = (scenario & 1u) ? make_instruction(1 + repeat * 4) : 0x00100093u;
                const uint32_t lane1 = (scenario & 2u) ? make_instruction(2 + repeat * 4) : 0x00200113u;
                const boom::CfiPacketPredecodeResult r = boom::predecode_cfi_packet(
                    masks[m], 0x1000, lane0, (repeat & 1u) != 0,
                    0x1004, lane1, (repeat & 2u) != 0);
                const bool lane0_cfi = (masks[m] & 1u) && ref_is_cfi(lane0);
                const bool lane1_cfi = (masks[m] & 2u) && ref_is_cfi(lane1);
                const bool has = lane0_cfi || lane1_cfi;
                const uint8_t selected = lane0_cfi ? 0 : 1;
                const uint8_t younger = lane0_cfi ? (masks[m] & 2u) : 0;
                const uint8_t effective = has ? (masks[m] & ~younger) : masks[m];
                const bool ok = r.packet_has_cfi == has &&
                    (!has || (r.selected_cfi_lane == selected &&
                              r.younger_lane_mask == younger)) &&
                    boom::mask_younger_packet_lanes(masks[m], r) == effective;
                ++directed_checks;
                if (!ok) ++directed_errors;
            }
        }
    }

    uint64_t classification_error = 0, target_error = 0;
    uint64_t lane_selection_error = 0, younger_mask_error = 0, false_positive = 0;
    const unsigned seeds = 256, packets_per_seed = 4096;
    for (unsigned seed = 0; seed < seeds; ++seed) {
        state = 0x5041434b45540000ull ^ (static_cast<uint64_t>(seed) << 1);
        for (unsigned packet = 0; packet < packets_per_seed; ++packet) {
            const uint8_t mask_values[] = {0, 1, 3};
            const uint8_t mask = mask_values[random32() % 3];
            const uint32_t i0 = make_instruction(random32());
            const uint32_t i1 = make_instruction(random32());
            const uint64_t pc0 = (static_cast<uint64_t>(random32()) << 32) | random32();
            const uint64_t pc1 = pc0 + ((random32() & 1u) ? 2u : 4u);
            const bool rvc0 = (random32() & 1u) != 0, rvc1 = (random32() & 1u) != 0;
            const boom::CfiPacketPredecodeResult got = boom::predecode_cfi_packet(
                mask, pc0, i0, rvc0, pc1, i1, rvc1);
            const bool cfi0 = (mask & 1u) && ref_is_cfi(i0);
            const bool cfi1 = (mask & 2u) && ref_is_cfi(i1);
            const bool has = cfi0 || cfi1;
            if (got.packet_has_cfi != has) ++classification_error;
            if (!has && got.packet_has_cfi) ++false_positive;
            if (has) {
                const uint8_t lane = cfi0 ? 0 : 1;
                if (got.selected_cfi_lane != lane) ++lane_selection_error;
                const uint8_t younger = cfi0 ? (mask & 2u) : 0;
                if (got.younger_lane_mask != younger ||
                    boom::mask_younger_packet_lanes(mask, got) != (mask & ~younger))
                    ++younger_mask_error;
                const boom::CfiPredecodeResult single = boom::predecode_cfi(
                    lane ? pc1 : pc0, lane ? i1 : i0, lane ? rvc1 : rvc0);
                if (single.static_target_valid &&
                    single.static_target != got.selected_cfi_result.static_target)
                    ++target_error;
            }
        }
    }
    const uint64_t errors = directed_errors + classification_error + target_error +
        lane_selection_error + younger_mask_error + false_positive;
    std::printf("PREDECODE_PACKET_DIRECTED,checks=%llu,errors=%llu\n",
                static_cast<unsigned long long>(directed_checks),
                static_cast<unsigned long long>(directed_errors));
    std::printf("PREDECODE_PACKET_RANDOM,seeds=%u,packets_per_seed=%u,classification_error=%llu,target_error=%llu,lane_selection_error=%llu,younger_mask_error=%llu,false_positive=%llu,total_error=%llu\n",
                seeds, packets_per_seed,
                static_cast<unsigned long long>(classification_error),
                static_cast<unsigned long long>(target_error),
                static_cast<unsigned long long>(lane_selection_error),
                static_cast<unsigned long long>(younger_mask_error),
                static_cast<unsigned long long>(false_positive),
                static_cast<unsigned long long>(errors));
    if (directed_checks < 100 || errors != 0) return 1;
    std::printf("GATE5_4_P1_PREDECODE_PACKET_PASS\n");
    return 0;
}
