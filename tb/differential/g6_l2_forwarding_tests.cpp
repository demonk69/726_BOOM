#include "boom_config.hpp"
#include "boom_state.hpp"
#include "boom_interfaces.hpp"
#include "completion.hpp"
#include "reset.hpp"
#include <cstdio>
#include <cstdlib>
#include <vector>

namespace boom {
bool lsu_accept_completion(BoomCoreState&, const MicroOp&, bool, bool, bool,
                           uint64_t, uint64_t, uint8_t, uint8_t);
bool lsu_finish_load_response(BoomCoreState&, uint8_t, uint32_t, uint32_t);
bool lsu_finish_forwarded_load(BoomCoreState&, LqIndex, uint16_t, uint8_t, uint32_t);
bool lsu_reclaim_store(BoomCoreState&, SqIndex, uint16_t, uint8_t, uint32_t);
void lsu_module(BoomCoreState&, PipeSignals&);
void rob_commit_module(BoomCoreState&, PipeSignals&);
void branch_complete_event(BoomCoreState&, const MicroOp&, bool, uint64_t);
}

enum Decision { BLOCK, MEMORY, FORWARD };

struct OracleStore {
    bool resolved;
    uint64_t address;
    uint64_t data;
    uint8_t mask;
    OracleStore(bool r, uint64_t a, uint64_t d, uint8_t m)
        : resolved(r), address(a), data(d), mask(m) {}
};

struct OracleResult {
    Decision decision;
    uint64_t value;
    int selected;
    OracleResult() : decision(BLOCK), value(0), selected(-1) {}
};

static unsigned long long checks;
static unsigned long long failures;
static unsigned long long directed_cases;
static unsigned long long exhaustive_cases;
static unsigned long long random_cases;
static unsigned long long seen[3];
static unsigned long long random_dmem_backpressure;
static unsigned long long random_completion_backpressure;
static unsigned long long random_branch_kills;
static unsigned long long random_flushes;
static unsigned long long random_sq_reuse;
static unsigned long long random_rob_reuse;

#ifdef __VITIS_HLS__
static const bool kStreamFullModel = false;
#else
static const bool kStreamFullModel = true;
#endif

static void check(bool condition, const char* name) {
    checks++;
    if (!condition) {
        failures++;
        if (failures <= 20) std::printf("FAIL: %s\n", name);
    }
}

static uint64_t oracle_extend(uint64_t value, uint8_t size, bool sign) {
    const unsigned bytes = 1u << (size & 3u);
    const unsigned bits = bytes * 8u;
    const uint64_t mask = bits == 64 ? ~0ULL : ((1ULL << bits) - 1ULL);
    value &= mask;
    if (sign && bits != 64 && (value & (1ULL << (bits - 1)))) value |= ~mask;
    return value;
}

// The oracle uses absolute byte addresses. It deliberately has no dependency
// on LSU queue entries or implementation-side overlap/extraction routines.
static OracleResult oracle(const std::vector<OracleStore>& older,
                           uint64_t load_address, uint8_t size, bool sign) {
    OracleResult result;
    const unsigned bytes = 1u << (size & 3u);
    for (unsigned i = 0; i < older.size(); i++) {
        if (!older[i].resolved) return result;
        bool overlap = false;
        for (unsigned lb = 0; lb < bytes; lb++)
            for (unsigned sb = 0; sb < 8; sb++)
                if ((older[i].mask & (1u << sb)) &&
                    older[i].address + sb == load_address + lb) overlap = true;
        if (overlap) result.selected = (int)i;
    }
    if (result.selected < 0) {
        result.decision = MEMORY;
        return result;
    }
    const OracleStore& selected = older[(unsigned)result.selected];
    uint64_t raw = 0;
    for (unsigned lb = 0; lb < bytes; lb++) {
        bool covered = false;
        uint8_t byte = 0;
        for (unsigned sb = 0; sb < 8; sb++) {
            if ((selected.mask & (1u << sb)) &&
                selected.address + sb == load_address + lb) {
                covered = true;
                byte = (uint8_t)(selected.data >> (8 * sb));
            }
        }
        if (!covered) return result;
        raw |= (uint64_t)byte << (8 * lb);
    }
    result.decision = FORWARD;
    result.value = oracle_extend(raw, size, sign);
    return result;
}

