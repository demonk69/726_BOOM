#include <cstdint>
#include <cstdio>
#include <cstdlib>

extern "C" void g6_l1r80_queue_allocate(
    uint8_t, uint8_t, uint8_t, uint8_t, uint16_t, uint32_t, uint8_t, uint64_t,
    uint64_t&, uint64_t&, uint64_t&, uint64_t&, uint64_t&, uint64_t&, uint64_t&, uint64_t&);
extern "C" void g6_l1r80_queue_lifecycle(
    uint8_t, uint8_t, uint8_t, uint16_t, uint8_t, uint32_t, uint32_t, uint8_t,
    uint64_t&, uint64_t&, uint64_t&, uint64_t&, uint64_t&, uint64_t&, uint64_t&, uint64_t&);
extern "C" void g6_l1r80_load_response(
    uint8_t, uint8_t, uint16_t, uint32_t, uint32_t, uint32_t, uint32_t,
    uint32_t, uint32_t, uint32_t, uint32_t, uint16_t, uint8_t, uint8_t,
    uint8_t, uint8_t, uint8_t, uint8_t, uint64_t, uint64_t,
    uint64_t&, uint64_t&, uint64_t&, uint64_t&, uint64_t&, uint64_t&, uint64_t&, uint64_t&);
extern "C" void g6_l1r80_branch_recovery(
    uint8_t, uint8_t, uint8_t, uint16_t, uint32_t, uint32_t, uint8_t,
    uint8_t, uint8_t, uint8_t, uint8_t,
    uint64_t&, uint64_t&, uint64_t&, uint64_t&, uint64_t&, uint64_t&, uint64_t&, uint64_t&);
extern "C" void g6_l1r80_global_flush(
    uint8_t, uint8_t, uint16_t, uint32_t, uint32_t, uint8_t,
    uint64_t&, uint64_t&, uint64_t&, uint64_t&, uint64_t&, uint64_t&, uint64_t&, uint64_t&);

struct Observations {
    uint64_t value[8];
};

static void require_case(bool condition, int id, const char* requirement) {
    if (condition) return;
    std::fprintf(stderr, "G6_L1R80_NATIVE_FAIL id=%d requirement=%s\n",
                 id, requirement);
    std::exit(EXIT_FAILURE);
}

