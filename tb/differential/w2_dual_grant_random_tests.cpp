#include "boom_config.hpp"
#include "boom_state.hpp"

#include <cstdint>
#include <cstdio>
#include <vector>

namespace boom { void issue_module(BoomCoreState& state); }

namespace {

const int kSeedCount = 64;
const int kCyclesPerSeed = 32;

struct Rng {
    uint32_t state;
    explicit Rng(uint32_t seed) : state(seed ? seed : 1u) {}
    uint32_t next() {
        uint32_t x = state;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        return state = x;
    }
    bool bit() { return (next() & 1u) != 0; }
    uint32_t range(uint32_t n) { return next() % n; }
};

enum RefClass { REF_INT, REF_MEM, REF_UNSUPPORTED };

RefClass ref_classify(const MicroOp& uop) {
    if (uop.exception) return REF_UNSUPPORTED;
    if (uop.iq_type == IQ_MEM && uop.fu_code == FU_MEM &&
        uop.uopc >= 39 && uop.uopc <= 49)
        return REF_MEM;
    if (uop.iq_type != IQ_ALU) return REF_UNSUPPORTED;
    if (uop.fu_code == FU_MUL)
        return uop.uopc == 16 ? REF_INT : REF_UNSUPPORTED;
    if (uop.fu_code != FU_ALU) return REF_UNSUPPORTED;
    if ((uop.uopc >= 1 && uop.uopc <= 13) || uop.uopc == 15 ||
        (uop.uopc >= 29 && uop.uopc <= 38) ||
        (uop.uopc >= 50 && uop.uopc <= 60) || uop.uopc == 62)
        return REF_INT;
    return REF_UNSUPPORTED;
}

const char* class_name(RefClass c) {
    return c == REF_INT ? "INT" : (c == REF_MEM ? "MEM" : "UNSUP");
}

uint8_t class_value(RefClass c) {
    return c == REF_INT ? ISSUE_PORT_INT :
           (c == REF_MEM ? ISSUE_PORT_MEM : ISSUE_PORT_UNSUPPORTED);
}

MicroOp random_uop(Rng& rng, uint64_t token) {
    static const uint8_t int_alu[] = {1, 7, 13, 15, 29, 38, 50, 60, 62};
    MicroOp uop;
    const uint32_t kind = rng.range(3);
    if (kind == 0) {
        uop.iq_type = IQ_ALU;
        if (rng.range(4) == 0) {
            uop.fu_code = FU_MUL;
            uop.uopc = 16;
        } else {
            uop.fu_code = FU_ALU;
            uop.uopc = int_alu[rng.range(sizeof(int_alu) / sizeof(int_alu[0]))];
        }
    } else if (kind == 1) {
        uop.iq_type = IQ_MEM;
        uop.fu_code = FU_MEM;
        uop.uopc = static_cast<uint8_t>(39 + rng.range(11));
    } else {
        switch (rng.range(5)) {
        case 0: uop.iq_type = IQ_FPU; uop.fu_code = FU_FPU; uop.uopc = 1; break;
        case 1: uop.iq_type = IQ_ALU; uop.fu_code = FU_DIV; uop.uopc = 1; break;
        case 2: uop.iq_type = IQ_MEM; uop.fu_code = FU_MEM; uop.uopc = 38; break;
        case 3: uop.iq_type = IQ_ALU; uop.fu_code = FU_ALU; uop.uopc = 14; break;
        default: uop.iq_type = IQ_ALU; uop.fu_code = FU_ALU; uop.uopc = 1;
                 uop.exception = true; break;
        }
    }
    uop.queue.rob_idx = static_cast<uint8_t>(rng.range(ROB_DEPTH));
    uop.debug_pc = 0x10000000ull + token * 4;
    uop.branch.br_mask = static_cast<uint8_t>(rng.next());
    uop.rename.prs1 = rng.range(5) == 0 ? 0 : static_cast<uint8_t>(rng.range(INT_PHYS_REGS));
    uop.rename.prs2 = rng.range(5) == 0 ? 0 : static_cast<uint8_t>(rng.range(INT_PHYS_REGS));
    uop.rename.prs1_busy = rng.bit();
    uop.rename.prs2_busy = rng.bit();
    return uop;
}

struct RefGrant {
    bool valid;
    bool accepted;
    bool from_dispatch;
    uint8_t entry_index;
    uint8_t port_class;
    MicroOp uop;
    RefGrant() : valid(false), accepted(false), from_dispatch(false),
                 entry_index(0xff), port_class(ISSUE_PORT_UNSUPPORTED), uop() {}
};

struct RefResult {
    RefGrant grants[ISSUE_WIDTH];
    bool issued_valids[ISSUE_WIDTH];
    MicroOp issued_uops[ISSUE_WIDTH];
    std::vector<IssueSlotEntry> survivors;
    uint8_t generated;
    uint8_t accepted;
    uint8_t retained;
    uint8_t dropped;
    int killed;
    bool dispatch_bypassed;
    bool dispatch_enqueued;
    RefResult() : generated(0), accepted(0), retained(0), dropped(0), killed(0),
                  dispatch_bypassed(false), dispatch_enqueued(false) {
        for (int lane = 0; lane < ISSUE_WIDTH; ++lane) issued_valids[lane] = false;
    }
};

bool ref_preg_busy(const BoomCoreState& state, uint8_t preg) {
    return preg != 0 && preg < INT_PHYS_REGS &&
           state.rename.int_free_list.busy_table[preg];
}

RefResult reference_step(const BoomCoreState& before) {
    RefResult out;
    std::vector<IssueSlotEntry> work;
    for (int i = 0; i < ISSUE_QUEUE_ALU_DEPTH; ++i) {
        IssueSlotEntry entry = before.issue.alu_iq.entries[i];
        if (!entry.valid) continue;
        if (before.brupdate.valid && before.brupdate.mispredict &&
            (entry.uop.branch.br_mask & before.brupdate.mispredict_mask) != 0) {
            ++out.killed;
            continue;
        }
        if (entry.uop.exception) continue;
        if (before.brupdate.valid)
            entry.uop.branch.br_mask &= static_cast<uint8_t>(~before.brupdate.resolve_mask);
        if (entry.uop.rename.prs1 != 0)
            entry.prs1_busy = ref_preg_busy(before, entry.uop.rename.prs1);
        if (entry.uop.rename.prs2 != 0)
            entry.prs2_busy = ref_preg_busy(before, entry.uop.rename.prs2);
        if (!entry.granted) work.push_back(entry);
    }

    int selected[2] = {-1, -1};
    for (size_t i = 0; i < work.size(); ++i) {
        const IssueSlotEntry& entry = work[i];
        if (!entry.request || entry.killed || entry.prs1_busy ||
            entry.prs2_busy || entry.pdst_busy)
            continue;
        const RefClass port = ref_classify(entry.uop);
        const int lane = port == REF_MEM ? MEM_ISSUE_LANE :
                         (port == REF_INT ? INT_ISSUE_LANE : -1);
        if (lane >= 0 && selected[lane] < 0) selected[lane] = static_cast<int>(i);
    }
    for (int lane = 0; lane < INTEGER_ISSUE_PORTS; ++lane) {
        if (selected[lane] < 0) continue;
        RefGrant& grant = out.grants[lane];
        grant.valid = true;
        grant.entry_index = static_cast<uint8_t>(selected[lane]);
        grant.port_class = lane == MEM_ISSUE_LANE ? ISSUE_PORT_MEM : ISSUE_PORT_INT;
        grant.uop = work[selected[lane]].uop;
    }

    bool dispatch_pending = before.rename.dispatch_packets[0].valid &&
                             before.rename.dispatch_packets[0].rob_allocated &&
                             !before.rename.dispatch_packets[0].uop.exception;
    MicroOp dispatch_uop;
    if (dispatch_pending) {
        dispatch_uop = before.rename.dispatch_packets[0].uop;
        const RefClass port = ref_classify(dispatch_uop);
        const int lane = port == REF_MEM ? MEM_ISSUE_LANE :
                         (port == REF_INT ? INT_ISSUE_LANE : -1);
        const bool dispatch_ready = !ref_preg_busy(before, dispatch_uop.rename.prs1) &&
                                    !ref_preg_busy(before, dispatch_uop.rename.prs2);
        bool old_grant_can_accept = false;
        for (int old_lane = 0; old_lane < INTEGER_ISSUE_PORTS; ++old_lane)
            old_grant_can_accept |= out.grants[old_lane].valid &&
                                    before.issue.port_ready[old_lane];
        const bool dispatch_can_be_preserved = work.size() < ISSUE_QUEUE_ALU_DEPTH ||
                                               (lane >= 0 && before.issue.port_ready[lane]) ||
                                               old_grant_can_accept;
        if (lane >= 0 && dispatch_ready && dispatch_can_be_preserved &&
            !out.grants[lane].valid) {
            RefGrant& grant = out.grants[lane];
            grant.valid = true;
            grant.from_dispatch = true;
            grant.entry_index = 0xff;
            grant.port_class = class_value(port);
            grant.uop = dispatch_uop;
        }
    }

    for (int lane = 0; lane < INTEGER_ISSUE_PORTS; ++lane)
        if (out.grants[lane].valid) ++out.generated;

    for (int lane = 0; lane < INTEGER_ISSUE_PORTS; ++lane) {
        if (!out.grants[lane].valid || !before.issue.port_ready[lane]) continue;
        RefGrant& grant = out.grants[lane];
        grant.accepted = true;
        ++out.accepted;
        out.issued_valids[lane] = true;
        out.issued_uops[lane] = grant.uop;
        if (grant.from_dispatch) {
            dispatch_pending = false;
            out.dispatch_bypassed = true;
        } else {
            work[grant.entry_index].granted = true;
            work[grant.entry_index].request = false;
        }
    }
    out.retained = static_cast<uint8_t>(out.generated - out.accepted);

    for (size_t i = 0; i < work.size(); ++i)
        if (work[i].valid && !work[i].granted) out.survivors.push_back(work[i]);
    if (dispatch_pending && out.survivors.size() < ISSUE_QUEUE_ALU_DEPTH) {
        IssueSlotEntry entry;
        entry.valid = true;
        entry.request = true;
        entry.uop = dispatch_uop;
        entry.prs1_busy = ref_preg_busy(before, dispatch_uop.rename.prs1);
        entry.prs2_busy = ref_preg_busy(before, dispatch_uop.rename.prs2);
        out.survivors.push_back(entry);
        out.dispatch_enqueued = true;
    }
    return out;
}

void print_entry(const IssueSlotEntry& entry, int index) {
    std::printf("  [%d] v=%d req=%d rob=%u pc=0x%llx class=%s br=0x%02x "
                "prs=(%u:%d,%u:%d) pdst_busy=%d\n",
                index, entry.valid, entry.request, entry.uop.queue.rob_idx,
                static_cast<unsigned long long>(entry.uop.debug_pc),
                class_name(ref_classify(entry.uop)), entry.uop.branch.br_mask,
                entry.uop.rename.prs1, entry.prs1_busy,
                entry.uop.rename.prs2, entry.prs2_busy, entry.pdst_busy);
}

void print_grant(const char* label, int lane, bool valid, bool accepted,
                 bool from_dispatch, uint8_t entry_index, uint8_t port_class,
                 const MicroOp& uop) {
    std::printf("  %s lane=%d v=%d acc=%d dispatch=%d entry=%u port=%u rob=%u pc=0x%llx\n",
                label, lane, valid, accepted, from_dispatch, entry_index,
                port_class, uop.queue.rob_idx,
                static_cast<unsigned long long>(uop.debug_pc));
}

void print_failure(uint32_t seed, int cycle, const BoomCoreState& before,
                   const RefResult& expected, const BoomCoreState& actual,
                   const char* reason) {
    std::printf("FAIL seed=0x%08x cycle=%d: %s\n", seed, cycle, reason);
    std::printf("Initial IQ count=%u br=(valid=%d mispredict=%d resolve=0x%02x kill=0x%02x) "
                "ports=0x%x dispatch=%d\n",
                before.issue.alu_iq.count, before.brupdate.valid,
                before.brupdate.mispredict, before.brupdate.resolve_mask,
                before.brupdate.mispredict_mask,
                (before.issue.port_ready[0] ? 1 : 0) |
                (before.issue.port_ready[1] ? 2 : 0),
                 before.rename.dispatch_packets[0].valid);
    for (int i = 0; i < ISSUE_QUEUE_ALU_DEPTH; ++i)
        if (before.issue.alu_iq.entries[i].valid) print_entry(before.issue.alu_iq.entries[i], i);
    if (before.rename.dispatch_packets[0].valid) {
        IssueSlotEntry dispatch;
        dispatch.valid = true;
        dispatch.uop = before.rename.dispatch_packets[0].uop;
        print_entry(dispatch, 0xff);
    }
    std::printf("Expected generated=%u accepted=%u retained=%u dropped=%u survivors=%zu\n",
                expected.generated, expected.accepted, expected.retained,
                expected.dropped, expected.survivors.size());
    for (int lane = 0; lane < ISSUE_WIDTH; ++lane)
        print_grant("EXP", lane, expected.grants[lane].valid,
                    expected.grants[lane].accepted, expected.grants[lane].from_dispatch,
                    expected.grants[lane].entry_index, expected.grants[lane].port_class,
                    expected.grants[lane].uop);
    for (size_t i = 0; i < expected.survivors.size(); ++i)
        print_entry(expected.survivors[i], static_cast<int>(i));
    std::printf("Actual generated=%u accepted=%u retained=%u dropped=%u survivors=%u\n",
                actual.issue.grants_generated, actual.issue.grants_accepted,
                actual.issue.grants_retained, actual.issue.grants_dropped,
                actual.issue.alu_iq.count);
    for (int lane = 0; lane < ISSUE_WIDTH; ++lane) {
        const IssueGrant& grant = actual.issue.grants[lane];
        print_grant("ACT", lane, grant.valid, grant.accepted, grant.from_dispatch,
                    grant.entry_index, grant.port_class, grant.uop);
    }
    for (int i = 0; i < actual.issue.alu_iq.count; ++i)
        print_entry(actual.issue.alu_iq.entries[i], i);
}

bool same_survivor(const IssueSlotEntry& expected, const IssueSlotEntry& actual) {
    return actual.valid == expected.valid && actual.request == expected.request &&
           actual.granted == expected.granted && actual.killed == expected.killed &&
           actual.prs1_busy == expected.prs1_busy &&
           actual.prs2_busy == expected.prs2_busy &&
           actual.pdst_busy == expected.pdst_busy &&
           actual.uop.debug_pc == expected.uop.debug_pc &&
           actual.uop.queue.rob_idx == expected.uop.queue.rob_idx &&
           actual.uop.branch.br_mask == expected.uop.branch.br_mask &&
           ref_classify(actual.uop) == ref_classify(expected.uop);
}

const char* compare(const RefResult& expected, const BoomCoreState& actual) {
    if (actual.issue.grants_generated != expected.generated) return "generated count";
    if (actual.issue.grants_accepted != expected.accepted) return "accepted count";
    if (actual.issue.grants_retained != expected.retained) return "retained count";
    if (actual.issue.grants_dropped != 0 || expected.dropped != 0) return "dropped is not zero";
    for (int lane = 0; lane < ISSUE_WIDTH; ++lane) {
        const RefGrant& e = expected.grants[lane];
        const IssueGrant& a = actual.issue.grants[lane];
        if (a.valid != e.valid) return "grant valid/fixed lane";
        if (a.accepted != e.accepted) return "accepted grant lane";
        if (a.from_dispatch != e.from_dispatch) return "grant dispatch source";
        if (a.entry_index != e.entry_index) return "grant original entry index";
        if (a.port_class != e.port_class) return "grant port class";
        if (e.valid && (a.uop.queue.rob_idx != e.uop.queue.rob_idx ||
                        a.uop.debug_pc != e.uop.debug_pc))
            return "grant rob_idx/identity";
        if (actual.issue.issued_valids[lane] != expected.issued_valids[lane])
            return "issued valid";
        if (expected.issued_valids[lane] &&
            actual.issue.issued_uops[lane].queue.rob_idx != expected.issued_uops[lane].queue.rob_idx)
            return "issued rob_idx";
    }
    if (actual.issue.alu_iq.count != expected.survivors.size()) return "IQ count";
    if (actual.issue.alu_iq.head != 0) return "IQ compacted head";
    if (actual.issue.alu_iq.tail != expected.survivors.size() % ISSUE_QUEUE_ALU_DEPTH)
        return "IQ compacted tail";
    for (size_t i = 0; i < expected.survivors.size(); ++i)
        if (!same_survivor(expected.survivors[i], actual.issue.alu_iq.entries[i]))
            return "survivor order/content";
    for (size_t i = expected.survivors.size(); i < ISSUE_QUEUE_ALU_DEPTH; ++i)
        if (actual.issue.alu_iq.entries[i].valid) return "valid entry beyond IQ count";
    return 0;
}

void initialize_iq(BoomCoreState& state, Rng& rng, uint64_t& token,
                   uint64_t class_counts[3]) {
    const int count = static_cast<int>(rng.range(ISSUE_QUEUE_ALU_DEPTH + 1));
    for (int i = 0; i < count; ++i) {
        IssueSlotEntry& entry = state.issue.alu_iq.entries[i];
        entry.valid = true;
        entry.request = rng.range(5) != 0;
        entry.pdst_busy = rng.range(5) == 0;
        entry.prs1_busy = rng.bit();
        entry.prs2_busy = rng.bit();
        entry.uop = random_uop(rng, token++);
        ++class_counts[ref_classify(entry.uop)];
    }
    state.issue.alu_iq.count = static_cast<uint8_t>(count);
    state.issue.alu_iq.tail = static_cast<uint8_t>(count % ISSUE_QUEUE_ALU_DEPTH);
}

void randomize_cycle_inputs(BoomCoreState& state, Rng& rng, uint64_t& token,
                            uint64_t class_counts[3]) {
    for (int preg = 0; preg < INT_PHYS_REGS; ++preg)
        state.rename.int_free_list.busy_table[preg] = preg != 0 && rng.range(3) == 0;

    state.brupdate = BranchUpdate();
    if (rng.range(2) == 0) {
        state.brupdate.valid = true;
        state.brupdate.mispredict = rng.bit();
        state.brupdate.resolve_mask = static_cast<uint8_t>(1u << rng.range(BR_MASK_BITS));
        if (rng.bit())
            state.brupdate.resolve_mask |= static_cast<uint8_t>(1u << rng.range(BR_MASK_BITS));
        state.brupdate.mispredict_mask = state.brupdate.mispredict ?
            static_cast<uint8_t>(1u << rng.range(BR_MASK_BITS)) : 0;
    }

    for (int lane = 0; lane < ISSUE_WIDTH; ++lane)
        state.issue.port_ready[lane] = lane != FP_ISSUE_LANE && rng.bit();

    state.rename.dispatch_packets[0] = RenameDispatchPacket();
    state.rename.dispatch_packets[0].valid = rng.range(2) == 0;
    state.rename.dispatch_packets[0].rob_allocated = state.rename.dispatch_packets[0].valid;
    if (state.rename.dispatch_packets[0].valid) {
        state.rename.dispatch_packets[0].uop = random_uop(rng, token++);
        ++class_counts[ref_classify(state.rename.dispatch_packets[0].uop)];
    }
}

}  // namespace