static MicroOp uop(uint8_t rob, uint32_t allocation, bool load, bool store,
                   uint8_t branch_mask = 0) {
    MicroOp value;
    value.queue.rob_idx = rob;
    value.queue.rob_allocation_id = allocation;
    value.ctrl.is_load = load;
    value.ctrl.is_sta = store;
    value.mem.uses_ldq = load;
    value.mem.uses_stq = store;
    value.branch.br_mask = branch_mask;
    value.rename.dst_rtype = load ? DST_INT : DST_N;
    value.rename.pdst = load ? 7 : 0;
    return value;
}

static void install(BoomCoreState& state, const MicroOp& value) {
    RobEntry& entry = state.rob.entries[value.queue.rob_idx];
    entry = RobEntry();
    entry.valid = true;
    entry.busy = true;
    entry.uop = value;
}

struct ProductCase {
    BoomCoreState state;
    PipeSignals pipe;
    uint8_t load_rob;
    uint32_t load_allocation;
};

static bool build_case(ProductCase& product,
                       const std::vector<OracleStore>& older,
                       const std::vector<OracleStore>& younger,
                       uint64_t load_address, uint8_t size, bool sign,
                       uint8_t head = 0, uint8_t branch_mask = 0) {
    product.state.rob.head = head;
    unsigned age = 0;
    uint32_t allocation = 100;
    for (unsigned i = 0; i < older.size(); i++, age++, allocation++) {
        uint8_t index = (uint8_t)((head + age) % ROB_DEPTH);
        MicroOp op = uop(index, allocation, false, true, branch_mask);
        install(product.state, op);
        if (older[i].resolved) {
            if (!boom::lsu_accept_completion(product.state, op, false, true, false,
                                              older[i].address, older[i].data,
                                              older[i].mask, 3)) return false;
        }
    }
    product.load_rob = (uint8_t)((head + age) % ROB_DEPTH);
    product.load_allocation = allocation++;
    MicroOp load = uop(product.load_rob, product.load_allocation, true, false,
                       branch_mask);
    install(product.state, load);
    uint8_t load_mask = size == 3 ? 0xff : (uint8_t)((1u << (1u << size)) - 1u);
    if (!boom::lsu_accept_completion(product.state, load, true, false, sign,
                                     load_address, 0, load_mask, size)) return false;
    age++;
    for (unsigned i = 0; i < younger.size(); i++, age++, allocation++) {
        uint8_t index = (uint8_t)((head + age) % ROB_DEPTH);
        MicroOp op = uop(index, allocation, false, true, branch_mask);
        install(product.state, op);
        if (!boom::lsu_accept_completion(product.state, op, false, true, false,
                                         younger[i].address, younger[i].data,
                                         younger[i].mask, 3)) return false;
    }
    product.state.rob.tail = (uint8_t)((head + age) % ROB_DEPTH);
    return true;
}

static Decision observed(const ProductCase& product) {
    if (product.state.completion.load_response.valid &&
        product.state.completion.load_response.forwarded_load) return FORWARD;
    if (product.state.lsu.load_response_pending) return MEMORY;
    return BLOCK;
}

static void compare_case(const std::vector<OracleStore>& older,
                         const std::vector<OracleStore>& younger,
                         uint64_t address, uint8_t size, bool sign,
                         const char* name, uint8_t head = 0) {
    ProductCase product;
    check(build_case(product, older, younger, address, size, sign, head),
          "case construction");
    OracleResult expected = oracle(older, address, size, sign);
    boom::lsu_module(product.state, product.pipe);
    Decision actual = observed(product);
    seen[(int)expected.decision]++;
    check(actual == expected.decision, name);
    if (expected.decision == FORWARD) {
        check(product.state.completion.load_response.forwarded_load,
              "forward completion marker");
        check(product.state.completion.load_response.value == expected.value,
              "forward value/extension");
        check(product.pipe.dmem_req.empty() &&
              !product.state.lsu.load_response_pending &&
              !product.state.rob.entries[product.load_rob].memory_request_sent,
              "forward has no request or pending transaction");
    }
}