static void validate(int id, const Observations& o) {
    const uint64_t seeded_prf = 0xaaaaaaaaaaaaaaaaULL;
    switch (id) {
    case 0: require_case(o.value[0] == 0 && o.value[5] == 0, id, "queues_empty"); break;
    case 1: require_case(o.value[4] == 0x101 && o.value[5] == 1, id, "lq_single"); break;
    case 2: require_case(o.value[4] == 0x101 && o.value[5] == 0x100, id, "sq_single"); break;
    case 3: require_case((o.value[1] & 1) == 0 && o.value[4] == 0x808 && o.value[5] == 8, id, "lq_full"); break;
    case 4: require_case((o.value[1] & 1) == 0 && o.value[4] == 0x808 && o.value[5] == 0x800, id, "sq_full"); break;
    case 5: require_case((o.value[2] & 0xff) == 7 && (o.value[3] & 0xff) == 0 && o.value[5] == 2, id, "lq_wrap"); break;
    case 6: require_case((o.value[2] & 0xff) == 7 && (o.value[3] & 0xff) == 0 && o.value[5] == 0x200, id, "sq_wrap"); break;
    case 7: require_case(o.value[0] == 0xd && o.value[1] == 0x1000101 && o.value[5] == 1 && (o.value[3] >> 8) == 2, id, "lq_reuse"); break;
    case 8: require_case(o.value[0] == 0xd && o.value[1] == 0x1000101 && o.value[5] == 0x100 && (o.value[3] >> 8) == 2, id, "sq_reuse"); break;
    case 9: case 10: require_case((o.value[2] >> 8) == 0xffff, id, "generation_ffff"); break;
    case 11: case 12: require_case((o.value[2] >> 8) == 0, id, "generation_wrap"); break;
    case 14: case 15: case 42: case 46:
        require_case(o.value[4] == 1 && o.value[5] == 0, id, "sq_owner_identity"); break;
    case 16: require_case(o.value[0] == 7 && o.value[4] == 0 && o.value[2] != seeded_prf, id, "correct_response"); break;
    case 23: require_case(o.value[0] == 0 && o.value[4] == 0x101 && o.value[2] == seeded_prf, id, "lq_reuse_stale_response"); break;
    case 30: require_case(o.value[0] == 1 && o.value[2] == 0 && o.value[3] == 1, id, "branch_clear_lq"); break;
    case 31: require_case(o.value[0] == 2 && o.value[2] == 0 && o.value[3] == 2, id, "branch_clear_sq"); break;
    case 32: case 33: case 34: case 35:
        require_case(o.value[0] == 0 && o.value[1] == 0 && o.value[3] == 0 && o.value[7] == seeded_prf, id, "branch_squash"); break;
    case 37: require_case(o.value[0] == 0 && o.value[1] == 0 && o.value[2] == 0, id, "global_flush"); break;
    case 41: require_case(o.value[1] == 0x1010201 && o.value[5] == 1, id, "lq_count_lifecycle"); break;
    case 43: require_case((o.value[1] >> 24) == 8 && o.value[5] == 8, id, "lq_full_free_allocate"); break;
    case 44: require_case((o.value[1] >> 24) == 8 && o.value[5] == 0x800, id, "sq_full_free_allocate"); break;
    default:
        if ((id >= 17 && id <= 22) || id == 24 || id == 25 || id == 45)
            require_case(o.value[0] == 0 && o.value[2] == seeded_prf && o.value[3] == seeded_prf,
                         id, "response_identity_reject");
        if (id >= 50) {
            switch ((id - 50) % 6) {
            case 0: require_case(o.value[0] == 7 && o.value[4] == 0, id, "variant_load_response"); break;
            case 1: require_case(o.value[4] == 1, id, "variant_store_reclaim"); break;
            case 2: require_case(o.value[0] == 0x38 && o.value[2] == seeded_prf && o.value[3] != seeded_prf, id, "variant_stale_then_response"); break;
            case 3: require_case(o.value[0] == 0 && o.value[1] == 0 && o.value[2] == 0, id, "variant_allocate_flush"); break;
            case 4: require_case((o.value[0] & 1) == 0 && o.value[1] == 0 && o.value[7] == seeded_prf, id, "variant_allocate_squash"); break;
            case 5: require_case((o.value[1] >> 24) == 8 && o.value[5] == 8, id, "variant_full_free_allocate"); break;
            }
        }
        break;
    }
}

static void emit(int id, const char* family, const Observations& obs) {
    validate(id, obs);
    std::printf("%d,%s", id, family);
    for (int i = 0; i < 8; ++i)
        std::printf(",%016llx", (unsigned long long)obs.value[i]);
    std::printf("\n");
}

static void queue_allocate(int id, uint8_t kind, uint8_t tail,
                           uint8_t count, uint8_t allocations,
                           uint16_t generation) {
    Observations o = {};
    g6_l1r80_queue_allocate(kind, tail, count, allocations, generation,
        0x10000000u + (uint32_t)id * 0x100u, 0xff,
        0x1000u + (uint64_t)id * 16u,
        o.value[0], o.value[1], o.value[2], o.value[3],
        o.value[4], o.value[5], o.value[6], o.value[7]);
    emit(id, "queue_allocate", o);
}

static void queue_lifecycle(int id, uint8_t kind, uint8_t count,
                            uint8_t slot, uint16_t generation,
                            uint8_t phases, uint32_t transaction,
                            uint8_t mask) {
    Observations o = {};
    g6_l1r80_queue_lifecycle(kind, count, slot, generation, phases,
        0x20000000u + (uint32_t)id * 0x100u, transaction, mask,
        o.value[0], o.value[1], o.value[2], o.value[3],
        o.value[4], o.value[5], o.value[6], o.value[7]);
    emit(id, "queue_lifecycle", o);
}

