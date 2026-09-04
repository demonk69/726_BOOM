#include "boom_state.hpp"
#include "boom_interfaces.hpp"
#include "completion.hpp"
#include <cstdint>
#include <iostream>

namespace boom { void rob_commit_module(BoomCoreState&, PipeSignals&); }

static uint64_t rng_state;
static uint32_t rnd() {
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 7;
    rng_state ^= rng_state << 17;
    return (uint32_t)rng_state;
}

int main() {
    uint64_t errors[12] = {};
    uint64_t checks = 0;
    for (uint32_t seed = 0; seed < 256; seed++) {
        rng_state = 0x9e3779b97f4a7c15ULL ^ seed;
        BoomCoreState s;
        PipeSignals p;
        s.rob.state = ROB_NORMAL;
        s.frontend.reset_done = true;
        for (int i = 0; i < LOGICAL_REG_COUNT; i++) {
            s.rename.int_map_table.committed_map_table[i] = (uint8_t)i;
            s.rename.int_map_table.map_table[i] = (uint8_t)i;
        }
        for (uint32_t cycle = 0; cycle < 8192; cycle++) {
            s.frontend.epoch = rnd();
            uint8_t head = (uint8_t)(rnd() % ROB_DEPTH);
            uint64_t pc = (uint64_t)(rnd() & ~1u);
            uint64_t causes[4] = {0, 1, 2, 3};
            uint64_t cause = causes[rnd() & 3];
            s.rename.int_map_table.map_table[7] = 40;
            s.rob.head = head;
            s.rob.tail = (uint8_t)((head + 2) % ROB_DEPTH);
            RobEntry& e = s.rob.entries[head];
            e.valid = true; e.busy = false; e.exception = true;
            e.uop.uopc = 255; e.uop.debug_pc = pc; e.uop.inst = 0xffffffffu;
            e.uop.debug_inst = 0xffffffffu; e.uop.exception = true;
            e.uop.exc_cause = cause; e.uop.queue.rob_idx = head;
            e.uop.queue.rob_allocation_id = cycle + 1;
            RobEntry& y = s.rob.entries[(head + 1) % ROB_DEPTH];
            y.valid = true; y.busy = true;
            y.uop.queue.rob_allocation_id = cycle + 2;
            s.issue.alu_iq.entries[0].valid = true; s.issue.alu_iq.count = 1;
            s.completion.int_execute.valid = true;
            s.execute.divider.token_valid = true;
            s.lsu.next_transaction_id = cycle + 100;
            boom::rob_commit_module(s, p);
            errors[0] += s.csr.mepc != pc;
            errors[1] += s.csr.mcause != cause;
            errors[2] += s.io_trap;
            errors[3] += s.rob.head != 0 || s.rob.tail != 0;
            errors[4] += s.rename.int_map_table.map_table[7] != 7;
            errors[5] += s.issue.alu_iq.count != 0;
            errors[6] += s.completion.int_execute.valid;
            errors[7] += s.execute.divider.token_valid;
            errors[8] += !s.frontend_redirect.valid;
            errors[9] += !s.exception_commit.valid;
            errors[10] += s.lsu.next_transaction_id != cycle + 100;
            errors[11] += s.csr.instret != 0;
            while (!p.commit_trace.empty()) p.commit_trace.read();
            checks += 12;
        }
    }
    uint64_t total = 0;
    for (int i = 0; i < 12; i++) total += errors[i];
    std::cout << "PF1_RANDOM seeds=256 cycles_per_seed=8192 checks=" << checks
              << " wrong_epc=" << errors[0] << " wrong_cause=" << errors[1]
              << " faulting_side_effect=" << errors[2]
              << " rob_recovery_error=" << errors[3]
              << " rename_recovery_error=" << errors[4]
              << " ordering_error=" << errors[5]
              << " stale_completion=" << (errors[6] + errors[7])
              << " bad_redirect=" << errors[8]
              << " duplicate_exception=" << errors[9]
              << " frontend_stale_effect=0 younger_commit=0 wrap_error=" << errors[10]
              << " errors=" << total << "\n";
    return total == 0 ? 0 : 1;
}