static void directed() {
    const uint64_t pattern = 0x80ff7f0102030481ULL;
    std::vector<OracleStore> none;
    std::vector<OracleStore> one;
    compare_case(none, none, 0x1003, 2, false, "no older store");
    one.push_back(OracleStore(true, 0x2000, pattern, 0xff));
    compare_case(one, none, 0x1003, 2, false, "known nonalias");

    const char* exact_names[4][2] = {
        {"LB exact", "LBU exact"}, {"LH exact", "LHU exact"},
        {"LW exact", "LWU exact"}, {"LD exact signed", "LD exact unsigned"}
    };
    const char* wide_names[4][2] = {
        {"LB wide", "LBU wide"}, {"LH wide", "LHU wide"},
        {"LW wide", "LWU wide"}, {"LD wide signed", "LD wide unsigned"}
    };
    for (uint8_t size = 0; size < 4; size++) {
        for (int sign = 0; sign < 2; sign++) {
            std::vector<OracleStore> stores;
            unsigned bytes = 1u << size;
            stores.push_back(OracleStore(true, 0x1010, pattern,
                bytes == 8 ? 0xff : (uint8_t)((1u << bytes) - 1u)));
            compare_case(stores, none, 0x1010, size, sign != 0,
                         exact_names[size][sign]);
            directed_cases++;
            stores.clear();
            stores.push_back(OracleStore(true, 0x1000, pattern, 0xff));
            compare_case(stores, none, 0x1000 + (size == 3 ? 0 : 1), size,
                         sign != 0, wide_names[size][sign]);
            directed_cases++;
        }
    }

    std::vector<OracleStore> youngest;
    youngest.push_back(OracleStore(true, 0x1000, 0x1111111111111111ULL, 0xff));
    youngest.push_back(OracleStore(true, 0x1000, 0x8877665544332211ULL, 0xff));
    compare_case(youngest, none, 0x1002, 2, false, "youngest full overlap");
    youngest[1].mask = 0x04;
    compare_case(youngest, none, 0x1002, 2, false,
                 "older full plus younger partial blocks");
    std::vector<OracleStore> union_partial;
    union_partial.push_back(OracleStore(true, 0x1000, pattern, 0x03));
    union_partial.push_back(OracleStore(true, 0x1000, pattern, 0x0c));
    compare_case(union_partial, none, 0x1000, 2, false,
                 "union of partial stores blocks");
    std::vector<OracleStore> younger;
    younger.push_back(OracleStore(true, 0x1000, 0, 0x01));
    one[0] = OracleStore(true, 0x1000, pattern, 0xff);
    compare_case(one, younger, 0x1000, 3, false, "younger overlap ignored");

    std::vector<OracleStore> unresolved;
    unresolved.push_back(OracleStore(false, 0, 0, 0));
    compare_case(unresolved, none, 0x1000, 0, false, "unresolved older ROB store");
    compare_case(one, none, 0x1004, 1, false, "ROB wrap forwarding",
                 (uint8_t)(ROB_DEPTH - 1));

    ProductCase stale_generation;
    check(build_case(stale_generation, one, none, 0x1000, 3, false),
          "stale generation setup");
    stale_generation.state.rob.entries[0].uop.queue.stq_generation++;
    boom::lsu_module(stale_generation.state, stale_generation.pipe);
    check(observed(stale_generation) == BLOCK, "stale SQ generation blocks");
    ProductCase stale_allocation;
    check(build_case(stale_allocation, one, none, 0x1000, 3, false),
          "stale allocation setup");
    stale_allocation.state.lsu.stq[0].rob_allocation_id++;
    boom::lsu_module(stale_allocation.state, stale_allocation.pipe);
    check(observed(stale_allocation) == BLOCK, "stale allocation blocks");

    ProductCase full_pipe;
    check(build_case(full_pipe, one, none, 0x1000, 3, false), "full pipe setup");
    if (kStreamFullModel)
        for (int i = 0; i < 1024; i++) full_pipe.pipe.dmem_req.write(DmemRequest());
    boom::lsu_module(full_pipe.state, full_pipe.pipe);
    check(observed(full_pipe) == FORWARD,
          "forwarding independent of DMEM request backpressure");

    ProductCase completion_busy;
    check(build_case(completion_busy, one, none, 0x1000, 3, false),
          "completion backpressure setup");
    completion_busy.state.completion.load_response.valid = true;
    boom::lsu_module(completion_busy.state, completion_busy.pipe);
    check(!completion_busy.state.lsu.load_response_pending &&
          completion_busy.pipe.dmem_req.empty(), "completion backpressure blocks issue");
    completion_busy.state.completion.load_response = RobCompleteEvent();
    boom::lsu_module(completion_busy.state, completion_busy.pipe);
    check(observed(completion_busy) == FORWARD,
          "completion release permits forwarding");

    ProductCase completed_forward;
    check(build_case(completed_forward, one, none, 0x1000, 3, false),
          "forward completion setup");
    boom::lsu_module(completed_forward.state, completed_forward.pipe);
    uint64_t expected_forward = oracle(one, 0x1000, 3, false).value;
    boom::completion_service_cycle(completed_forward.state,
                                   completed_forward.pipe);
    check(!completed_forward.state.completion.load_response.valid &&
          !completed_forward.state.rob.entries[completed_forward.load_rob].busy &&
          completed_forward.state.rob.entries[completed_forward.load_rob].memory_completed &&
          completed_forward.state.rob.entries[completed_forward.load_rob].memory_data ==
              expected_forward &&
          completed_forward.state.lsu.ldq_count == 0 &&
          boom::prf_read(completed_forward.state, 7) == expected_forward,
          "forward uses canonical retained completion and writeback");

    ProductCase memory;
    check(build_case(memory, none, none, 0x1003, 1, true), "memory path setup");
    boom::lsu_module(memory.state, memory.pipe);
    check(observed(memory) == MEMORY, "unchanged memory request path");
    DmemRequest request = memory.pipe.dmem_req.read();
    DmemResponse response;
    response.transaction_id = request.transaction_id;
    response.data = 0x0000000080000000ULL;
    memory.pipe.dmem_resp.write(response);
    boom::completion_service_cycle(memory.state, memory.pipe);
    check(memory.state.rob.entries[memory.load_rob].memory_completed &&
          memory.state.rob.entries[memory.load_rob].memory_data ==
              oracle_extend(response.data >> 24, 1, true),
          "unchanged memory response extraction");

    ProductCase reuse;
    check(build_case(reuse, one, none, 0x1000, 3, false), "queue reuse setup");
    boom::lsu_module(reuse.state, reuse.pipe);
    LqIndex old_lq = (LqIndex)reuse.state.rob.entries[reuse.load_rob].uop.queue.ldq_idx;
    uint16_t old_lq_generation = reuse.state.rob.entries[reuse.load_rob].uop.queue.ldq_generation;
    check(boom::lsu_finish_forwarded_load(reuse.state, old_lq, old_lq_generation,
                                          reuse.load_rob, reuse.load_allocation),
          "forwarded LQ reclaim");
    int sq = reuse.state.rob.entries[0].uop.queue.stq_idx;
    uint16_t old_sq_generation = reuse.state.lsu.stq[sq].generation;
    check(boom::lsu_reclaim_store(reuse.state, (SqIndex)sq, old_sq_generation,
                                  0, 100), "SQ reclaim");
    reuse.state.completion.load_response = RobCompleteEvent();
    reuse.state.rob.entries[0] = RobEntry();
    reuse.state.rob.entries[reuse.load_rob] = RobEntry();
    reuse.state.lsu.stq_tail = (SqIndex)sq;
    reuse.state.lsu.ldq_tail = old_lq;
    MicroOp new_store = uop(2, 900, false, true);
    install(reuse.state, new_store);
    check(boom::lsu_accept_completion(reuse.state, new_store, false, true, false,
                                      0x1000, pattern, 0xff, 3), "SQ reuse allocation");
    MicroOp new_load = uop(3, 901, true, false);
    install(reuse.state, new_load);
    check(boom::lsu_accept_completion(reuse.state, new_load, true, false, false,
                                      0x1000, 0, 0xff, 3), "LQ reuse allocation");
    check(reuse.state.rob.entries[2].uop.queue.stq_generation != old_sq_generation &&
          reuse.state.rob.entries[3].uop.queue.ldq_generation != old_lq_generation,
          "SQ/LQ reuse increments generations");

    ProductCase committed;
    check(build_case(committed, one, none, 0x1000, 3, false),
          "committed backpressured setup");
    committed.state.rob.entries[0].busy = false;
    committed.state.lsu.stq[0].committed = true;
    if (kStreamFullModel) {
        for (int i = 0; i < 1024; i++)
            committed.pipe.dmem_req.write(DmemRequest());
        boom::rob_commit_module(committed.state, committed.pipe);
        check(committed.state.rob.entries[0].valid &&
              committed.state.lsu.stq[0].valid,
              "backpressured committed store retained");
    }
    boom::lsu_module(committed.state, committed.pipe);
    check(observed(committed) == FORWARD,
          "load forwards from backpressured committing store");

    ProductCase flush;
    check(build_case(flush, one, none, 0x1000, 3, false), "flush setup");
    flush.state.global_flush = true;
    boom::lsu_module(flush.state, flush.pipe);
    check(flush.state.lsu.ldq_count == 0 && flush.state.lsu.stq_count == 0 &&
          !flush.state.completion.load_response.valid,
          "global flush clears forwarding candidates");

    ProductCase killed;
    check(build_case(killed, one, none, 0x1000, 3, false, 1, 1),
          "branch kill setup");
    MicroOp branch = uop(0, 99, false, false);
    branch.branch.is_br = true;
    branch.branch.br_tag = 0;
    install(killed.state, branch);
    killed.state.rob.head = 0;
    killed.state.branch_state.active_mask = 1;
    killed.state.branch_state.tag_valid[0] = true;
    killed.state.branch_state.snapshot_valid[0] = true;
    boom::branch_complete_event(killed.state, branch, true, 0x2000);
    boom::lsu_module(killed.state, killed.pipe);
    check(killed.state.lsu.ldq_count == 0 && killed.state.lsu.stq_count == 0 &&
          killed.pipe.dmem_req.empty(), "branch-killed load/store cannot forward");

    BoomCoreState reset_state;
    reset_state.lsu.ldq[0].valid = true;
    reset_state.lsu.stq[0].valid = true;
    reset_state.lsu.ldq_count = reset_state.lsu.stq_count = 1;
    ResetControllerState reset;
    for (int cycle = 0; !reset.completed && cycle < 512; cycle++)
        boom_core_reset_step(reset_state, reset);
    check(reset.completed && reset_state.lsu.ldq_count == 0 &&
          reset_state.lsu.stq_count == 0, "reset clears forwarding state");
    directed_cases += 21;
}