static void load_response(int id, uint8_t slot, uint16_t generation,
                          uint32_t transaction, uint8_t mask, uint8_t mode) {
    Observations o = {};
    const uint8_t rob = 5;
    const uint32_t allocation = 0x30000000u + (uint32_t)id * 0x100u;
    uint32_t live = allocation;
    uint32_t pending = allocation;
    uint32_t owner = allocation;
    uint32_t pending_tx = transaction;
    uint32_t owner_tx = transaction;
    uint32_t first_tx = transaction;
    uint32_t second_tx = transaction + 1;
    uint16_t pending_generation = generation;
    uint8_t pending_rob = rob;
    uint8_t owner_rob = rob;
    uint8_t flags = 0x77;

    switch (id) {
    case 17: first_tx = second_tx = transaction + 1; break;
    case 18: pending_rob = rob + 1; break;
    case 19: pending = allocation + 1; break;
    case 20: pending_generation = (uint16_t)(generation + 1); break;
    case 21: owner_tx = transaction + 1; break;
    case 22: flags = 0x57; break;
    case 23: live = allocation + 1; mode = 1; break;
    case 24: live = allocation + 1; mode = 2; break;
    case 25: first_tx = second_tx = transaction + 1; break;
    case 45: owner_rob = rob + 1; break;
    default: break;
    }

    g6_l1r80_load_response(rob, slot, generation, live, pending, owner,
        transaction, pending_tx, owner_tx, first_tx, second_tx,
        pending_generation, pending_rob, owner_rob, flags, 5, mask, mode,
        0x1122334455660000ULL + (uint64_t)id,
        0x4000u + (uint64_t)id * 8u,
        o.value[0], o.value[1], o.value[2], o.value[3],
        o.value[4], o.value[5], o.value[6], o.value[7]);
    emit(id, "load_response", o);
}

static void branch_recovery(int id, uint8_t tag, uint8_t slot,
                            uint16_t generation, uint32_t transaction,
                            uint8_t valids, uint8_t lq_mask,
                            uint8_t sq_mask, uint8_t memory_mask,
                            bool mispredict) {
    Observations o = {};
    g6_l1r80_branch_recovery(tag, slot, slot, generation,
        0x40000000u + (uint32_t)id * 0x100u, transaction, valids,
        lq_mask, sq_mask, memory_mask, mispredict,
        o.value[0], o.value[1], o.value[2], o.value[3],
        o.value[4], o.value[5], o.value[6], o.value[7]);
    emit(id, "branch_recovery", o);
}

static void global_flush(int id, uint8_t slot, uint16_t generation,
                         uint32_t transaction, uint8_t mask) {
    Observations o = {};
    g6_l1r80_global_flush(slot, slot, generation,
        0x50000000u + (uint32_t)id * 0x100u, transaction, mask,
        o.value[0], o.value[1], o.value[2], o.value[3],
        o.value[4], o.value[5], o.value[6], o.value[7]);
    emit(id, "global_flush", o);
}

static uint8_t first_set_bit(uint8_t mask) {
    for (uint8_t bit = 0; bit < 8; ++bit)
        if ((mask & (uint8_t)(1u << bit)) != 0) return bit;
    return 0;
}