int main() {
    uint64_t class_counts[3] = {0, 0, 0};
    uint64_t branch_resolves = 0, branch_mispredicts = 0, killed = 0;
    uint64_t dispatches = 0, dispatch_bypasses = 0, dispatch_enqueues = 0;
    uint64_t dual_grants = 0, accepts = 0, retained = 0;
    uint64_t port_masks[4] = {0, 0, 0, 0};

    for (int seed_index = 0; seed_index < kSeedCount; ++seed_index) {
        const uint32_t seed = 0x6d2b79f5u ^
            (0x9e3779b9u * static_cast<uint32_t>(seed_index + 1));
        Rng rng(seed);
        BoomCoreState state;
        uint64_t token = static_cast<uint64_t>(seed_index) << 32;
        initialize_iq(state, rng, token, class_counts);

        for (int cycle = 0; cycle < kCyclesPerSeed; ++cycle) {
            randomize_cycle_inputs(state, rng, token, class_counts);
            BoomCoreState before = state;
            const RefResult expected = reference_step(before);

            if (before.brupdate.valid) ++branch_resolves;
            if (before.brupdate.valid && before.brupdate.mispredict) ++branch_mispredicts;
            if (before.rename.dispatch_packets[0].valid) ++dispatches;
            const unsigned port_mask = (before.issue.port_ready[0] ? 1u : 0u) |
                                       (before.issue.port_ready[1] ? 2u : 0u);
            ++port_masks[port_mask];
            killed += expected.killed;
            dispatch_bypasses += expected.dispatch_bypassed;
            dispatch_enqueues += expected.dispatch_enqueued;
            dual_grants += expected.generated == 2;
            accepts += expected.accepted;
            retained += expected.retained;

            boom::issue_module(state);
            const char* reason = compare(expected, state);
            if (reason) {
                print_failure(seed, cycle, before, expected, state, reason);
                return 1;
            }
        }
    }

    std::printf("W2 dual-grant random differential: PASS\n");
    std::printf("coverage seeds=%d cycles/seed=%d total_cycles=%d "
                "classes(INT/MEM/UNSUP)=%llu/%llu/%llu\n",
                kSeedCount, kCyclesPerSeed, kSeedCount * kCyclesPerSeed,
                static_cast<unsigned long long>(class_counts[REF_INT]),
                static_cast<unsigned long long>(class_counts[REF_MEM]),
                static_cast<unsigned long long>(class_counts[REF_UNSUPPORTED]));
    std::printf("coverage branch(resolve/mispredict/killed)=%llu/%llu/%llu "
                "dispatch(total/bypass/enqueue)=%llu/%llu/%llu\n",
                static_cast<unsigned long long>(branch_resolves),
                static_cast<unsigned long long>(branch_mispredicts),
                static_cast<unsigned long long>(killed),
                static_cast<unsigned long long>(dispatches),
                static_cast<unsigned long long>(dispatch_bypasses),
                static_cast<unsigned long long>(dispatch_enqueues));
    std::printf("coverage port_ready_masks(00/01/10/11)=%llu/%llu/%llu/%llu "
                "dual_grants=%llu accepts=%llu retained=%llu dropped=0\n",
                static_cast<unsigned long long>(port_masks[0]),
                static_cast<unsigned long long>(port_masks[1]),
                static_cast<unsigned long long>(port_masks[2]),
                static_cast<unsigned long long>(port_masks[3]),
                static_cast<unsigned long long>(dual_grants),
                static_cast<unsigned long long>(accepts),
                static_cast<unsigned long long>(retained));
    return 0;
}
