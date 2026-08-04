#include "boom_state.hpp"
#include <cstdio>

void synth_w4d_oracle_top(uint8_t scenario, uint64_t* observable);

static int passed, failed;
#define CHECK(c,m) do { if (!(c)) { std::printf("FAIL: %s\n",m); failed++; } else { passed++; } } while (0)

int main() {
    uint64_t value = 0;
    synth_w4d_oracle_top(0, &value);
    CHECK((value & 0xf) == 2 && (value & (3ULL << 16)) == (3ULL << 16),
          "dual-write oracle");
    synth_w4d_oracle_top(1, &value);
    CHECK((value & 0xf) == 0 && (value & (1ULL << 8)) &&
          !(value & (1ULL << 9)), "conflict/fault oracle");
    synth_w4d_oracle_top(2, &value);
    CHECK((value & 0xf) == 0 && (value & (1ULL << 10)) &&
          (value & (1ULL << 13)), "branch-fence oracle");
    synth_w4d_oracle_top(0x82, &value);
    CHECK((value & 0xf) == 1 && (value & (1ULL << 18)),
          "deferred wake/write oracle");
    synth_w4d_oracle_top(3, &value);
    CHECK((value & 0xf) == 2 && (value & (1ULL << 11)),
          "retention first-cycle oracle");
    synth_w4d_oracle_top(0x83, &value);
    CHECK((value & 0xf) == 1 && (value & (7ULL << 19)) == (7ULL << 19),
          "retention drain oracle");
    synth_w4d_oracle_top(4, &value);
    CHECK((value & (1ULL << 8)) && (value & (1ULL << 22)) &&
          !(value & 0xf), "correct-branch cross-boundary conflict oracle");
    synth_w4d_oracle_top(5, &value);
    CHECK((value & (1ULL << 8)) && (value & (1ULL << 11)) &&
          !(value & 0xf), "fault suppresses unrelated younger oracle");
    synth_w4d_oracle_top(6, &value);
    CHECK(!(value & (1ULL << 8)) && (value & (1ULL << 23)) &&
           (value & (1ULL << 24)) && !(value & (1ULL << 25)) &&
           (value & 0xf) == 1, "mispredict-killed non-conflict oracle");
    synth_w4d_oracle_top(7, &value);
    CHECK((value & (1ULL << 8)) && (value & (1ULL << 11)) &&
          (value & (15ULL << 26)) == (15ULL << 26) && !(value & 0xf) &&
           !(value & (15ULL << 4)), "persistent exception publication fence oracle");
    synth_w4d_oracle_top(16, &value);
    CHECK(value == 0x3f, "production-order queued response retention oracle");
    std::printf("W4D RTL oracle preparation: %d passed, %d failed\n", passed, failed);
    return failed ? 1 : 0;
}