static void run_variant(int id) {
    static const uint8_t slots[30] = {
        2,3,4,5,6,7,0,1,2,3,4,5,6,7,0,1,2,3,4,5,6,7,0,1,2,3,4,5,6,7
    };
    static const uint16_t generations[30] = {
        0,1,0x7fff,0xfffd,0xfffe,0xffff,
        0,1,0x7fff,0xfffd,0xfffe,0xffff,
        0,1,0x7fff,0xfffd,0xfffe,0xffff,
        0,1,0x7fff,0xfffd,0xfffe,0xffff,
        0,1,0x7fff,0xfffd,0xfffe,0xffff
    };
    static const uint8_t masks[30] = {
        0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80,0x3f,0x1f,
        0x0f,0x07,0x03,0x01,0x00,0x00,0xb5,0xb4,0xb7,0xb6,
        0xb1,0xb0,0xb3,0xb2,0xbd,0xbc,0xbf,0xbe,0xb9,0xb8
    };
    static const uint32_t transactions[30] = {
        0x00000001,0x00010102,0x00020203,0x00030304,0x00040405,
        0x00050506,0x00060607,0x00070708,0x00080809,0x0009090a,
        0x000a0a0b,0x000b0b0c,0x000c0c0d,0x000d0d0e,0x000e0e0f,
        0x000f0f10,0x00101011,0x00111112,0x00121213,0x00131314,
        0x00141415,0x00151516,0x00161617,0x00171718,0x00181819,
        0x0019191a,0x001a1a1b,0x001b1b1c,0x001c1c1d,0x001d1d1e
    };
    const int index = id - 50;
    const uint8_t slot = slots[index];
    const uint16_t generation = generations[index];
    const uint8_t mask = masks[index];
    const uint32_t transaction = transactions[index];
    switch (index % 6) {
    case 0:
        load_response(id, slot, generation, transaction, mask, 0);
        break;
    case 1:
        queue_lifecycle(id, 1, slot, slot, generation, 0x01,
                        transaction, mask);
        break;
    case 2: {
        Observations o = {};
        const uint32_t allocation = 0x30000000u + (uint32_t)id * 0x100u;
        g6_l1r80_load_response(5, slot, generation, allocation, allocation,
            allocation, transaction, transaction, transaction,
            transaction + 1, transaction, generation, 5, 5, 0x77, 5,
            mask, 0, 0x1122334455660000ULL + (uint64_t)id,
            0x4000u + (uint64_t)id * 8u,
            o.value[0], o.value[1], o.value[2], o.value[3],
            o.value[4], o.value[5], o.value[6], o.value[7]);
        emit(id, "load_response", o);
        break;
    }
    case 3:
        global_flush(id, slot, generation, transaction, mask);
        break;
    case 4:
        branch_recovery(id, first_set_bit(mask), slot, generation,
                        transaction, 1, mask, 0, mask, true);
        break;
    case 5:
        queue_lifecycle(id, 0, 8, slot, generation, 0x0c,
                        transaction, mask);
        break;
    }
}

int main() {
    std::printf("case_id,family,obs0,obs1,obs2,obs3,obs4,obs5,obs6,obs7\n");
    queue_allocate(0, 0, 0, 0, 0, 0);
    queue_allocate(1, 0, 0, 0, 1, 0);
    queue_allocate(2, 1, 0, 0, 1, 0);
    queue_allocate(3, 0, 0, 8, 1, 0);
    queue_allocate(4, 1, 0, 8, 1, 0);
    queue_allocate(5, 0, 7, 0, 2, 0);
    queue_allocate(6, 1, 7, 0, 2, 0);
    queue_lifecycle(7, 0, 0, 0, 0, 0x05, 7, 0xff);
    queue_lifecycle(8, 1, 0, 0, 0, 0x05, 8, 0xff);
    queue_allocate(9, 0, 0, 0, 1, 0xfffe);
    queue_allocate(10, 1, 1, 0, 1, 0xfffe);
    queue_allocate(11, 0, 2, 0, 1, 0xffff);
    queue_allocate(12, 1, 3, 0, 1, 0xffff);
    queue_lifecycle(14, 1, 0, 0, 0, 0x01, 14, 0xff);
    queue_lifecycle(15, 1, 0, 0, 0, 0x01, 15, 0xff);
    for (int id = 16; id <= 25; ++id)
        load_response(id, 2, 7, 0x1000u + (uint32_t)id, 0xff, 0);
    branch_recovery(30, 0, 2, 7, 30, 1, 1, 0, 0xff, false);
    branch_recovery(31, 0, 2, 7, 31, 2, 0, 1, 0xff, false);
    branch_recovery(32, 0, 2, 7, 32, 1, 1, 0, 0xff, true);
    branch_recovery(33, 0, 2, 7, 33, 2, 0, 1, 0xff, true);
    branch_recovery(34, 0, 2, 7, 34, 5, 1, 0, 0xff, true);
    branch_recovery(35, 0, 2, 7, 35, 5, 1, 0, 0xff, true);
    global_flush(37, 2, 7, 37, 0xff);
    queue_lifecycle(41, 0, 0, 0, 0, 0x03, 41, 0xff);
    queue_lifecycle(42, 1, 0, 0, 0, 0x01, 42, 0xff);
    queue_lifecycle(43, 0, 8, 0, 0, 0x0c, 43, 0xff);
    queue_lifecycle(44, 1, 8, 0, 0, 0x0c, 44, 0xff);
    load_response(45, 2, 7, 45, 0xff, 0);
    queue_lifecycle(46, 1, 0, 0, 0, 0x01, 46, 0xff);
    for (int id = 50; id < 80; ++id) run_variant(id);
    return EXIT_SUCCESS;
}