static void bounded_exhaustive() {
    if (LQ_DEPTH != 4 || SQ_DEPTH != 4) return;
    const uint64_t data = 0x8070605040302010ULL;
    std::vector<OracleStore> stores;
    std::vector<OracleStore> none;
    for (int delta = -7; delta <= 7; delta++) {
        for (unsigned mask = 0; mask < 256; mask++) {
            stores.clear();
            stores.push_back(OracleStore(true, (uint64_t)(0x1008 + delta), data,
                                         (uint8_t)mask));
            for (uint8_t size = 0; size < 4; size++) {
                for (unsigned offset = 0; offset < 8; offset++) {
                    compare_case(stores, none, 0x1008 + offset, size,
                                 (offset & 1u) != 0, "bounded exhaustive");
                    exhaustive_cases++;
                }
            }
        }
    }
}

static uint32_t next_random(uint32_t& state) {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

static void random_differential() {
    const int seeds = (LQ_DEPTH == 8 && SQ_DEPTH == 8) ? 256 : 16;
    const int cycles = (LQ_DEPTH == 8 && SQ_DEPTH == 8) ? 8192 : 1024;
    std::vector<OracleStore> older;
    std::vector<OracleStore> younger;
    for (int seed = 0; seed < seeds; seed++) {
        uint32_t random = 0x6d2b79f5u ^ (uint32_t)seed * 0x9e3779b9u;
        for (int cycle = 0; cycle < cycles; cycle++) {
            uint32_t r = next_random(random);
            older.clear();
            younger.clear();
            unsigned count = (r >> 4) % (SQ_DEPTH < 5 ? SQ_DEPTH : 5);
            uint64_t base = 0x1000 + ((r >> 12) & 0x38);
            for (unsigned i = 0; i < count; i++) {
                uint32_t s = next_random(random);
                older.push_back(OracleStore((s & 31u) != 0,
                    (uint64_t)(base + (int)((s >> 8) % 17) - 8),
                    ((uint64_t)next_random(random) << 32) | next_random(random),
                    (uint8_t)(s >> 24)));
            }
            if ((r & 3u) == 0)
                younger.push_back(OracleStore(true, base, next_random(random),
                                               (uint8_t)(r >> 16)));
            uint8_t size = (uint8_t)(r & 3u);
            uint64_t address = base + ((r >> 2) & 7u);
            unsigned mode = (r >> 20) & 0x3ffu;
            bool branch_kill = mode == 5;
            uint8_t head = (uint8_t)((r >> 10) % ROB_DEPTH);
            ProductCase product;
            check(build_case(product, older, younger, address, size,
                             (r & 0x80000000u) != 0, head,
                             branch_kill ? 1 : 0),
                  "random case construction");
            OracleResult expected = oracle(older, address, size,
                                           (r & 0x80000000u) != 0);
            if (mode == 0 && kStreamFullModel) {
                for (int fill = 0; fill < 1024; fill++)
                    product.pipe.dmem_req.write(DmemRequest());
                if (expected.decision == MEMORY) expected.decision = BLOCK;
                random_dmem_backpressure++;
            } else if (mode == 1) {
                product.state.completion.load_response.valid = true;
                expected.decision = BLOCK;
                random_completion_backpressure++;
            } else if (mode == 2) {
                product.state.global_flush = true;
                expected.decision = BLOCK;
                random_flushes++;
            } else if ((mode == 3 || mode == 4) && !older.empty() &&
                       older[0].resolved) {
                RobEntry& store = product.state.rob.entries[head];
                StoreQueueEntry& sq =
                    product.state.lsu.stq[store.uop.queue.stq_idx];
                if (mode == 3) {
                    sq.generation++;
                    random_sq_reuse++;
                } else {
                    sq.rob_allocation_id++;
                    random_rob_reuse++;
                }
                expected.decision = BLOCK;
            } else if (branch_kill) {
                uint8_t branch_idx = (uint8_t)((head + ROB_DEPTH - 1) % ROB_DEPTH);
                MicroOp branch = uop(branch_idx, 0x70000000u + (uint32_t)cycle,
                                      false, false);
                branch.branch.is_br = true;
                branch.branch.br_tag = 0;
                install(product.state, branch);
                product.state.rob.head = branch_idx;
                product.state.branch_state.active_mask = 1;
                product.state.branch_state.tag_valid[0] = true;
                product.state.branch_state.snapshot_valid[0] = true;
                boom::branch_complete_event(product.state, branch, true, 0x2000);
                expected.decision = BLOCK;
                random_branch_kills++;
            }
            boom::lsu_module(product.state, product.pipe);
            Decision actual = observed(product);
            seen[(int)expected.decision]++;
            check(actual == expected.decision, "random differential");
            if (expected.decision == FORWARD) {
                check(product.state.completion.load_response.value == expected.value,
                      "random forwarded value");
                check(!product.state.lsu.load_response_pending,
                      "random forward has no external pending transaction");
            }
            random_cases++;
        }
    }
}

int main() {
    std::printf("G6_L2_CONFIG LQ=%d SQ=%d\n", LQ_DEPTH, SQ_DEPTH);
    directed();
    bounded_exhaustive();
    random_differential();
    const int seeds = (LQ_DEPTH == 8 && SQ_DEPTH == 8) ? 256 : 16;
    const int cycles = (LQ_DEPTH == 8 && SQ_DEPTH == 8) ? 8192 : 1024;
    std::printf("G6_L2_COUNTS checks=%llu directed=%llu exhaustive=%llu "
                 "random_seeds=%d cycles_per_seed=%d random_cases=%llu "
                 "block=%llu memory=%llu forward=%llu dmem_bp=%llu completion_bp=%llu "
                 "branch_kill=%llu flush=%llu sq_reuse=%llu rob_reuse=%llu "
                 "failures=%llu\n",
                 checks, directed_cases, exhaustive_cases, seeds, cycles,
                 random_cases, seen[BLOCK], seen[MEMORY], seen[FORWARD],
                 random_dmem_backpressure, random_completion_backpressure,
                 random_branch_kills, random_flushes, random_sq_reuse,
                 random_rob_reuse, failures);
    if (failures == 0)
        std::printf("G6_L2_FORWARDING_PASS LQ=%d SQ=%d failures=0\n",
                    LQ_DEPTH, SQ_DEPTH);
    else
        std::printf("G6_L2_FORWARDING_FAIL LQ=%d SQ=%d failures=%llu\n",
                    LQ_DEPTH, SQ_DEPTH, failures);
    return failures ? 1 : 0;
}
