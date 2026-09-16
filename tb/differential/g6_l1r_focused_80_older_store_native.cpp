#include <cstdint>
#include <cstdio>
#include <cstdlib>

extern "C" void g6_l1r80_older_store(
    uint8_t, uint8_t, uint8_t, uint32_t, uint64_t, uint64_t, uint8_t, uint8_t,
    uint64_t&, uint64_t&, uint64_t&, uint64_t&, uint64_t&, uint64_t&, uint64_t&, uint64_t&);

static void run_case(int id, uint64_t store_address, uint64_t load_address,
                     uint8_t store_mask, uint8_t load_mask) {
    uint64_t obs[8] = {};
    g6_l1r80_older_store(0, 1, 2, 0x60000000u + (uint32_t)id * 0x100u,
        store_address, load_address, store_mask, load_mask,
        obs[0], obs[1], obs[2], obs[3], obs[4], obs[5], obs[6], obs[7]);
    if (obs[0] != 1 || obs[1] != 0 || obs[2] != store_address ||
        obs[3] != load_address || obs[5] != ((uint64_t)store_mask | ((uint64_t)load_mask << 8)) ||
        obs[6] != (store_address == load_address)) {
        std::fprintf(stderr, "G6_L1R80_NATIVE_FAIL id=%d requirement=older_store_blocks\n", id);
        std::exit(EXIT_FAILURE);
    }
    std::printf("%d,older_store", id);
    for (int i = 0; i < 8; ++i)
        std::printf(",%016llx", (unsigned long long)obs[i]);
    std::printf("\n");
}

int main() {
    std::printf("case_id,family,obs0,obs1,obs2,obs3,obs4,obs5,obs6,obs7\n");
    run_case(26, 0x7000, 0x7000, 0x0f, 0xf0);
    run_case(27, 0x7000, 0x8000, 0xff, 0xff);
    return EXIT_SUCCESS;
}
