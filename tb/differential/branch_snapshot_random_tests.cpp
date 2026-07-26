#include "boom_config.hpp"
#include "boom_types.hpp"
#include "boom_state.hpp"
#include "boom_interfaces.hpp"
#include <cstdio>
#include <cstdlib>

namespace boom {
void rename_module(BoomCoreState& state);
void rob_allocate(BoomCoreState& state);
void branch_module(BoomCoreState& state);
}

static const uint32_t RANDOM_SEED = 0x3A33B007u;
static int tests_passed=0, tests_failed=0;
#define TEST(n) printf("  [BR-RAND] %-58s ... ", n)
#define PASS() do { printf("PASS\n"); tests_passed++; } while(0)
#define FAIL(m) do { printf("FAIL: %s\n", m); tests_failed++; } while(0)
#define CHECK(c,m) do { if(!(c)) { FAIL(m); return; } } while(0)

static uint32_t rng_state = RANDOM_SEED;
static uint32_t rnd() {
    rng_state = rng_state * 1664525u + 1013904223u;
    return rng_state;
}

static uint8_t bit(uint8_t tag) { return (uint8_t)(1u << tag); }

static MicroOp make_branch() {
    MicroOp u;
    u.uopc = 31;
    u.iq_type = IQ_ALU;
    u.fu_code = FU_ALU;
    u.rename.dst_rtype = DST_N;
    u.branch.is_br = true;
    u.inst = 0x00000063u;
    return u;
}

static MicroOp make_add(uint8_t rd) {
    MicroOp u;
    u.uopc = 50;
    u.iq_type = IQ_ALU;
    u.fu_code = FU_ALU;
    u.rename.ldst = rd;
    u.rename.dst_rtype = (rd != 0) ? DST_INT : DST_X0;
    u.inst = 0x00100093u;
    return u;
}

static bool rename_alloc(BoomCoreState& s, const MicroOp& in, MicroOp& out) {
    s.decode.dec_valids[0] = true;
    s.decode.dec_uops[0] = in;
    boom::rename_module(s);
    s.decode.dec_valids[0] = false;
    if (!s.rename.renamed_valids[0]) return false;
    boom::rob_allocate(s);
    out = s.rename.renamed_uops[0];
    return true;
}

static void resolve(BoomCoreState& s, const MicroOp& br, bool mispredict) {
    s.execute.alu_results[0] = ExecuteState::AluResult();
    s.execute.alu_results[0].valid = true;
    s.execute.alu_results[0].uop = br;
    s.execute.alu_results[0].mispredict = mispredict;
    s.execute.alu_results[0].redirect_pc = RESET_VECTOR + 0x80;
    boom::branch_module(s);
}

static int choose_active_tag(uint8_t mask) {
    int active[MAX_BRANCH_COUNT];
    int n=0;
    for (int i=0; i<MAX_BRANCH_COUNT; i++) if ((mask & bit((uint8_t)i)) != 0) active[n++] = i;
    if (n == 0) return -1;
    return active[rnd() % (uint32_t)n];
}

static bool free_queue_has_duplicates(const RenameFreeListState& fl) {
    bool seen[INT_PHYS_REGS];
    for (int i=0; i<INT_PHYS_REGS; i++) seen[i]=false;
    uint8_t idx = fl.head;
    for (int i=0; i<fl.count && i<INT_PHYS_REGS; i++) {
        uint8_t p = fl.free_list[idx];
        if (p < INT_PHYS_REGS) {
            if (seen[p]) return true;
            seen[p] = true;
        }
        idx = (uint8_t)((idx + 1) % INT_PHYS_REGS);
    }
    return false;
}

void random_tag_mask_pressure() { TEST("random branch mask/tag pressure sequence");
    BoomCoreState s;
    MicroOp by_tag[MAX_BRANCH_COUNT];
    bool live[MAX_BRANCH_COUNT];
    for (int i=0; i<MAX_BRANCH_COUNT; i++) live[i]=false;
    uint8_t model_mask = 0;

    for (int step=0; step<300; step++) {
        bool do_alloc = (model_mask == 0) || ((rnd() & 3u) != 0 && model_mask != 0xff);
        if (do_alloc) {
            MicroOp br;
            CHECK(rename_alloc(s, make_branch(), br), "random branch allocation failed");
            CHECK(br.branch.br_mask == model_mask, "allocated branch inherited wrong mask");
            by_tag[br.branch.br_tag] = br;
            live[br.branch.br_tag] = true;
            model_mask |= bit(br.branch.br_tag);
        } else {
            int tag = choose_active_tag(model_mask);
            CHECK(tag >= 0, "no active tag to resolve");
            bool mispredict = (rnd() & 7u) == 0;
            MicroOp br = by_tag[tag];
            resolve(s, br, mispredict);
            if (mispredict) {
                model_mask = br.branch.br_mask;
                for (int i=0; i<MAX_BRANCH_COUNT; i++) live[i] = (model_mask & bit((uint8_t)i)) != 0;
            } else {
                model_mask &= (uint8_t)~bit((uint8_t)tag);
                live[tag] = false;
            }
        }
        CHECK(s.branch_state.active_mask == model_mask, "active mask diverged from model");
        for (int i=0; i<MAX_BRANCH_COUNT; i++) CHECK(s.branch_state.tag_valid[i] == live[i], "tag_valid diverged");
    }
    PASS(); }

void random_rollback_pressure() { TEST("random rollback restores free-list without duplicates");
    for (int iter=0; iter<64; iter++) {
        BoomCoreState s; MicroOp br,tmp;
        CHECK(rename_alloc(s, make_branch(), br), "branch allocation failed");
        uint8_t before = s.rename.int_free_list.count;
        int allocs = 1 + (int)(rnd() % 20u);
        for (int i=0; i<allocs; i++) CHECK(rename_alloc(s, make_add((uint8_t)((rnd()%31u)+1u)), tmp), "post-branch allocation failed");
        resolve(s, br, true);
        CHECK(s.rename.int_free_list.count == before, "free-list count mismatch after rollback");
        CHECK(!free_queue_has_duplicates(s.rename.int_free_list), "free-list duplicate after rollback");
        CHECK(s.branch_state.active_mask == 0, "tag leaked after rollback");
    }
    PASS(); }

int main() {
    printf("=== BOOM-HLS Gate 3.3 Branch Snapshot Random Tests ===\n");
    printf("Seed: 0x%08x\n\n", RANDOM_SEED);
    random_tag_mask_pressure();
    random_rollback_pressure();
    printf("\n=== %d passed, %d failed ===\n", tests_passed, tests_failed);
    return tests_failed ? 1 : 0;
}
